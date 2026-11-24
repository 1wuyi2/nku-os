#include <sbi.h>

static struct mm_struct *f3_parent;
static struct proc_struct *f3_user;
static int f3_fail_after = -1;
static bool f3_fail_pcb;
#define F3_VA (UTEXT + PGSIZE)
#define F3_RO (UTEXT + 3 * PGSIZE)
#define F3_FAR (UTEXT + PTSIZE)

struct Page *f3_alloc_pages(size_t n)
{
    if (f3_fail_after == 0) return NULL;
    if (f3_fail_after > 0) f3_fail_after--;
    return alloc_pages(n);
}

static void *f3_alloc_pcb(size_t n)
{
    return f3_fail_pcb ? NULL : kmalloc(n);
}

static struct mm_struct *f3_new_mm(void)
{
    struct mm_struct *mm = mm_create();
    assert(mm != NULL && setup_pgdir(mm) == 0);
    set_mm_count(mm, 1);
    return mm;
}

static char *f3_data(struct mm_struct *mm, uintptr_t va)
{
    pte_t *ptep = get_pte(mm->pgdir, va, 0);
    assert(ptep != NULL && (*ptep & PTE_V));
    return (char *)page2kva(pte2page(*ptep)) + PGOFF(va);
}

static void f3_free_mm(struct mm_struct *mm)
{
    if (mm_count_dec(mm) == 0)
    {
        exit_mmap(mm);
        put_pgdir(mm);
        mm_destroy(mm);
    }
}

static void f3_free_child(struct proc_struct *child)
{
    unhash_proc(child);
    remove_links(child);
    if (child->mm != NULL) f3_free_mm(child->mm);
    put_kstack(child);
    kfree(child);
}

static int f3_fork(uint32_t flags)
{
    struct trapframe tf;
    memset(&tf, 0, sizeof(tf));
    tf.gpr.a0 = 0x123;
    tf.gpr.sp = USTACKTOP;
    return do_fork(flags, 0, &tf);
}

static void f3_map(struct mm_struct *mm, uintptr_t va, size_t len, uint32_t flags)
{
    assert(mm_map(mm, va, len, flags, NULL) == 0);
    struct Page *page = pgdir_alloc_page(mm->pgdir, va,
        PTE_U | PTE_R | PTE_A | ((flags & VM_WRITE) ? PTE_W | PTE_D : 0));
    assert(page != NULL);
    memset(page2kva(page), 0, PGSIZE);
}

static void f3_test(void)
{
    intr_disable();
    size_t baseline = nr_free_pages();
    struct proc_struct *initial_child = current->cptr;
    f3_parent = f3_new_mm();
    f3_map(f3_parent, F3_VA, 2 * PGSIZE, VM_READ | VM_WRITE);
    f3_map(f3_parent, F3_RO, PGSIZE, VM_READ);
    f3_map(f3_parent, F3_FAR, PGSIZE, VM_READ | VM_WRITE);
    f3_data(f3_parent, F3_VA)[0] = 0x21;
    f3_data(f3_parent, F3_RO)[0] = 0x42;
    current->mm = f3_parent;
    current->pgdir = PADDR(f3_parent->pgdir);

    struct mm_struct *eager = f3_new_mm();
    assert(mm_map(eager, F3_VA, 2 * PGSIZE, VM_READ | VM_WRITE, NULL) == 0);
    assert(copy_range(eager->pgdir, f3_parent->pgdir, F3_VA, F3_VA + 2 * PGSIZE, 0) == 0);
    assert(f3_data(eager, F3_VA) != f3_data(f3_parent, F3_VA));
    assert(f3_data(eager, F3_VA)[0] == 0x21);
    f3_data(eager, F3_VA)[0] = 0x22;
    assert(f3_data(f3_parent, F3_VA)[0] == 0x21);
    pte_t *hole = get_pte(eager->pgdir, F3_VA + PGSIZE, 0);
    assert(hole == NULL || !(*hole & PTE_V));
    f3_free_mm(eager);
    cprintf("[Lab5 F3] eager copy and unmapped hole PASS\n");

    int pid = f3_fork(0);
    struct proc_struct *child = find_proc(pid);
    assert(child != NULL && child->parent == current && current->cptr == child);
    assert(child->tf->gpr.a0 == 0 && child->tf->gpr.sp == USTACKTOP);
    assert(child->mm != current->mm && child->pgdir == PADDR(child->mm->pgdir));
    assert(child->state == PROC_RUNNABLE && child->mm->map_count == 3);
    assert(f3_data(child->mm, F3_VA) == f3_data(f3_parent, F3_VA));
    struct Page *shared = pte2page(*get_pte(f3_parent->pgdir, F3_VA, 0));
    assert(page_ref(shared) == 2);
    pte_t parent_pte = *get_pte(f3_parent->pgdir, F3_VA, 0);
    pte_t child_pte = *get_pte(child->mm->pgdir, F3_VA, 0);
    assert((parent_pte & (PTE_W | PTE_COW)) == PTE_COW);
    assert((child_pte & (PTE_W | PTE_COW)) == PTE_COW);
    cprintf("[COW demo] After fork: same page, refs=%d, parent W=%d COW=%d, child W=%d COW=%d\n",
            page_ref(shared), !!(parent_pte & PTE_W), !!(parent_pte & PTE_COW),
            !!(child_pte & PTE_W), !!(child_pte & PTE_COW));
    f3_fail_after = 0;
    size_t before = nr_free_pages();
    assert(cow_resolve(child->mm, F3_VA) == -E_NO_MEM);
    f3_fail_after = -1;
    assert(page_ref(shared) == 2 && nr_free_pages() == before);
    assert(cow_resolve(child->mm, F3_VA) == 0);
    f3_data(child->mm, F3_VA)[0] = 0x31;
    assert(f3_data(f3_parent, F3_VA)[0] == 0x21 && page_ref(shared) == 1);
    child_pte = *get_pte(child->mm->pgdir, F3_VA, 0);
    assert((child_pte & (PTE_W | PTE_COW)) == PTE_W);
    assert(pte2page(child_pte) != shared && page_ref(pte2page(child_pte)) == 1);
    cprintf("[COW demo] After child write: separate pages, parent=0x%02x, child=0x%02x\n",
            (unsigned char)f3_data(f3_parent, F3_VA)[0],
            (unsigned char)f3_data(child->mm, F3_VA)[0]);
    before = nr_free_pages();
    assert(cow_resolve(f3_parent, F3_VA) == 0);
    assert(nr_free_pages() == before);
    parent_pte = *get_pte(f3_parent->pgdir, F3_VA, 0);
    assert(pte2page(parent_pte) == shared && (parent_pte & (PTE_W | PTE_COW)) == PTE_W);
    cprintf("[COW demo] Parent writes next: same frame, refs=1, W=1 COW=0\n");
    assert(cow_resolve(child->mm, F3_RO) == -E_FAULT);
    assert(!copy_to_user(child->mm, (void *)F3_RO, "x", 1));
    assert(f3_data(child->mm, F3_RO)[0] == 0x42);
    cprintf("[COW demo] Read-only write rejected: page still holds 0x42\n");
    assert(!copy_to_user(child->mm, (void *)(USERTOP - 1), "xx", 2));
    assert(!copy_to_user(child->mm, (void *)(F3_VA + PGSIZE), "x", 1));
    uintptr_t edge = F3_VA + PGSIZE - 1;
    *f3_data(child->mm, edge) = 'A';
    assert(!copy_to_user(child->mm, (void *)edge, "BC", 2));
    assert(*f3_data(child->mm, edge) == 'A');
    cprintf("[COW demo] Cross-page write with unmapped second page leaves first byte unchanged PASS\n");
    f3_free_child(child);
    cprintf("[Lab5 F3] fork tree/trapframe, COW split/exclusive/OOM/read-only PASS\n");

    pid = f3_fork(CLONE_VM);
    child = find_proc(pid);
    assert(child != NULL && child->mm == f3_parent && mm_count(f3_parent) == 2);
    f3_free_child(child);
    assert(mm_count(f3_parent) == 1);
    pid = f3_fork(0);
    child = find_proc(pid);
    struct proc_struct *older = child;
    int pid2 = f3_fork(0);
    child = find_proc(pid2);
    assert(current->cptr == child && child->optr == older && older->yptr == child);
    assert(f3_data(child->mm, F3_VA) == f3_data(f3_parent, F3_VA));
    assert(copy_to_user(child->mm, (void *)F3_VA, "Z", 1));
    assert(f3_data(child->mm, F3_VA)[0] == 'Z');
    assert(f3_data(f3_parent, F3_VA)[0] == 0x21);
    assert(f3_data(older->mm, F3_VA)[0] == 0x21);
    f3_free_child(child);
    f3_free_child(older);
    cprintf("[Lab5 F3] CLONE_VM, multiple forks and safe kernel writes PASS\n");

    struct Page *second = pgdir_alloc_page(f3_parent->pgdir, edge + 1,
                                          PTE_U | PTE_R | PTE_W);
    assert(second != NULL);
    *f3_data(f3_parent, edge) = 'A';
    *f3_data(f3_parent, edge + 1) = 'D';
    child = find_proc(f3_fork(0));
    assert(child != NULL);
    f3_fail_after = 1;
    assert(!copy_to_user(child->mm, (void *)edge, "BC", 2));
    f3_fail_after = -1;
    assert(*f3_data(child->mm, edge) == 'A' && *f3_data(child->mm, edge + 1) == 'D');
    assert(copy_to_user(child->mm, (void *)edge, "BC", 2));
    assert(*f3_data(child->mm, edge) == 'B' && *f3_data(child->mm, edge + 1) == 'C');
    assert(*f3_data(f3_parent, edge) == 'A' && *f3_data(f3_parent, edge + 1) == 'D');
    f3_free_child(child);
    page_remove(f3_parent->pgdir, edge + 1);
    cprintf("[COW demo] Cross-page COW allocation failure preserves data; retry succeeds PASS\n");

    int count = nr_process;
    before = nr_free_pages();
    f3_fail_pcb = 1;
    assert(f3_fork(0) == -E_NO_MEM);
    f3_fail_pcb = 0;
    for (int i = 0; i < 8; i++)
    {
        f3_fail_after = i;
        pid = f3_fork(0);
        f3_fail_after = -1;
        if (pid > 0) f3_free_child(find_proc(pid));
        else assert(pid == -E_NO_MEM);
        assert(nr_process == count && current->cptr == initial_child);
        assert(mm_count(f3_parent) == 1 && nr_free_pages() == before);
    }
    nr_process = MAX_PROCESS;
    assert(f3_fork(0) == -E_NO_FREE_PROC);
    nr_process = count;
    cprintf("[Lab5 F3] allocation rollback and process limit PASS\n");

    current->mm = NULL;
    current->pgdir = boot_pgdir_pa;
    f3_free_mm(f3_parent);
    assert(nr_free_pages() == baseline);
    cprintf("[Lab5 F3] page-table cleanup and baseline free-page count PASS\n");

    f3_parent = f3_new_mm();
    f3_map(f3_parent, F3_VA, PGSIZE, VM_READ | VM_WRITE);
    f3_data(f3_parent, F3_VA)[0] = 0x21;
    current->mm = f3_parent;
    current->pgdir = PADDR(f3_parent->pgdir);
    pid = f3_fork(0);
    f3_user = find_proc(pid);
    struct Page *code = pgdir_alloc_page(f3_user->mm->pgdir, UTEXT,
                                       PTE_U | PTE_R | PTE_X | PTE_A);
    assert(code != NULL);
    assert(mm_map(f3_user->mm, UTEXT, PGSIZE, VM_READ | VM_EXEC, NULL) == 0);
    uint32_t instructions[] = {
        0x008012b7, 0x07300313, 0x00628023, // Store into the COW page.
        (SYS_fork << 20) | 0x513, 0x00000073, // a0 = SYS_fork; ecall
        0x00000073, // Test completion trap, with the child PID still in a0.
    };
    memcpy(page2kva(code), instructions, sizeof(instructions));
    current = f3_user;
    memset(current->tf, 0, sizeof(*current->tf));
    current->tf->epc = UTEXT;
    current->tf->status = (read_csr(sstatus) | SSTATUS_SPIE) & ~(SSTATUS_SPP | SSTATUS_SIE);
    current->tf->gpr.sp = USTACKTOP;
    write_csr(satp, ((uintptr_t)8 << 60) | (current->pgdir >> RISCV_PGSHIFT));
    flush_tlb();
    forkrets(current->tf);
    panic("unexpected return from user test");
}

void f3_user_done(struct trapframe *tf)
{
    assert(tf->cause == CAUSE_USER_ECALL && !(tf->status & SSTATUS_SPP));
    assert(tf->epc == UTEXT + 20);
    assert(f3_data(current->mm, F3_VA)[0] == 0x73);
    assert(f3_data(f3_parent, F3_VA)[0] == 0x21);
    assert(f3_data(current->mm, F3_VA) != f3_data(f3_parent, F3_VA));
    cprintf("[Lab5 F3] real U-mode store fault/retry and parent isolation PASS\n");
    int child_pid = (int)tf->gpr.a0;
    struct proc_struct *child = find_proc(child_pid);
    assert(child != NULL && child->parent == current && child->tf->gpr.a0 == 0);
    assert(child->tf->epc == tf->epc && child->tf->gpr.sp == tf->gpr.sp);
    cprintf("[Lab5 F3] real user fork syscall and both return values PASS\n");

    int *status = (int *)(F3_VA + sizeof(int));
    assert(f3_data(current->mm, F3_VA) == f3_data(child->mm, F3_VA));
    assert(do_wait(child_pid, (int *)F3_RO) == -E_INVAL);
    assert(find_proc(child_pid) == child);
    int exit_code = 42;
    child->exit_code = exit_code;
    child->state = PROC_ZOMBIE;
    struct mm_struct *child_mm = child->mm;
    struct Page *before_wait = pte2page(*get_pte(current->mm->pgdir, F3_VA, 0));
    assert(page_ref(before_wait) == 2);
    assert(do_wait(child_pid, status) == 0 && find_proc(child_pid) == NULL);
    assert(*(int *)f3_data(current->mm, (uintptr_t)status) == exit_code);
    assert(*(int *)f3_data(child_mm, (uintptr_t)status) == 0);
    f3_free_mm(child_mm);
    cprintf("[Lab5 F3] do_wait status write preserves COW isolation PASS\n");
    cprintf("[Lab5 F3] ALL TESTS PASS\n");
    sbi_shutdown();
    while (1) {}
}
