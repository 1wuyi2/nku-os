#include <sbi.h>
#include "test_lab4.c"

static bool function2_fail_pcb;
static bool function2_fail_stack;

static void *function2_kmalloc(size_t size)
{
    return function2_fail_pcb ? NULL : kmalloc(size);
}

static struct Page *function2_alloc_stack(size_t n)
{
    return function2_fail_stack ? NULL : alloc_pages(n);
}

static int function2_test_entry(void *arg)
{
    return (int)(intptr_t)arg;
}

static void function2_release_child(struct proc_struct *proc)
{
    list_del(&proc->list_link);
    list_del(&proc->hash_link);
    nr_process--;
    put_kstack(proc);
    kfree(proc);
}

void cpu_idle(void)
{
    bool initial_intr;
    local_intr_save(initial_intr);
    test_lab4_process_create();
    assert(nr_process == 2 && idleproc->need_resched);
    assert(idleproc->pgdir == boot_pgdir_pa);
    assert(initproc->pgdir == boot_pgdir_pa);

    struct proc_struct *blank = alloc_proc();
    assert(blank != NULL && blank->state == PROC_UNINIT && blank->pid == -1);
    assert(blank->runs == 0 && blank->kstack == 0 && !blank->need_resched);
    assert(blank->parent == NULL && blank->mm == NULL && blank->tf == NULL);
    assert(blank->flags == 0 && blank->pgdir == boot_pgdir_pa);
    struct context zero_context;
    memset(&zero_context, 0, sizeof(zero_context));
    assert(memcmp(&blank->context, &zero_context, sizeof(zero_context)) == 0);
    for (int i = 0; i <= PROC_NAME_LEN; i++) assert(blank->name[i] == 0);
    assert(list_empty(&blank->list_link) && list_empty(&blank->hash_link));
    kfree(blank);
    cprintf("[Function 2] PCB initialization PASS\n");

    int pid1 = kernel_thread(function2_test_entry, (void *)0x1234, 0);
    int pid2 = kernel_thread(function2_test_entry, (void *)0x5678, 0);
    assert(pid1 > 1 && pid2 > 1 && pid1 != pid2 && nr_process == 4);
    struct proc_struct *child = find_proc(pid1);
    struct proc_struct *other = find_proc(pid2);
    assert(child != NULL && other != NULL && current == idleproc);
    assert(child->state == PROC_RUNNABLE && child->parent == idleproc);
    assert(child->mm == NULL && child->pgdir == boot_pgdir_pa);
    assert(child->kstack != other->kstack && child->kstack != idleproc->kstack);
    assert(child->tf == (struct trapframe *)(child->kstack + KSTACKSIZE) - 1);
    assert(child->tf->gpr.sp == (uintptr_t)child->tf && child->tf->gpr.a0 == 0);
    assert(child->tf->gpr.s0 == (uintptr_t)function2_test_entry);
    assert(child->tf->gpr.s1 == 0x1234 && other->tf->gpr.s1 == 0x5678);
    assert(child->tf->epc == (uintptr_t)kernel_thread_entry);
    assert((child->tf->status & (SSTATUS_SPP | SSTATUS_SPIE)) ==
           (SSTATUS_SPP | SSTATUS_SPIE));
    assert(!(child->tf->status & SSTATUS_SIE));
    assert(child->context.ra == (uintptr_t)forkret);
    assert(child->context.sp == (uintptr_t)child->tf);
    int listed = 0;
    list_entry_t *le = &proc_list;
    while ((le = list_next(le)) != &proc_list) listed++;
    assert(listed == nr_process - 1);
    function2_release_child(child);
    function2_release_child(other);
    assert(find_proc(pid1) == NULL && find_proc(pid2) == NULL);
    cprintf("[Function 2] PID/list/hash and kernel trapframe PASS\n");

    struct trapframe tf;
    memset(&tf, 0, sizeof(tf));
    tf.gpr.a0 = 123;
    tf.epc = 0x12345678;
    int pid = do_fork(0, 0x12345000, &tf);
    child = find_proc(pid);
    assert(child != NULL && child->tf->gpr.sp == 0x12345000);
    assert(child->tf->gpr.a0 == 0 && tf.gpr.a0 == 123);
    assert(child->tf->epc == tf.epc && !(read_csr(sstatus) & SSTATUS_SIE));
    function2_release_child(child);
    intr_enable();
    pid = kernel_thread(function2_test_entry, NULL, 0);
    assert(pid > 0 && (read_csr(sstatus) & SSTATUS_SIE));
    intr_disable();
    function2_release_child(find_proc(pid));
    cprintf("[Function 2] explicit stack and interrupt restoration PASS\n");

    size_t free_before = nr_free_pages();
    int count_before = nr_process;
    function2_fail_pcb = 1;
    assert(do_fork(0, 0, &tf) == -E_NO_MEM);
    function2_fail_pcb = 0;
    assert(nr_process == count_before && nr_free_pages() == free_before);
    function2_fail_stack = 1;
    assert(do_fork(0, 0, &tf) == -E_NO_MEM);
    function2_fail_stack = 0;
    assert(nr_process == count_before && nr_free_pages() == free_before);
    struct mm_struct dummy_mm;
    memset(&dummy_mm, 0, sizeof(dummy_mm));
    current->mm = &dummy_mm;
    assert(do_fork(0, 0, &tf) == -E_INVAL);
    current->mm = NULL;
    assert(nr_process == count_before && nr_free_pages() == free_before);
    nr_process = MAX_PROCESS;
    assert(do_fork(0, 0, &tf) == -E_NO_FREE_PROC);
    nr_process = count_before;
    assert(do_fork(0, 0, NULL) == -E_INVAL);
    assert(nr_process == 2 && nr_free_pages() == free_before);
    test_lab4_process_create();
    cprintf("[Function 2] allocation failures/rollback/process limit PASS\n");
    cprintf("[Function 2] ALL TESTS PASS (creation only; scheduling excluded)\n");
    sbi_shutdown();
    while (1) {}
}
