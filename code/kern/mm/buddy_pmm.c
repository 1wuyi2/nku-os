#include <buddy_pmm.h>
#include <stdio.h>

#define MAX_ORDER 20
#define ALLOCATED_HEAD (1UL << 2)
#define CHECKED_PAGE (1UL << 3)

static list_entry_t free_lists[MAX_ORDER + 1];
static size_t nr_free;

static size_t order_size(unsigned int order) {
    return (size_t)1 << order;
}

static unsigned int request_order(size_t n) {
    unsigned int order = 0;
    while (order_size(order) < n) {
        order++;
    }
    return order;
}

static void insert_block(struct Page *page, unsigned int order) {
    page->property = order_size(order);
    SetPageProperty(page);
    list_add(&free_lists[order], &page->page_link);
}

static void remove_block(struct Page *page) {
    list_del_init(&page->page_link);
    ClearPageProperty(page);
    page->property = 0;
}

static void merge_block(struct Page *page, unsigned int order) {
    size_t ppn = page2ppn(page);
    while (order < MAX_ORDER) {
        size_t buddy_ppn = ppn ^ order_size(order);
        if (buddy_ppn < nbase || buddy_ppn >= npage ||
            order_size(order) > npage - buddy_ppn) {
            break;
        }
        struct Page *buddy = &pages[buddy_ppn - nbase];
        if (!PageProperty(buddy) || buddy->property != order_size(order)) {
            break;
        }
        remove_block(buddy);
        if (buddy_ppn < ppn) {
            ppn = buddy_ppn;
            page = buddy;
        }
        order++;
    }
    insert_block(page, order);
}

static void buddy_init(void) {
    for (unsigned int order = 0; order <= MAX_ORDER; order++) {
        list_init(&free_lists[order]);
    }
    nr_free = 0;
}

static void buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0 && base >= pages && base < pages + npage - nbase);
    assert(n <= (size_t)(pages + npage - nbase - base));
    for (size_t i = 0; i < n; i++) {
        assert(PageReserved(base + i));
        base[i].flags = 0;
        base[i].property = 0;
        set_page_ref(base + i, 0);
        list_init(&base[i].page_link);
    }
    nr_free += n;
    while (n > 0) {
        unsigned int order = 0;
        size_t ppn = page2ppn(base);
        while (order < MAX_ORDER && order_size(order + 1) <= n &&
               (ppn & (order_size(order + 1) - 1)) == 0) {
            order++;
        }
        merge_block(base, order);
        base += order_size(order);
        n -= order_size(order);
    }
}

static struct Page *buddy_alloc_pages(size_t n) {
    if (n == 0 || n > nr_free || n > order_size(MAX_ORDER)) {
        return NULL;
    }
    unsigned int order = request_order(n), found = order;
    while (found <= MAX_ORDER && list_empty(&free_lists[found])) {
        found++;
    }
    if (found > MAX_ORDER) {
        return NULL;
    }
    struct Page *page = le2page(list_next(&free_lists[found]), page_link);
    remove_block(page);
    while (found > order) {
        found--;
        insert_block(page + order_size(found), found);
    }
    nr_free -= order_size(order);
    page->flags |= ALLOCATED_HEAD;
    page->property = n;
    return page;
}

static void buddy_free_pages(struct Page *base, size_t n) {
    assert(n > 0 && n <= order_size(MAX_ORDER));
    assert(base >= pages && base < pages + npage - nbase);
    assert((base->flags & ALLOCATED_HEAD) && base->property == n);
    unsigned int order = request_order(n);
    size_t size = order_size(order);
    assert((page2ppn(base) & (size - 1)) == 0);
    assert(size <= (size_t)(pages + npage - nbase - base));
    for (size_t i = 0; i < size; i++) {
        assert(!PageReserved(base + i) && !PageProperty(base + i));
        base[i].flags = 0;
        base[i].property = 0;
        set_page_ref(base + i, 0);
    }
    nr_free += size;
    merge_block(base, order);
}

static size_t buddy_nr_free_pages(void) {
    return nr_free;
}

static void check_state(void) {
    size_t total = 0;
    for (unsigned int order = 0; order <= MAX_ORDER; order++) {
        list_entry_t *head = &free_lists[order];
        assert(head->next->prev == head && head->prev->next == head);
        for (list_entry_t *le = list_next(head); le != head; le = list_next(le)) {
            assert(le->next->prev == le && le->prev->next == le);
            struct Page *page = le2page(le, page_link);
            size_t size = order_size(order), ppn = page2ppn(page);
            assert(ppn >= nbase && ppn < npage && size <= npage - ppn);
            assert((ppn & (size - 1)) == 0);
            assert(PageProperty(page) && page->property == size);
            for (size_t i = 0; i < size; i++) {
                assert(!PageReserved(page + i));
                assert(!(page[i].flags & (ALLOCATED_HEAD | CHECKED_PAGE)));
                page[i].flags |= CHECKED_PAGE;
            }
            size_t buddy_ppn = ppn ^ size;
            if (order < MAX_ORDER && buddy_ppn >= nbase && buddy_ppn < npage) {
                struct Page *buddy = &pages[buddy_ppn - nbase];
                assert(!PageProperty(buddy) || buddy->property != size);
            }
            total += size;
        }
    }
    assert(total == nr_free);
    for (unsigned int order = 0; order <= MAX_ORDER; order++) {
        list_entry_t *head = &free_lists[order];
        for (list_entry_t *le = list_next(head); le != head; le = list_next(le)) {
            struct Page *page = le2page(le, page_link);
            for (size_t i = 0; i < order_size(order); i++) {
                page[i].flags &= ~CHECKED_PAGE;
            }
        }
    }
}

static void move_list(list_entry_t *dest, list_entry_t *source) {
    list_init(dest);
    if (!list_empty(source)) {
        dest->next = source->next;
        dest->prev = source->prev;
        dest->next->prev = dest;
        dest->prev->next = dest;
        list_init(source);
    }
}

static void prepare_arena(struct Page *arena, size_t offset, size_t n) {
    buddy_init();
    for (size_t i = 0; i < 64; i++) {
        arena[i].flags = 0;
        arena[i].property = 0;
        SetPageReserved(arena + i);
    }
    buddy_init_memmap(arena + offset, n);
    check_state();
}

static void buddy_check(void) {
    size_t initial = nr_free_pages();
    check_state();
    cprintf("[Buddy] initial free pages: %lu\n", initial);
    struct Page *arena = alloc_pages(64);
    assert(arena != NULL);
    list_entry_t saved[MAX_ORDER + 1];
    size_t saved_free = nr_free;
    for (unsigned int order = 0; order <= MAX_ORDER; order++) {
        move_list(&saved[order], &free_lists[order]);
        for (list_entry_t *le = list_next(&saved[order]); le != &saved[order];
             le = list_next(le)) {
            ClearPageProperty(le2page(le, page_link));
        }
    }

    struct Page *slots[64];
    prepare_arena(arena, 1, 61);
    assert(nr_free_pages() == 61);
    for (size_t i = 0; i < 61; i++) {
        slots[i] = alloc_page();
        assert(slots[i] >= arena + 1 && slots[i] < arena + 62);
        for (size_t j = 0; j < i; j++) {
            assert(slots[i] != slots[j]);
        }
    }
    assert(alloc_page() == NULL && nr_free_pages() == 0);
    for (size_t i = 0; i < 61; i++) {
        free_page(slots[i]);
    }
    check_state();
    assert(nr_free_pages() == 61);
    cprintf("[Buddy Test 1] unaligned 61-page range: PASS\n");

    prepare_arena(arena, 0, 64);
    struct Page *a = alloc_pages(3), *b = alloc_pages(5);
    assert(a != NULL && b != NULL && nr_free_pages() == 52);
    assert((page2ppn(a) & 3) == 0 && (page2ppn(b) & 7) == 0);
    assert(a + 4 <= b || b + 8 <= a);
    for (size_t i = 0; i < 3; i++) {
        *(volatile size_t *)(page2pa(a + i) + va_pa_offset) = 0x12340000 + i;
    }
    for (size_t i = 0; i < 5; i++) {
        *(volatile size_t *)(page2pa(b + i) + va_pa_offset) = 0x56780000 + i;
    }
    for (size_t i = 0; i < 3; i++) {
        assert(*(volatile size_t *)(page2pa(a + i) + va_pa_offset) == 0x12340000 + i);
    }
    for (size_t i = 0; i < 5; i++) {
        assert(*(volatile size_t *)(page2pa(b + i) + va_pa_offset) == 0x56780000 + i);
    }
    free_pages(a, 3);
    free_pages(b, 5);
    assert(nr_free_pages() == 64);
    check_state();
    cprintf("[Buddy Test 2] rounding 3->4, 5->8 and data isolation: PASS\n");

    a = alloc_page();
    b = alloc_page();
    assert(a == arena && b == arena + 1);
    free_page(a);
    check_state();
    free_page(b);
    assert((a = alloc_pages(64)) == arena && nr_free_pages() == 0);
    free_pages(a, 64);
    check_state();
    cprintf("[Buddy Test 3] split and recursive merge to 64 pages: PASS\n");

    a = alloc_pages(16);
    b = alloc_pages(16);
    struct Page *c = alloc_pages(32);
    assert(a == arena && b == arena + 16 && c == arena + 32);
    free_pages(a, 16);
    free_pages(c, 32);
    assert(nr_free_pages() == 48 && alloc_pages(64) == NULL);
    check_state();
    free_pages(b, 16);
    assert((a = alloc_pages(64)) == arena);
    free_pages(a, 64);
    cprintf("[Buddy Test 4] different orders do not merge prematurely: PASS\n");

    for (size_t i = 0; i < 64; i++) {
        slots[i] = alloc_page();
        assert(slots[i] != NULL);
    }
    assert(alloc_page() == NULL && nr_free_pages() == 0);
    for (size_t i = 0; i < 64; i += 2) {
        free_page(slots[i]);
    }
    assert(nr_free_pages() == 32 && alloc_pages(2) == NULL);
    check_state();
    for (size_t i = 1; i < 64; i += 2) {
        free_page(slots[i]);
    }
    check_state();
    assert((a = alloc_pages(64)) == arena);
    free_pages(a, 64);
    cprintf("[Buddy Test 5] fragmented 32 free pages reject 2-page request: PASS\n");

    assert(alloc_pages(0) == NULL);
    assert(alloc_pages((size_t)-1) == NULL);
    assert(alloc_pages(65) == NULL && nr_free_pages() == 64);
    cprintf("[Buddy Test 6] zero, oversized and overflow requests: PASS\n");

    for (size_t i = 0; i < 32; i++) {
        slots[i] = NULL;
    }
    size_t requests[32];
    uint32_t seed = 2413581;
    size_t used = 0, successes = 0, failures = 0;
    for (size_t step = 0; step < 512; step++) {
        seed = seed * 1664525U + 1013904223U;
        size_t index = (seed >> 16) % 32;
        if (slots[index] != NULL) {
            used -= order_size(request_order(requests[index]));
            free_pages(slots[index], requests[index]);
            slots[index] = NULL;
        } else {
            size_t n = ((seed >> 24) % 9) + 1;
            struct Page *p = alloc_pages(n);
            if (p != NULL) {
                size_t size = order_size(request_order(n));
                assert(p >= arena && p + size <= arena + 64);
                assert((page2ppn(p) & (size - 1)) == 0);
                for (size_t j = 0; j < 32; j++) {
                    if (slots[j] != NULL) {
                        size_t other = order_size(request_order(requests[j]));
                        assert(p + size <= slots[j] || slots[j] + other <= p);
                    }
                }
                slots[index] = p;
                requests[index] = n;
                used += size;
                successes++;
            } else {
                failures++;
            }
        }
        assert(nr_free_pages() + used == 64);
        check_state();
    }
    for (size_t i = 0; i < 32; i++) {
        if (slots[i] != NULL) {
            free_pages(slots[i], requests[i]);
        }
    }
    assert(successes > 0 && failures > 0 && nr_free_pages() == 64);
    assert((a = alloc_pages(64)) == arena);
    free_pages(a, 64);
    check_state();
    cprintf("[Buddy Test 7] 512-operation stress: PASS (%lu alloc, %lu failures)\n",
            successes, failures);

    assert(alloc_pages(64) == arena);
    assert(nr_free == 0);
    for (unsigned int order = 0; order <= MAX_ORDER; order++) {
        assert(list_empty(&free_lists[order]));
        move_list(&free_lists[order], &saved[order]);
        for (list_entry_t *le = list_next(&free_lists[order]); le != &free_lists[order];
             le = list_next(le)) {
            SetPageProperty(le2page(le, page_link));
        }
    }
    nr_free = saved_free;
    free_pages(arena, 64);
    check_state();
    assert(nr_free_pages() == initial);
    cprintf("[Buddy Test 8] real memory restored: PASS (%lu -> %lu pages)\n",
            initial, nr_free_pages());
    cprintf("[Buddy] ALL 8 TESTS PASSED\n");
}

const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};
