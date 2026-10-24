#ifdef SLUB_SELFTEST
#include <slub.h>
#include <pmm.h>
#include <stdio.h>
#include <string.h>

static void *objects[600];
static struct {
    void *ptr;
    size_t size;
    uint8_t pattern;
} live[64];
static struct Page *held[128];
static size_t held_sizes[128];

static void verify_bytes(const void *ptr, size_t n, uint8_t pattern) {
    const uint8_t *p = ptr;
    for (size_t i = 0; i < n; i++) {
        assert(p[i] == pattern);
    }
}

static void verify_pointer(void *ptr, size_t n) {
    assert(ptr != NULL && (uintptr_t)ptr >= KERNBASE);
    assert((uintptr_t)ptr % SLUB_ALIGNMENT == 0);
    size_t capacity = slub_usable_size(ptr);
    assert(capacity >= n);
    assert(PADDR(ptr) < npage * PGSIZE);
    assert(capacity <= npage * PGSIZE - PADDR(ptr));
}

static void verify_restored(size_t initial) {
    struct slub_stats stats;
    slub_validate();
    slub_get_stats(&stats);
    assert(stats.slabs == 0 && stats.large == 0 && stats.objects == 0);
    assert(stats.partial == 0 && stats.full == 0 && stats.backing_pages == 0);
    assert(nr_free_pages() == initial);
}

static void verify_live(size_t initial) {
    size_t count = 0;
    for (size_t i = 0; i < 64; i++) {
        if (live[i].ptr == NULL) {
            continue;
        }
        count++;
        verify_pointer(live[i].ptr, live[i].size);
        verify_bytes(live[i].ptr, live[i].size, live[i].pattern);
        uintptr_t a = (uintptr_t)live[i].ptr;
        size_t an = slub_usable_size(live[i].ptr);
        for (size_t j = 0; j < i; j++) {
            if (live[j].ptr != NULL) {
                uintptr_t b = (uintptr_t)live[j].ptr;
                size_t bn = slub_usable_size(live[j].ptr);
                assert(a + an <= b || b + bn <= a);
            }
        }
    }
    slub_validate();
    struct slub_stats stats;
    slub_get_stats(&stats);
    assert(stats.objects == count);
    assert(nr_free_pages() + stats.backing_pages == initial);
}

void slub_check(void) {
    size_t initial = nr_free_pages();
    struct slub_stats stats;
    slub_init();
    verify_restored(initial);
    cprintf("[SLUB Test 1] Initialization: PASS\n");

    for (size_t i = 0; i < 8; i++) {
        objects[i] = slub_alloc(16);
        verify_pointer(objects[i], 16);
        assert(ROUNDDOWN((uintptr_t)objects[i], PGSIZE) ==
               ROUNDDOWN((uintptr_t)objects[0], PGSIZE));
        for (size_t j = 0; j < i; j++) {
            assert((uintptr_t)objects[j] + 16 <= (uintptr_t)objects[i] ||
                   (uintptr_t)objects[i] + 16 <= (uintptr_t)objects[j]);
        }
    }
    slub_get_stats(&stats);
    assert(stats.slabs == 1 && stats.partial == 1 && stats.objects == 8);
    assert(nr_free_pages() == initial - 1);
    slub_validate();
    for (size_t i = 0; i < 8; i++) {
        assert(slub_free(objects[i]));
    }
    verify_restored(initial);
    cprintf("[SLUB Test 2] Eight 16-byte objects share one page: PASS\n");

    static const size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 20, 100, 300};
    static const size_t classes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 32, 128, 512};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        objects[i] = slub_alloc(sizes[i]);
        verify_pointer(objects[i], sizes[i]);
        assert(slub_usable_size(objects[i]) == classes[i]);
        memset(objects[i], (int)i + 17, sizes[i]);
    }
    slub_init();                      /* reinitialization must preserve live objects */
    slub_validate();
    cprintf("[SLUB Test 3] Eight classes, rounding and alignment: PASS\n");
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        verify_bytes(objects[i], sizes[i], i + 17);
        for (size_t j = 0; j < i; j++) {
            uintptr_t a = (uintptr_t)objects[i], b = (uintptr_t)objects[j];
            assert(a + classes[i] <= b || b + classes[j] <= a);
        }
    }
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        assert(slub_free(objects[i]));
    }
    verify_restored(initial);
    cprintf("[SLUB Test 4] Data isolation across caches: PASS\n");

    for (size_t i = 0; i < 8; i++) {
        objects[i] = slub_alloc(64);
        verify_pointer(objects[i], 64);
        memset(objects[i], i + 32, 64);
    }
    assert(slub_free(objects[3]));
    assert(!slub_free(objects[3]));
    assert(slub_usable_size(objects[3]) == 0);
    void *reused = slub_alloc(64);
    assert(reused == objects[3]);
    memset(reused, 35, 64);
    for (size_t i = 0; i < 8; i++) {
        verify_bytes(objects[i], 64, i + 32);
    }
    for (size_t i = 0; i < 8; i++) {
        assert(slub_free(objects[i]));
    }
    verify_restored(initial);
    cprintf("[SLUB Test 5] Freed slots reused, neighbours intact: PASS\n");

    size_t first_capacity = 0;
    for (size_t i = 0; i < 600; i++) {
        objects[i] = slub_alloc(16);
        verify_pointer(objects[i], 16);
        memset(objects[i], (i % 251) + 1, 16);
        if (first_capacity == 0 && ROUNDDOWN((uintptr_t)objects[i], PGSIZE) !=
                                   ROUNDDOWN((uintptr_t)objects[0], PGSIZE)) {
            first_capacity = i;
            slub_get_stats(&stats);
            assert(stats.slabs == 2 && stats.full == 1 && stats.partial == 1);
            assert(slub_free(objects[0]));
            slub_get_stats(&stats);
            assert(stats.full == 0 && stats.partial == 2);
            void *p = slub_alloc(16);
            assert(p == objects[0]);
            memset(p, 1, 16);
        }
        slub_validate();
    }
    assert(first_capacity > 8 && first_capacity < 256);
    slub_get_stats(&stats);
    assert(stats.slabs >= 3 && stats.full >= 2);
    for (size_t i = 0; i < 600; i++) {
        verify_bytes(objects[i], 16, (i % 251) + 1);
        for (size_t j = 0; j < i; j++) {
            assert(objects[i] != objects[j]);
        }
    }
    for (size_t i = 0; i < 600; i++) {
        assert(slub_free(objects[i]));
        slub_validate();
    }
    verify_restored(initial);
    cprintf("[SLUB Test 6] 600 objects, full/partial/empty transitions: PASS (capacity=%lu)\n",
            first_capacity);

    assert(slub_alloc(0) == NULL);
    assert(slub_alloc((size_t)-1) == NULL);
    assert(slub_alloc((size_t)-PGSIZE) == NULL);
    assert(slub_alloc(npage * PGSIZE) == NULL);
    verify_restored(initial);
    void *anchor = slub_alloc(16);
    assert(anchor != NULL);
    memset(anchor, 0xa5, 16);
    size_t nheld = 0;
    /* Exhaust the real PMM without changing any allocator internals. */
    while (nr_free_pages() > 0) {
        assert(nheld < 128);
        size_t n = nr_free_pages();
        struct Page *p;
        while ((p = alloc_pages(n)) == NULL) {
            assert(n > 1);
            n /= 2;
        }
        held[nheld] = p;
        held_sizes[nheld++] = n;
    }
    slub_get_stats(&stats);
    assert(stats.slabs == 1 && stats.objects == 1);
    assert(slub_alloc(2048) == NULL && slub_alloc(8193) == NULL);
    void *spare = slub_alloc(16);       /* an existing partial slab still works */
    assert(spare != NULL && nr_free_pages() == 0);
    assert(slub_free(spare));
    verify_bytes(anchor, 16, 0xa5);
    slub_validate();
    slub_get_stats(&stats);
    assert(stats.slabs == 1 && stats.objects == 1 && stats.backing_pages == 1);
    assert(slub_free(anchor));
    for (size_t i = nheld; i > 0; i--) {
        free_pages(held[i - 1], held_sizes[i - 1]);
    }
    verify_restored(initial);
    cprintf("[SLUB Test 7] Zero, oversized, overflow and real PMM exhaustion: PASS\n");

    for (size_t i = 0; i < 8; i++) {
        objects[i] = slub_alloc((size_t)16 << i);
        assert(objects[i] != NULL);
    }
    for (size_t i = 8; i > 0; i--) {
        assert(slub_free(objects[i - 1]));
    }
    verify_restored(initial);
    cprintf("[SLUB Test 8] All physical pages reclaimed: PASS (%lu -> %lu)\n",
            initial, nr_free_pages());

    uint32_t seed = 2411302;
    size_t allocations = 0, releases = 0;
    static const size_t mixed[] = {1, 15, 16, 17, 20, 31, 32, 33, 64, 100,
                                 128, 255, 256, 300, 512, 1024, 2048, 2049, 4096, 8193};
    for (size_t step = 0; step < 1024; step++) {
        verify_live(initial);
        seed = seed * 1664525U + 1013904223U;
        size_t i = (seed >> 16) % 64;
        if (live[i].ptr != NULL) {
            assert(slub_free(live[i].ptr));
            live[i].ptr = NULL;
            releases++;
        } else {
            size_t n = mixed[(seed >> 8) % (sizeof(mixed) / sizeof(mixed[0]))];
            void *p = slub_alloc(n);
            verify_pointer(p, n);
            live[i].ptr = p;
            live[i].size = n;
            live[i].pattern = (seed >> 24) | 1;
            memset(p, live[i].pattern, n);
            allocations++;
        }
        verify_live(initial);
    }
    for (size_t i = 0; i < 64; i++) {
        assert(slub_free(live[i].ptr));
        live[i].ptr = NULL;
    }
    verify_restored(initial);
    cprintf("[SLUB Test 9] 1024-operation stress: PASS (%lu alloc, %lu free, seed=2411302)\n",
            allocations, releases);

    static const size_t large_sizes[] = {2049, 4096, 8193, 65536};
    for (size_t i = 0; i < 4; i++) {
        objects[i] = slub_alloc(large_sizes[i]);
        verify_pointer(objects[i], large_sizes[i]);
        assert(slub_usable_size(objects[i]) == large_sizes[i]);
        memset(objects[i], i + 0x50, large_sizes[i]);
    }
    slub_get_stats(&stats);
    assert(stats.large == 4 && stats.slabs == 0 && stats.objects == 4);
    assert(nr_free_pages() + stats.backing_pages == initial);
    slub_validate();
    for (size_t i = 0; i < 4; i++) {
        verify_bytes(objects[i], large_sizes[i], i + 0x50);
        assert(!slub_free((char *)objects[i] + 16));
        assert(slub_free(objects[i]));
        assert(!slub_free(objects[i]));
    }
    verify_restored(initial);
    cprintf("[SLUB Test 10] Large objects through 64 KiB, all bytes checked: PASS\n");

    void *p = slub_alloc(32);
    assert(p != NULL);
    memset(p, 0x6b, 32);
    struct Page *foreign = alloc_page();
    assert(foreign != NULL);
    void *foreign_ptr = (void *)(page2pa(foreign) + va_pa_offset);
    size_t before = nr_free_pages();
    assert(slub_free(NULL));
    assert(!slub_free((void *)1));
    assert(!slub_free((void *)&initial));
    assert(!slub_free(foreign_ptr));
    assert(!slub_free((char *)p + 1));
    assert(!slub_free((char *)p + 16));
    assert(!slub_free((void *)ROUNDDOWN((uintptr_t)p, PGSIZE)));
    assert(slub_usable_size((char *)p + 1) == 0);
    assert(nr_free_pages() == before);
    verify_bytes(p, 32, 0x6b);
    slub_validate();
    assert(slub_free(p) && !slub_free(p));
    free_page(foreign);
    verify_restored(initial);
    cprintf("[SLUB Test 11] Invalid, interior, metadata and double frees rejected: PASS\n");
    cprintf("[SLUB] ALL TESTS PASSED\n");
}
#endif
