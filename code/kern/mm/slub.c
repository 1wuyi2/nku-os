#include <slub.h>
#include <pmm.h>
#include <string.h>

/* All metadata lives inside owned pages. Page fields remain owned by PMM. */
struct slub_cache {
    size_t size, slabs, objects;
    list_entry_t partial, full;
};

struct slab {
    list_entry_t all_link, state_link;
    struct slub_cache *cache;         /* NULL denotes a large allocation */
    struct Page *page;
    size_t npages, charged, capacity, inuse, requested;
    void *freelist;
    uint64_t allocated[4];            /* at most 256 minimum-size objects */
};

#define HEADER_SIZE ROUNDUP(sizeof(struct slab), SLUB_ALIGNMENT)

static struct slub_cache caches[SLUB_CACHE_COUNT];
static list_entry_t active;
static bool initialized;

void slub_init(void) {
    if (initialized) {
        return;                      /* never reset lists containing live objects */
    }
    list_init(&active);
    for (size_t i = 0; i < SLUB_CACHE_COUNT; i++) {
        caches[i].size = (size_t)SLUB_ALIGNMENT << i;
        list_init(&caches[i].partial);
        list_init(&caches[i].full);
    }
    initialized = 1;
}

static void *slot(struct slab *s, size_t index) {
    return (char *)s + HEADER_SIZE + index * s->cache->size;
}

static bool is_allocated(struct slab *s, size_t index) {
    return (s->allocated[index / 64] >> (index % 64)) & 1;
}

/* Walk only trusted metadata; never dereference an untrusted free pointer. */
static struct slab *find_object(const void *ptr, size_t *index) {
    if (!initialized || ptr == NULL) {
        return NULL;
    }
    uintptr_t addr = (uintptr_t)ptr;
    for (list_entry_t *le = list_next(&active); le != &active; le = list_next(le)) {
        struct slab *s = to_struct(le, struct slab, all_link);
        uintptr_t start = (uintptr_t)s + HEADER_SIZE;
        if (s->cache == NULL) {
            if (addr == start) {
                *index = 0;
                return s;
            }
        } else if (addr >= start && addr - start < s->capacity * s->cache->size &&
                   (addr - start) % s->cache->size == 0) {
            *index = (addr - start) / s->cache->size;
            return is_allocated(s, *index) ? s : NULL;
        }
    }
    return NULL;
}

static struct slab *new_slab(struct slub_cache *cache, size_t size) {
    /* Guard both header addition and page rounding before calling the PMM. */
    if (size > (size_t)-1 - HEADER_SIZE - (PGSIZE - 1)) {
        return NULL;
    }
    size_t npages = cache ? 1 : (size + HEADER_SIZE + PGSIZE - 1) / PGSIZE;
    size_t before = nr_free_pages();
    if (npages > before) {
        return NULL;
    }
    struct Page *page = alloc_pages(npages);
    if (page == NULL) {
        return NULL;
    }
    struct slab *s = (void *)(page2pa(page) + va_pa_offset);
    memset(s, 0, sizeof(*s));
    s->page = page;
    s->npages = npages;
    /* Buddy may round up; free_pages still requires the original request. */
    s->charged = before - nr_free_pages();
    s->cache = cache;
    s->requested = size;
    list_init(&s->state_link);
    list_add(&active, &s->all_link);
    if (cache != NULL) {
        s->capacity = (PGSIZE - HEADER_SIZE) / cache->size;
        assert(s->capacity > 0 && s->capacity <= 256);
        for (size_t i = s->capacity; i > 0; i--) {
            void *obj = slot(s, i - 1);
            *(void **)obj = s->freelist;
            s->freelist = obj;
        }
        list_add(&cache->partial, &s->state_link);
        cache->slabs++;
    } else {
        s->capacity = s->inuse = 1;
    }
    return s;
}

void *slub_alloc(size_t size) {
    assert(initialized);
    if (size == 0) {
        return NULL;
    }
    if (size > SLUB_SMALL_MAX) {
        struct slab *s = new_slab(NULL, size);
        return s ? (char *)s + HEADER_SIZE : NULL;
    }
    struct slub_cache *cache = &caches[0];
    while (cache->size < size) {
        cache++;
    }
    struct slab *s;
    if (list_empty(&cache->partial)) {
        s = new_slab(cache, size);
        if (s == NULL) {
            return NULL;
        }
    } else {
        s = to_struct(list_next(&cache->partial), struct slab, state_link);
    }
    void *obj = s->freelist;
    size_t index = ((uintptr_t)obj - (uintptr_t)s - HEADER_SIZE) / cache->size;
    assert(index < s->capacity && !is_allocated(s, index));
    s->freelist = *(void **)obj;
    s->allocated[index / 64] |= (uint64_t)1 << (index % 64);
    s->inuse++;
    cache->objects++;
    if (s->inuse == s->capacity) {
        assert(s->freelist == NULL);
        list_del(&s->state_link);
        list_add(&cache->full, &s->state_link);
    }
    return obj;
}

bool slub_free(void *ptr) {
    if (ptr == NULL) {
        return 1;
    }
    size_t index;
    struct slab *s = find_object(ptr, &index);
    if (s == NULL) {
        return 0;
    }
    if (s->cache != NULL) {
        struct slub_cache *cache = s->cache;
        if (s->inuse == s->capacity) {
            list_del(&s->state_link);
            list_add(&cache->partial, &s->state_link);
        }
        s->allocated[index / 64] &= ~((uint64_t)1 << (index % 64));
        *(void **)ptr = s->freelist;
        s->freelist = ptr;
        s->inuse--;
        cache->objects--;
        if (s->inuse != 0) {
            return 1;
        }
        list_del(&s->state_link);
        cache->slabs--;
    }
    /* Remove from every registry before returning the page to the PMM. */
    struct Page *page = s->page;
    size_t npages = s->npages;
    list_del(&s->all_link);
    free_pages(page, npages);
    return 1;
}

size_t slub_usable_size(const void *ptr) {
    size_t index;
    struct slab *s = find_object(ptr, &index);
    return s ? (s->cache ? s->cache->size : s->requested) : 0;
}

void slub_get_stats(struct slub_stats *stats) {
    assert(initialized && stats != NULL);
    memset(stats, 0, sizeof(*stats));
    for (list_entry_t *le = list_next(&active); le != &active; le = list_next(le)) {
        struct slab *s = to_struct(le, struct slab, all_link);
        stats->backing_pages += s->charged;
        stats->objects += s->inuse;
        if (s->cache == NULL) {
            stats->large++;
        } else {
            stats->slabs++;
            if (s->inuse == s->capacity) {
                stats->full++;
            } else {
                stats->partial++;
            }
        }
    }
}

static void check_link(list_entry_t *le) {
    assert(le->next->prev == le && le->prev->next == le);
}

void slub_validate(void) {
    assert(initialized);
    check_link(&active);
    size_t registered = 0, listed = 0;
    for (list_entry_t *le = list_next(&active); le != &active; le = list_next(le)) {
        check_link(le);
        struct slab *s = to_struct(le, struct slab, all_link);
        assert((uintptr_t)s == page2pa(s->page) + va_pa_offset);
        assert(s->npages > 0 && s->charged >= s->npages);
        assert(s->charged <= npage - page2ppn(s->page));
        for (size_t i = 0; i < s->charged; i++) {
            assert(!PageReserved(s->page + i) && !PageProperty(s->page + i));
        }
        for (list_entry_t *other = list_next(le); other != &active; other = list_next(other)) {
            struct slab *t = to_struct(other, struct slab, all_link);
            assert(s->page + s->charged <= t->page || t->page + t->charged <= s->page);
        }
        if (s->cache == NULL) {
            assert(s->inuse == 1 && s->requested > SLUB_SMALL_MAX);
            assert(s->requested <= s->npages * PGSIZE - HEADER_SIZE);
            assert(list_empty(&s->state_link));
            continue;
        }
        registered++;
        assert(s->npages == 1 && s->charged == 1);
        assert(s->capacity == (PGSIZE - HEADER_SIZE) / s->cache->size);
        assert(s->inuse > 0 && s->inuse <= s->capacity);
        uint64_t seen[4] = {0, 0, 0, 0};
        size_t nfree = 0, used = 0;
        for (void *obj = s->freelist; obj != NULL; obj = *(void **)obj) {
            uintptr_t start = (uintptr_t)s + HEADER_SIZE, addr = (uintptr_t)obj;
            assert(addr >= start && addr - start < s->capacity * s->cache->size);
            assert((addr - start) % s->cache->size == 0);
            size_t index = (addr - start) / s->cache->size;
            assert(!is_allocated(s, index));
            uint64_t mask = (uint64_t)1 << (index % 64);
            assert(!(seen[index / 64] & mask));
            seen[index / 64] |= mask;
            nfree++;
        }
        for (size_t i = 0; i < 256; i++) {
            if (i < s->capacity) {
                bool allocated = is_allocated(s, i);
                assert(allocated != ((seen[i / 64] >> (i % 64)) & 1));
                used += allocated;
            } else {
                assert(!is_allocated(s, i));
            }
        }
        assert(used == s->inuse && used + nfree == s->capacity);
    }
    for (size_t i = 0; i < SLUB_CACHE_COUNT; i++) {
        struct slub_cache *c = &caches[i];
        assert(c->size == ((size_t)SLUB_ALIGNMENT << i));
        size_t slabs = 0, objects = 0;
        for (int full = 0; full <= 1; full++) {
            list_entry_t *head = full ? &c->full : &c->partial;
            check_link(head);
            for (list_entry_t *le = list_next(head); le != head; le = list_next(le)) {
                check_link(le);
                struct slab *s = to_struct(le, struct slab, state_link);
                assert(s->cache == c && (s->inuse == s->capacity) == full);
                bool found = 0;
                for (list_entry_t *a = list_next(&active); a != &active; a = list_next(a)) {
                    if (a == &s->all_link) {
                        found = 1;
                    }
                }
                assert(found);
                slabs++;
                objects += s->inuse;
            }
        }
        assert(slabs == c->slabs && objects == c->objects);
        listed += slabs;
    }
    assert(registered == listed);
}
