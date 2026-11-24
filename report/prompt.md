## Lab5 Function 1

[PROMPT]

完成本人负责的 Lab5 Function 1，实现用户态系统调用。

主要完成：

- `exception_handler()` 中 `CAUSE_USER_ECALL` 的处理；
- `syscall()` 系统调用分发；
- `sys_exit()`、`sys_fork()`、`sys_wait()`、`sys_exec()`、`sys_yield()`、`sys_kill()`、`sys_getpid()`、`sys_putc()`、`sys_pgdir()`；
- 正确处理系统调用号、参数、返回值以及 `ecall` 后的 `epc`；
- 对涉及用户地址的参数进行必要合法性检查。

完成后根据真实实现和测试结果补充 `report.md`。

[RELY]

主要依据当前 `lab5` 工作区：

- `code/kern/trap/trap.c`
- `code/kern/syscall/syscall.c`
- `code/user/libs/syscall.c`
- `code/libs/unistd.h`
- `code/kern/process/proc.c`
- 当前 `TrapFrame`、`current->tf`、`user_mem_check()` 和各 `do_xxx()` 接口

系统调用整体接口关系：

```text
用户态 syscall
    ↓
ecall
    ↓
exception_handler()
    ↓
syscall()
    ↓
sys_xxx()
    ↓
do_xxx()
```

所有函数签名、寄存器约定和接口均以当前 Lab5 实际代码为准。

[GUARANTEE]

- 只实现 Function 1；
- 不提前实现 Function 2 / 3 / 4；
- Function 1 中只调用已有 `do_xxx()` 接口，不补全其底层实现；
- 不修改测试代码、评分脚本和 Makefile；
- 保留原实验注释；
- 不做无关重构和格式化；
- 不 commit、不 push；
- 其他 Function 未完成导致的错误不通过临时实现绕过；
- 未实际运行的测试不得填写为通过。

[SPECIFICATION]

最终完成：

1. `CAUSE_USER_ECALL` 处理及 `epc` 推进；
2. `syscall()` 中系统调用号、参数读取、合法性检查、分发和返回值写回；
3. Function 1 要求的全部 `sys_xxx()`；
4. 必要的用户地址检查；
5. 执行编译和 `git diff --check` 验证；
6. 区分 Function 1 自身问题与其他 Function 尚未完成造成的问题；
7. 根据真实实现和测试结果更新本人 `report.md`；
8. 记录 Function 1 的真实完成和测试状态。

## Lab5 Function 2

[PROMPT]

完成本人负责的 Lab5 Function 2，实现第一个用户进程的创建和加载。

主要完成：

- 更新 `alloc_proc()` 和 `kernel_thread()`；
- 完成 `init_main()`、`user_main()`；
- 完成 `kernel_execve()`；
- 完成 `do_execve()`、`load_icode()`；
- 完成 `do_wait()`；
- 正确建立用户地址空间、加载 ELF、建立用户栈并构造第一次进入用户态所需的 TrapFrame。

完成后根据真实实现和测试结果补充 `report.md`。

[RELY]

主要依据当前 `lab5` 工作区：

- `code/kern/process/proc.c`
- `code/kern/process/proc.h`
- `code/kern/mm/vmm.c`
- `code/kern/mm/vmm.h`
- `code/kern/mm/pmm.c`
- `code/kern/mm/pmm.h`
- `code/kern/trap/trap.h`
- `code/kern/trap/trapentry.S`
- `code/libs/elf.h`
- `code/libs/mmu.h`
- `code/user/`
- 当前 `proc_struct`、`mm_struct`、`vma_struct`、TrapFrame、ELF 和页表相关接口

整体启动关系：

```text
proc_init()
    ↓
init_main()
    ↓
user_main()
    ↓
kernel_execve()
    ↓
do_execve()
    ↓
load_icode()
    ↓
建立用户地址空间
    ↓
RESTORE_ALL
    ↓
sret
    ↓
进入 U Mode
```

所有函数签名、权限位、地址空间和 TrapFrame 设置均以当前 Lab5 实际代码为准。

[GUARANTEE]

- 只实现 Function 2；
- 不提前实现 Function 3 的 `do_fork()` 更新、`copy_mm()`、`dup_mmap()`、`copy_range()`；
- 不提前实现 Function 4 的 `do_exit()`、`do_yield()`、`do_kill()`；
- 不随意修改已经完成的 Function 1；
- 不修改测试代码、评分脚本和 Makefile；
- 保留原实验注释；
- 不做无关重构和格式化；
- 不 commit、不 push；
- 其他 Function 未完成造成的问题不通过临时实现绕过；
- 未实际运行的测试不得填写为通过。

[SPECIFICATION]

最终完成：

1. Lab5 版本 `alloc_proc()`、`kernel_thread()`；
2. `init_main()`、`user_main()`；
3. `kernel_execve()`、`do_execve()`；
4. `load_icode()` 中 ELF 解析、VMA、页表、代码段、数据段、BSS 和用户栈加载；
5. 第一次进入用户态所需 TrapFrame 的正确构造；
6. `do_wait()` 的等待和子进程回收逻辑；
7. 编译和 `git diff --check` 验证；
8. 区分 Function 2 自身问题与 Function 3 / 4 尚未完成造成的问题；
9. 根据真实实现和测试结果更新本人 `report.md`；
10. 记录 Function 2 的真实完成和测试状态。