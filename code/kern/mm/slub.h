#ifndef __KERN_MM_SLUB_H__
#define __KERN_MM_SLUB_H__

#include <defs.h>

#define SLUB_ALIGNMENT 16
#define SLUB_CACHE_COUNT 8
#define SLUB_SMALL_MAX 2048

struct slub_stats {
    size_t slabs, partial, full, large, objects, backing_pages;
};

/* Single-hart boot-time allocator; callers must serialize access. */
void slub_init(void);
void *slub_alloc(size_t size);
/* NULL is a successful no-op; invalid/interior/already-free pointers return 0. */
bool slub_free(void *ptr);
/* Zero for pointers which are not the start of a live allocation. */
size_t slub_usable_size(const void *ptr);
void slub_get_stats(struct slub_stats *stats);
/* Assert list, bitmap, freelist and accounting invariants. */
void slub_validate(void);
#ifdef SLUB_SELFTEST
void slub_check(void);
#endif

#endif
