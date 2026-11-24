#include <proc.h>
#include <kmalloc.h>
#include <string.h>
#include <sync.h>
#include <pmm.h>
#include <error.h>
#include <sched.h>
#include <elf.h>
#include <vmm.h>
#include <trap.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

/* ------------- process/thread mechanism design&implementation -------------
(an simplified Linux process/thread mechanism )
introduction:
  ucore implements a simple process/thread mechanism. process contains the independent memory sapce, at least one threads
for execution, the kernel data(for management), processor state (for context switch), files(in lab6), etc. ucore needs to
manage all these details efficiently. In ucore, a thread is just a special kind of process(share process's memory).
------------------------------
process state       :     meaning               -- reason
    PROC_UNINIT     :   uninitialized           -- alloc_proc
    PROC_SLEEPING   :   sleeping                -- try_free_pages, do_wait, do_sleep
    PROC_RUNNABLE   :   runnable(maybe running) -- proc_init, wakeup_proc,
    PROC_ZOMBIE     :   almost dead             -- do_exit

-----------------------------
process state changing:

  alloc_proc                                 RUNNING
      +                                   +--<----<--+
      +                                   + proc_run +
      V                                   +-->---->--+
PROC_UNINIT -- proc_init/wakeup_proc --> PROC_RUNNABLE -- try_free_pages/do_wait/do_sleep --> PROC_SLEEPING --
                                           A      +                                                           +
                                           |      +--- do_exit --> PROC_ZOMBIE                                +
                                           +                                                                  +
                                           -----------------------wakeup_proc----------------------------------
-----------------------------
process relations
parent:           proc->parent  (proc is children)
children:         proc->cptr    (proc is parent)
older sibling:    proc->optr    (proc is younger sibling)
younger sibling:  proc->yptr    (proc is older sibling)
-----------------------------
related syscall for process:
SYS_exit        : process exit,                           -->do_exit
SYS_fork        : create child process, dup mm            -->do_fork-->wakeup_proc
SYS_wait        : wait process                            -->do_wait
SYS_exec        : after fork, process execute a program   -->load a program and refresh the mm
SYS_clone       : create child thread                     -->do_fork-->wakeup_proc
SYS_yield       : process flag itself need resecheduling, -- proc->need_sched=1, then scheduler will rescheule this process
SYS_sleep       : process sleep                           -->do_sleep
SYS_kill        : kill process                            -->do_kill-->proc->flags |= PF_EXITING
                                                                 -->wakeup_proc-->do_wait-->do_exit
SYS_getpid      : get the process's pid

*/

// the process set's list
list_entry_t proc_list;

#define HASH_SHIFT 10
#define HASH_LIST_SIZE (1 << HASH_SHIFT)
#define pid_hashfn(x) (hash32(x, HASH_SHIFT))

// has list for process set based on pid
static list_entry_t hash_list[HASH_LIST_SIZE];

// idle proc
struct proc_struct *idleproc = NULL;
// init proc
struct proc_struct *initproc = NULL;
// current proc
struct proc_struct *current = NULL;

static int nr_process = 0;

void kernel_thread_entry(void);
void forkrets(struct trapframe *tf);
void switch_to(struct context *from, struct context *to);
static int init_main(void *arg);
static int user_main(void *arg);
static int kernel_execve(const char *name, unsigned char *binary, size_t size);
static int load_icode(unsigned char *binary, size_t size);

//lab4 请补充你在lab4中的代码

//lab5 function2 YOUR CODE update alloc_proc
// 2412771-谭锦睿
static struct proc_struct *
alloc_proc(void)
{
    struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
    if (proc != NULL)
    {
        memset(proc, 0, sizeof(struct proc_struct));
        proc->state = PROC_UNINIT;
        proc->pid = -1;
        proc->runs = 0;
        proc->kstack = 0;
        proc->need_resched = 0;
        proc->parent = NULL;
        proc->mm = NULL;
        proc->tf = NULL;
        proc->pgdir = boot_pgdir_pa;
        proc->flags = 0;
        proc->exit_code = 0;
        proc->wait_state = 0;
        proc->cptr = proc->yptr = proc->optr = NULL;
        list_init(&proc->list_link);
        list_init(&proc->hash_link);
    }
    return proc;
}


// set_proc_name - set the name of proc
char *
set_proc_name(struct proc_struct *proc, const char *name)
{
    memset(proc->name, 0, sizeof(proc->name));
    return memcpy(proc->name, name, PROC_NAME_LEN);
}

// get_proc_name - get the name of proc
char *
get_proc_name(struct proc_struct *proc)
{
    static char name[PROC_NAME_LEN + 1];
    memset(name, 0, sizeof(name));
    return memcpy(name, proc->name, PROC_NAME_LEN);
}

// set_links - set the relation links of process
static void
set_links(struct proc_struct *proc)
{
    list_add(&proc_list, &(proc->list_link));
    proc->yptr = NULL;
    if ((proc->optr = proc->parent->cptr) != NULL)
    {
        proc->optr->yptr = proc;
    }
    proc->parent->cptr = proc;
    nr_process++;
}

// remove_links - clean the relation links of process
static void
remove_links(struct proc_struct *proc)
{
    list_del(&(proc->list_link));
    if (proc->optr != NULL)
    {
        proc->optr->yptr = proc->yptr;
    }
    if (proc->yptr != NULL)
    {
        proc->yptr->optr = proc->optr;
    }
    else
    {
        proc->parent->cptr = proc->optr;
    }
    nr_process--;
}

// get_pid - alloc a unique pid for process
static int
get_pid(void)
{
    static_assert(MAX_PID > MAX_PROCESS);
    struct proc_struct *proc;
    list_entry_t *list = &proc_list, *le;
    static int next_safe = MAX_PID, last_pid = MAX_PID;
    if (++last_pid >= MAX_PID)
    {
        last_pid = 1;
        goto inside;
    }
    if (last_pid >= next_safe)
    {
    inside:
        next_safe = MAX_PID;
    repeat:
        le = list;
        while ((le = list_next(le)) != list)
        {
            proc = le2proc(le, list_link);
            if (proc->pid == last_pid)
            {
                if (++last_pid >= next_safe)
                {
                    if (last_pid >= MAX_PID)
                    {
                        last_pid = 1;
                    }
                    next_safe = MAX_PID;
                    goto repeat;
                }
            }
            else if (proc->pid > last_pid && next_safe > proc->pid)
            {
                next_safe = proc->pid;
            }
        }
    }
    return last_pid;
}






// forkret -- the first kernel entry point of a new thread/process
// NOTE: the addr of forkret is setted in copy_thread function
//       after switch_to, the current proc will execute here.
static void
forkret(void)
{
    forkrets(current->tf);
}

// hash_proc - add proc into proc hash_list
static void
hash_proc(struct proc_struct *proc)
{
    list_add(hash_list + pid_hashfn(proc->pid), &(proc->hash_link));
}

// unhash_proc - delete proc from proc hash_list
static void
unhash_proc(struct proc_struct *proc)
{
    list_del(&(proc->hash_link));
}

// find_proc - find proc frome proc hash_list according to pid
struct proc_struct *
find_proc(int pid)
{
    if (0 < pid && pid < MAX_PID)
    {
        list_entry_t *list = hash_list + pid_hashfn(pid), *le = list;
        while ((le = list_next(le)) != list)
        {
            struct proc_struct *proc = le2proc(le, hash_link);
            if (proc->pid == pid)
            {
                return proc;
            }
        }
    }
    return NULL;
}

//lab5 function2 YOUR CODE update kernel_thread
// 2412771-谭锦睿
int
kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags)
{
    struct trapframe tf;
    memset(&tf, 0, sizeof(tf));
    tf.gpr.s0 = (uintptr_t)fn;
    tf.gpr.s1 = (uintptr_t)arg;
    tf.status = (read_csr(sstatus) | SSTATUS_SPP | SSTATUS_SPIE) & ~SSTATUS_SIE;
    tf.epc = (uintptr_t)kernel_thread_entry;
    return do_fork(clone_flags | CLONE_VM, 0, &tf);
}


// setup_kstack - alloc pages with size KSTACKPAGE as process kernel stack
static int
setup_kstack(struct proc_struct *proc)
{
    struct Page *page = alloc_pages(KSTACKPAGE);
    if (page != NULL)
    {
        proc->kstack = (uintptr_t)page2kva(page);
        return 0;
    }
    return -E_NO_MEM;
}

// put_kstack - free the memory space of process kernel stack
static void
put_kstack(struct proc_struct *proc)
{
    free_pages(kva2page((void *)(proc->kstack)), KSTACKPAGE);
}

// setup_pgdir - alloc one page as PDT
static int
setup_pgdir(struct mm_struct *mm)
{
    struct Page *page;
    if ((page = alloc_page()) == NULL)
    {
        return -E_NO_MEM;
    }
    pde_t *pgdir = page2kva(page);
    memcpy(pgdir, boot_pgdir_va, PGSIZE);

    mm->pgdir = pgdir;
    return 0;
}

// put_pgdir - free the memory space of PDT
static void
put_pgdir(struct mm_struct *mm)
{
    free_page(kva2page(mm->pgdir));
}

//lab5 function3 2413581 update copy_mm
static int
copy_mm(uint32_t clone_flags, struct proc_struct *proc)
{
    struct mm_struct *oldmm = current->mm, *mm = NULL;
    int ret;
    if (oldmm == NULL)
    {
        proc->mm = NULL;
        return 0;
    }
    if (clone_flags & CLONE_VM)
    {
        mm = oldmm;
    }
    else
    {
        if ((mm = mm_create()) == NULL)
            return -E_NO_MEM;
        if ((ret = setup_pgdir(mm)) != 0)
            goto bad_mm;
        lock_mm(oldmm);
        ret = dup_mmap(mm, oldmm);
        unlock_mm(oldmm);
        if (ret != 0)
        {
            exit_mmap(mm);
            put_pgdir(mm);
            goto bad_mm;
        }
    }
    mm_count_inc(mm);
    proc->mm = mm;
    proc->pgdir = PADDR(mm->pgdir);
    return 0;
bad_mm:
    mm_destroy(mm);
    return ret;
}



//lab5 function2 YOUR CODE
static void
copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf)
{
    proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1;
    *proc->tf = *tf;
    proc->tf->gpr.a0 = 0;
    proc->tf->gpr.sp = esp != 0 ? esp : (uintptr_t)proc->tf;
    proc->context.ra = (uintptr_t)forkret;
    proc->context.sp = (uintptr_t)proc->tf;
}


//lab5 function3 2413581
int
do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf)
{
    int ret = -E_NO_FREE_PROC;
    struct proc_struct *proc;
    bool intr_flag;

    if (nr_process >= MAX_PROCESS)
    {
        return ret;
    }
    if (current == NULL || tf == NULL)
    {
        return -E_INVAL;
    }
    ret = -E_NO_MEM;
    if ((proc = alloc_proc()) == NULL)
    {
        return ret;
    }
    proc->parent = current;
    if ((ret = setup_kstack(proc)) != 0)
    {
        goto bad_fork_cleanup_proc;
    }
    if ((ret = copy_mm(clone_flags, proc)) != 0)
    {
        goto bad_fork_cleanup_kstack;
    }
    if (stack == 0 && !trap_in_kernel(tf))
    {
        stack = tf->gpr.sp;
    }
    copy_thread(proc, stack, tf);

    local_intr_save(intr_flag);
    proc->pid = get_pid();
    hash_proc(proc);
    set_links(proc);
    wakeup_proc(proc);
    local_intr_restore(intr_flag);
    return proc->pid;

bad_fork_cleanup_kstack:
    put_kstack(proc);
bad_fork_cleanup_proc:
    kfree(proc);
    return ret;
}


//lab5 function4 YOUR CODE


#define __KERNEL_EXECVE(name, binary, size) ({           \
    cprintf("kernel_execve: pid = %d, name = \"%s\".\n", \
            current->pid, name);                         \
    kernel_execve(name, binary, (size_t)(size));         \
})

#define KERNEL_EXECVE(x) ({                                    \
    extern unsigned char _binary_obj___user_##x##_out_start[], \
        _binary_obj___user_##x##_out_size[];                   \
    __KERNEL_EXECVE(#x, _binary_obj___user_##x##_out_start,    \
                    _binary_obj___user_##x##_out_size);        \
})

#define __KERNEL_EXECVE2(x, xstart, xsize) ({   \
    extern unsigned char xstart[], xsize[];     \
    __KERNEL_EXECVE(#x, xstart, (size_t)xsize); \
})

#define KERNEL_EXECVE2(x, xstart, xsize) __KERNEL_EXECVE2(x, xstart, xsize)


// 2412771-谭锦睿: Lab5 Function 2
static int
kernel_execve(const char *name, unsigned char *binary, size_t size)
{
    int ret = do_execve(name, strlen(name), binary, size);
    if (ret == 0)
    {
        forkrets(current->tf);
    }
    return ret;
}

static int
user_main(void *arg)
{
#ifdef TEST
    return KERNEL_EXECVE2(TEST, TESTSTART, TESTSIZE);
#else
    return KERNEL_EXECVE(hello);
#endif
}

static int
init_main(void *arg)
{
    int pid = kernel_thread(user_main, NULL, 0);
    if (pid <= 0)
    {
        panic("create user_main failed.\n");
    }

    while (do_wait(0, NULL) == 0)
    {
        /* Reap every user process adopted by initproc. */
    }
    cprintf("all user-mode processes have quit.\n");
    cprintf("init check memory pass.\n");
    return 0;
}

int
do_execve(const char *name, size_t len, unsigned char *binary, size_t size)
{
    struct mm_struct *mm = current->mm;
    unsigned char *binary_copy = NULL;
    char local_name[PROC_NAME_LEN + 1];
    int ret;

    if (len > PROC_NAME_LEN)
    {
        len = PROC_NAME_LEN;
    }
    if (len == 0 || size == 0 ||
        !user_mem_check(mm, (uintptr_t)name, len, 0) ||
        !user_mem_check(mm, (uintptr_t)binary, size, 0))
    {
        return -E_INVAL;
    }

    memset(local_name, 0, sizeof(local_name));
    if (!copy_from_user(mm, local_name, name, len, 0))
    {
        return -E_INVAL;
    }

    if (mm != NULL)
    {
        if ((binary_copy = kmalloc(size)) == NULL)
        {
            return -E_NO_MEM;
        }
        if (!copy_from_user(mm, binary_copy, binary, size, 0))
        {
            kfree(binary_copy);
            return -E_INVAL;
        }
        binary = binary_copy;

        write_csr(satp, ((uintptr_t)8 << 60) | (boot_pgdir_pa >> RISCV_PGSHIFT));
        flush_tlb();
        current->mm = NULL;
        current->pgdir = boot_pgdir_pa;
        if (mm_count_dec(mm) == 0)
        {
            exit_mmap(mm);
            put_pgdir(mm);
            mm_destroy(mm);
        }
    }

    ret = load_icode(binary, size);
    if (binary_copy != NULL)
    {
        kfree(binary_copy);
    }
    if (ret != 0)
    {
        do_exit(ret);
        return ret;
    }
    set_proc_name(current, local_name);
    return 0;
}

static int
load_icode(unsigned char *binary, size_t size)
{
    struct mm_struct *mm;
    struct elfhdr *elf;
    struct proghdr *ph;
    struct trapframe *tf = current->tf;
    int ret = -E_NO_MEM;

    if ((mm = mm_create()) == NULL)
    {
        return ret;
    }
    if ((ret = setup_pgdir(mm)) != 0)
    {
        goto bad_mm;
    }

    ret = -E_INVAL_ELF;
    if (size < sizeof(struct elfhdr))
    {
        goto bad_cleanup_mmap;
    }
    elf = (struct elfhdr *)binary;
    if (elf->e_magic != ELF_MAGIC ||
        elf->e_phentsize != sizeof(struct proghdr) ||
        elf->e_phoff > size ||
        elf->e_phnum > (size - elf->e_phoff) / sizeof(struct proghdr) ||
        elf->e_entry < USERBASE || elf->e_entry >= USERTOP)
    {
        goto bad_cleanup_mmap;
    }

    ph = (struct proghdr *)(binary + elf->e_phoff);
    for (int i = 0; i < elf->e_phnum; i++, ph++)
    {
        if (ph->p_type != ELF_PT_LOAD)
        {
            continue;
        }
        if (ph->p_filesz > ph->p_memsz ||
            ph->p_offset > size || ph->p_filesz > size - ph->p_offset ||
            ph->p_va < USERBASE || ph->p_va >= USERTOP ||
            ph->p_memsz > USERTOP - ph->p_va)
        {
            goto bad_cleanup_mmap;
        }
        if (ph->p_memsz == 0)
        {
            continue;
        }

        uint32_t vm_flags = 0, perm = PTE_U;
        if (ph->p_flags & ELF_PF_R)
        {
            vm_flags |= VM_READ;
            perm |= PTE_R;
        }
        if (ph->p_flags & ELF_PF_W)
        {
            vm_flags |= VM_WRITE;
            perm |= PTE_W;
        }
        if (ph->p_flags & ELF_PF_X)
        {
            vm_flags |= VM_EXEC;
            perm |= PTE_X;
        }

        if ((ret = mm_map(mm, (uintptr_t)ph->p_va, (size_t)ph->p_memsz,
                          vm_flags, NULL)) != 0)
        {
            goto bad_cleanup_mmap;
        }

        uintptr_t seg_start = (uintptr_t)ph->p_va;
        uintptr_t seg_end = seg_start + (uintptr_t)ph->p_memsz;
        uintptr_t file_end = seg_start + (uintptr_t)ph->p_filesz;
        uintptr_t la = ROUNDDOWN(seg_start, PGSIZE);

        while (la < seg_end)
        {
            struct Page *page = pgdir_alloc_page(mm->pgdir, la, perm);
            if (page == NULL)
            {
                ret = -E_NO_MEM;
                goto bad_cleanup_mmap;
            }
            unsigned char *page_kva = page2kva(page);
            memset(page_kva, 0, PGSIZE);

            uintptr_t copy_start = la > seg_start ? la : seg_start;
            uintptr_t page_end = la + PGSIZE;
            uintptr_t copy_end = page_end < file_end ? page_end : file_end;
            if (copy_start < copy_end)
            {
                memcpy(page_kva + (copy_start - la),
                       binary + ph->p_offset + (copy_start - seg_start),
                       copy_end - copy_start);
            }
            la += PGSIZE;
        }
    }

    struct vma_struct *entry_vma = find_vma(mm, (uintptr_t)elf->e_entry);
    if (entry_vma == NULL || !(entry_vma->vm_flags & VM_EXEC))
    {
        ret = -E_INVAL_ELF;
        goto bad_cleanup_mmap;
    }

    if ((ret = mm_map(mm, USTACKTOP - USTACKSIZE, USTACKSIZE,
                      VM_READ | VM_WRITE | VM_STACK, NULL)) != 0)
    {
        goto bad_cleanup_mmap;
    }
    for (int i = 1; i <= 4; i++)
    {
        struct Page *page = pgdir_alloc_page(mm->pgdir, USTACKTOP - i * PGSIZE,
                                             PTE_U | PTE_R | PTE_W);
        if (page == NULL)
        {
            ret = -E_NO_MEM;
            goto bad_cleanup_mmap;
        }
        memset(page2kva(page), 0, PGSIZE);
    }

    mm_count_inc(mm);
    current->mm = mm;
    current->pgdir = PADDR(mm->pgdir);
    write_csr(satp, ((uintptr_t)8 << 60) | (current->pgdir >> RISCV_PGSHIFT));
    flush_tlb();

    memset(tf, 0, sizeof(struct trapframe));
    tf->gpr.sp = USTACKTOP;
    tf->epc = (uintptr_t)elf->e_entry;
    tf->status = (read_csr(sstatus) | SSTATUS_SPIE) &
                 ~(SSTATUS_SPP | SSTATUS_SIE);
    return 0;

bad_cleanup_mmap:
    exit_mmap(mm);
    put_pgdir(mm);
bad_mm:
    mm_destroy(mm);
    return ret;
}

int
do_wait(int pid, int *code_store)
{
    struct proc_struct *proc;
    bool intr_flag, haskid;

    if (code_store != NULL &&
        !user_mem_check(current->mm, (uintptr_t)code_store, sizeof(int), 1))
    {
        return -E_INVAL;
    }

repeat:
    haskid = 0;
    if (pid != 0)
    {
        proc = find_proc(pid);
        if (proc != NULL && proc->parent == current)
        {
            haskid = 1;
            if (proc->state == PROC_ZOMBIE)
            {
                goto found;
            }
        }
    }
    else
    {
        proc = current->cptr;
        for (; proc != NULL; proc = proc->optr)
        {
            haskid = 1;
            if (proc->state == PROC_ZOMBIE)
            {
                goto found;
            }
        }
    }

    if (haskid)
    {
        current->state = PROC_SLEEPING;
        current->wait_state = WT_CHILD;
        schedule();
        if (current->flags & PF_EXITING)
        {
            return do_exit(-E_KILLED);
        }
        goto repeat;
    }
    return -E_BAD_PROC;

found:
    if (proc == idleproc || proc == initproc)
    {
        panic("wait idleproc or initproc.\n");
    }
    if (code_store != NULL)
    {
        if (!copy_to_user(current->mm, code_store, &proc->exit_code, sizeof(int)))
        {
            return -E_INVAL;
        }
    }

    local_intr_save(intr_flag);
    unhash_proc(proc);
    remove_links(proc);
    local_intr_restore(intr_flag);

    put_kstack(proc);
    kfree(proc);
    return 0;
}


//lab5 function2 YOUR CODE
void
proc_run(struct proc_struct *proc)
{
    if (proc != current)
    {
        bool intr_flag;
        local_intr_save(intr_flag);
        struct proc_struct *prev = current;
        current = proc;
        write_csr(satp, ((uintptr_t)8 << 60) | (proc->pgdir >> RISCV_PGSHIFT));
        flush_tlb();
        switch_to(&prev->context, &proc->context);
        local_intr_restore(intr_flag);
    }
}

void
proc_init(void)
{
    list_init(&proc_list);
    for (int i = 0; i < HASH_LIST_SIZE; i++)
    {
        list_init(&hash_list[i]);
    }
    if ((idleproc = alloc_proc()) == NULL)
    {
        panic("cannot allocate idleproc.\n");
    }
    idleproc->pid = 0;
    idleproc->state = PROC_RUNNABLE;
    idleproc->kstack = (uintptr_t)bootstack;
    idleproc->need_resched = 1;
    set_proc_name(idleproc, "idle");
    nr_process++;
    current = idleproc;

    int pid = kernel_thread(init_main, NULL, 0);
    if (pid <= 0 || (initproc = find_proc(pid)) == NULL)
    {
        panic("create initproc failed.\n");
    }
    set_proc_name(initproc, "init");
}

void
cpu_idle(void)
{
    while (1)
    {
        if (current->need_resched)
        {
            schedule();
        }
    }
}



