# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
| --- | --- |
| 实验名称 | Lab5：用户程序 |
| 小组成员 | 2412771-谭锦睿、2413581-黄奕浩、2411302-王乐之 |
| 完成日期 | 待完成后填写 |
| 实验分支 | `lab5` |
| 代码目录 | `code/` |
| 报告目录 | `report/` |

---

## 小组分工

### 练习分工

| 成员 | 负责内容 |
| --- | --- |
| 2412771-谭锦睿 | 功能模块一：用户态系统调用；功能模块二：创建并加载第一个用户进程。 |
| 2413581-黄奕浩 | 功能模块三：复制用户进程。 |
| 2411302-王乐之 | 功能模块四：进程退出、让出与终止相关功能。 |
| 三人共同完成 | 最终代码整合、编译运行、官方测试以及实验报告整理。 |

### 拓展任务分工

| 成员 | 负责内容 |
| --- | --- |
| 2412771-谭锦睿 | 用户程序预加载与普通操作系统程序加载方式分析。 |
| 2413581-黄奕浩 | Copy on Write / Dirty COW。 |
| 2411302-王乐之 | 使用 GDB 跟踪用户态系统调用进入与返回过程。 |

### 任务完成情况

- [x] 建立 Lab5 分支并导入实验代码
- [x] 补全 Lab2 / Lab3 / Lab4 前序依赖
- [x] 修改 `RESTORE_ALL`，支持用户态 / 内核态双向异常返回
- [x] 功能模块一：用户态系统调用
- [x] 功能模块二：创建并加载第一个用户进程
- [ ] 功能模块三：复制用户进程
- [ ] 功能模块四：进程退出、让出与终止
- [x] 用户程序预加载方式分析
- [ ] Copy on Write / Dirty COW
- [ ] GDB 系统调用调试
- [ ] 完成实验代码整合
- [ ] 完成编译验证
- [ ] 完成 QEMU 运行验证
- [ ] 完成官方测试
- [ ] 完成实验报告
- [ ] 整理实际使用的 Prompt

### 报告分工

| 成员 | 报告负责内容 |
| --- | --- |
| 2412771-谭锦睿 | 用户态系统调用、第一个用户进程创建与加载、用户程序预加载方式分析 |
| 2413581-黄奕浩 | 用户进程复制、Copy on Write / Dirty COW |
| 2411302-王乐之 | 进程退出、让出与终止、GDB 系统调用调试 |
| 三人共同完成 | 实验整体逻辑、测试记录、实验结果和最终报告整理 |

实际使用的 Prompt 统一整理在：

```text
report/prompt.md
```

---

# 一、实验目的

Lab5 在前面物理内存管理、虚拟内存、Trap 机制以及内核线程管理的基础上进一步加入用户进程，使用户程序能够运行在用户态，并通过系统调用请求内核提供服务。

主要目标如下：

1. 理解用户态与内核态之间的区别及切换过程；
2. 理解用户程序通过 `ecall` 发起系统调用的过程；
3. 理解系统调用号、参数和返回值的传递方式；
4. 理解系统调用的异常入口和分发机制；
5. 理解第一个用户进程的创建过程；
6. 理解 ELF 用户程序加载到用户地址空间的过程；
7. 理解 `mm_struct`、`vma_struct` 与页表之间的关系；
8. 理解用户程序第一次从内核态进入用户态的过程；
9. 理解 `fork()` 创建用户子进程的基本过程；
10. 理解进程退出、等待、让出 CPU 和终止机制；
11. 完成 Lab5 要求的四个功能模块，并进行编译、运行和测试验证。

---

# 二、实验环境

## 2.1 系统与开发环境

| 项目 | 环境 |
| --- | --- |
| 主机环境 | Windows + WSL2 Ubuntu 24.04 |
| 目标架构 | RISC-V 64 |
| 交叉编译工具链 | `riscv64-unknown-elf-*` |
| 虚拟机 | QEMU |
| 固件 | OpenSBI |
| 调试工具 | RISC-V GDB |
| 版本管理 | Git |
| 实验分支 | `lab5` |

## 2.2 AI 工具

| 成员 | AI 工具 | 用途 |
| --- | --- | --- |
| 2412771-谭锦睿 | ChatGPT / Codex | 系统调用、用户进程创建与程序加载的理解、实现和检查辅助 |
| 2413581-黄奕浩 | ChatGPT / Codex | 用户进程复制及拓展任务实现辅助 |
| 2411302-王乐之 | ChatGPT / Codex | 进程控制及 GDB 调试辅助 |

完整 Prompt 见：

```text
report/prompt.md
```

---

# 三、实验整体逻辑分析

## 3.1 Lab5 主要内容

Lab5 主要包含四个功能模块：

```text
功能模块一：用户态系统调用
    ↓
exception_handler()
syscall()
sys_exit()
sys_fork()
sys_wait()
sys_exec()
sys_yield()
sys_kill()
sys_getpid()
sys_putc()
sys_pgdir()

功能模块二：创建并加载第一个用户进程
    ↓
alloc_proc()
init_main()
user_main()
kernel_thread()
kernel_execve()
do_execve()
load_icode()
do_wait()

功能模块三：复制用户进程
    ↓
do_fork()
copy_mm()
dup_mmap()
copy_range()

功能模块四：进程控制
    ↓
do_exit()
do_yield()
do_kill()
```

四个功能之间的关系可以概括为：

```text
创建并加载用户程序
        ↓
进入用户态
        ↓
用户程序发起系统调用
        ↓
进入内核
        ↓
fork 创建新的用户进程
        ↓
继续执行和调度
        ↓
exit / wait / yield / kill
```

---

## 3.2 与前序实验的联系

Lab5 继续使用前面实验已经实现的功能：

```text
Lab2
→ 物理内存管理、Page、PTE、页表

Lab3
→ Trap、TrapFrame、SAVE_ALL、RESTORE_ALL、sret

Lab4
→ proc_struct、内核线程、调度、上下文切换
```

Lab5 starter 中需要补回前序实验已经完成的相关代码。

其中需要特别修改的是 Lab3 中的：

```text
RESTORE_ALL
```

Lab3 中该宏只需要支持纯内核态环境，而 Lab5 开始运行用户程序，因此需要扩展为同时支持：

```text
用户态 Trap
    ↓
返回用户态

内核态 Trap
    ↓
返回内核态
```

需要结合：

```text
sstatus
SSTATUS_SPP
sscratch
sp
```

判断返回的特权级并正确恢复用户栈或内核栈。

其余前序实验代码主要作为 Lab5 的已有基础使用。

---

## 3.3 Lab5 总体执行流程

整体执行过程可以概括为：

```text
kern_init()
    ↓
proc_init()
    ↓
创建 idleproc
    ↓
创建 initproc
    ↓
init_main()
    ↓
创建 user_main
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
加载用户程序
    ↓
构造 TrapFrame
    ↓
sret
    ↓
进入用户态
    ↓
用户程序执行
    ↓
ecall
    ↓
Trap
    ↓
exception_handler()
    ↓
syscall()
    ↓
具体系统调用
    ↓
返回用户态
```

---

# 四、实验内容与实现

## 4.1 功能模块一：用户态系统调用

**负责人：2412771-谭锦睿**

### 4.1.1 任务要求

本功能负责建立用户态程序进入内核的系统调用通路。

按照实验指导书，需要更新和完善：

```text
exception_handler()

syscall()

sys_exit()
sys_fork()
sys_wait()
sys_exec()
sys_yield()
sys_kill()
sys_getpid()
sys_putc()
sys_pgdir()
```

主要文件：

```text
kern/trap/trap.c
kern/syscall/syscall.c
```

同时需要参考：

```text
libs/unistd.h
user/libs/syscall.c
```

用户态系统调用整体过程为：

```text
用户程序
    ↓
用户态系统调用封装
    ↓
准备系统调用号和参数
    ↓
ecall
    ↓
CAUSE_USER_ECALL
    ↓
exception_handler()
    ↓
syscall()
    ↓
sys_xxx()
    ↓
do_xxx()
    ↓
返回结果
    ↓
异常返回
    ↓
用户程序继续执行
```

其中主要使用：

```text
a0
→ 系统调用号

a1 ~ a5
→ 系统调用参数
```

在处理用户态 `ecall` 时，还需要正确修改 `epc`，使异常返回以后从 `ecall` 的下一条指令继续执行。

---

### 4.1.2 主要涉及知识

- U Mode；
- S Mode；
- `ecall`；
- `CAUSE_USER_ECALL`；
- Trap；
- `TrapFrame`；
- `epc` / `sepc`；
- 系统调用号；
- 寄存器传参；
- 系统调用分发；
- `current`；
- `current->tf`；
- 用户地址检查；
- `sret`。

---

### 4.1.3 实现过程

实现前先核对了当前工程的 `trapframe` 布局、用户库系统调用封装和 `do_xxx()` 声明。`user/libs/syscall.c` 使用 `a0` 传递系统调用号，使用 `a1` 至 `a5` 传递最多五个参数，并从 `a0` 取回返回值。

`exception_handler()` 遇到 `CAUSE_USER_ECALL` 时，先将 `epc` 增加 4，再调用 `syscall()`。RISC-V 的 `ecall` 为 4 字节指令，因此返回用户态后会从其下一条指令继续执行。非法指令和断点原有的变长指令推进逻辑保持不变。

`syscall()` 从 `current->tf` 取得 TrapFrame，读取 `a0`，并将 `a1` 至 `a5` 保存为参数数组。只有系统调用号在分发表范围内且对应函数指针非空时才调用 `sys_xxx()`；否则按当前 Lab5 测试约定返回 `-E_INVAL`。最终结果写回 `tf->gpr.a0`。

各包装函数的实际参数传递为：

- `sys_exit(error_code)` 调用 `do_exit(error_code)`；
- `sys_fork()` 调用 `do_fork(0, current->tf->gpr.sp, current->tf)`；
- `sys_wait(pid, code_store)` 检查非空的可写 `code_store` 后调用 `do_wait()`；
- `sys_exec(name, len, binary, size)` 检查名称和程序数据的可读区间后调用 `do_execve()`；
- `sys_yield()` 和 `sys_kill(pid)` 分别调用 `do_yield()` 和 `do_kill()`；
- `sys_getpid()` 返回 `current->pid`；
- `sys_putc(c)` 调用 `cputchar(c)`；
- `sys_pgdir()` 调用工程已声明的 `print_pgdir()`。

当前 `user/libs/syscall.c` 尚没有 `SYS_exec` 的用户封装；Function 1 按当前 `do_execve(const char *, size_t, unsigned char *, size_t)` 的真实声明完成了 `a1` 至 `a4` 的内核分发布局，未越界实现 Function 2 的 `kernel_execve()` 或 `do_execve()`。

整体调用链为：

```text
用户程序
    ↓
user/libs/syscall.c
    ↓
准备寄存器
    ↓
ecall
    ↓
exception_handler()
    ↓
识别 CAUSE_USER_ECALL
    ↓
syscall()
    ↓
读取系统调用号和参数
    ↓
调用对应 sys_xxx()
    ↓
执行具体内核功能
    ↓
写回返回值
```

---

### 4.1.4 关键代码

用户态 `ecall` 分支的关键点是单独处理并立即返回，避免再进入非法指令和断点使用的指令长度判定：

```c
case CAUSE_USER_ECALL:
    tf->epc += 4;
    syscall();
    return;
```

分发器依次检查范围和函数指针，并把返回值写回 `a0`：

```c
if (num < NUM_SYSCALLS && syscalls[num] != NULL) {
    tf->gpr.a0 = syscalls[num](arg);
}
else {
    tf->gpr.a0 = -E_INVAL;
}
```

---

### 4.1.5 Prompt

完整 Prompt 见：

```text
report/prompt.md
```

---

### 4.1.6 测试与验证

实际执行了：

```bash
cd ~/os-lab/nku-os/code
make clean
make -j4
```

`kern/trap/trap.c` 和 `kern/syscall/syscall.c` 均成功编译，所有用户库与用户测试程序也已编译。最终内核链接失败，未定义符号为 `do_execve`、`do_wait`、`do_exit`、`do_yield`、`do_kill` 和 `print_pgdir`。前五个分别属于尚未完成的 Function 2 和 Function 4；`print_pgdir()` 已在 `pmm.h` 中声明，但当前 starter 没有函数定义。本轮未为通过链接而越界增加临时实现。

`git diff --check` 实际执行时无输出，未发现空白错误。由于完整内核未能链接，本轮未运行 QEMU 和 grade，也未将它们记录为通过。

---

## 4.2 功能模块二：创建并加载第一个用户进程

**负责人：2412771-谭锦睿**

### 4.2.1 任务要求

本功能负责建立并加载第一个用户进程，使系统从只能执行内核线程进一步发展为能够运行用户程序。

主要需要更新和完善：

```text
alloc_proc()

init_main()

kernel_thread()

do_wait()

user_main()

kernel_execve()

do_execve()

load_icode()
```

主要文件：

```text
kern/process/proc.c
```

整体过程为：

```text
proc_init()
    ↓
建立 initproc
    ↓
init_main()
    ↓
建立用户程序执行实体
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
加载用户程序
    ↓
建立用户栈
    ↓
构造 TrapFrame
    ↓
进入用户态
```

---

### 4.2.2 `proc_struct` 的扩展

Lab5 中需要进一步利用 `proc_struct` 中和进程关系有关的字段。

重点包括：

```text
parent
→ 父进程

cptr
→ 子进程

yptr
→ younger sibling

optr
→ older sibling

wait_state
→ 等待状态
```

同时继续使用：

```text
state
pid
kstack
need_resched
mm
context
tf
pgdir
```

等已有成员。

---

### 4.2.3 `init_main()`

`init_main()` 是用户进程启动过程中的重要内核线程入口。

总体关系：

```text
proc_init()
    ↓
kernel_thread()
    ↓
init_main()
```

实现中，`proc_init()` 通过 `kernel_thread(init_main, NULL, 0)` 创建 PID 1，再通过 `find_proc()` 设置 `initproc` 并命名为 `init`。`init_main()` 继续创建执行 `user_main()` 的内核线程，并用 `do_wait(0, NULL)` 回收子进程。

当前 `do_fork()` 中的 Lab5 父子链更新属于 Function 3，因此完整等待链仍依赖 Function 3 将新进程接入 `cptr/yptr/optr`。

---

### 4.2.4 `user_main()`

`user_main()` 用于从已有内核线程执行流过渡到用户程序加载过程。

主要关系：

```text
init_main()
    ↓
创建 user_main
    ↓
user_main()
    ↓
kernel_execve()
```

`user_main()` 使用 starter 提供的用户程序嵌入符号。普通构建时执行 `KERNEL_EXECVE(hello)`；grade 通过 `TEST`、`TESTSTART` 和 `TESTSIZE` 指定程序时，执行 `KERNEL_EXECVE2(TEST, TESTSTART, TESTSIZE)`。本轮没有创建额外用户程序。

---

### 4.2.5 `kernel_execve()`

`kernel_execve()` 用于从当前内核执行环境启动用户程序。

它需要连接：

```text
内核线程执行流
    ↓
exec
    ↓
用户程序执行流
```

实现中先调用 `do_execve()`。装载成功后不作普通 C 函数返回，而是调用 `forkrets(current->tf)` 进入现有 `__trapret`。`RESTORE_ALL` 恢复新的用户 TrapFrame，再由 `sret` 首次进入 U Mode。

---

### 4.2.6 `do_execve()`

`do_execve()` 用于用一个新的用户程序替换当前进程原来的执行内容。

可以概括为：

```text
当前进程
    ↓
处理原地址空间
    ↓
加载新的程序
    ↓
建立新的执行现场
```

进程本身仍然存在，但运行的程序被替换。`do_execve()` 先校验并复制进程名。对已有用户地址空间的调用者，先将 ELF 复制到内核缓冲区，避免释放旧页表后访问失效用户指针。

随后切换至 `boot_pgdir`，通过 `mm_count_dec()`、`exit_mmap()`、`put_pgdir()` 和 `mm_destroy()` 释放不再共享的旧地址空间，再调用 `load_icode()`。若旧空间已释放后装载失败，则调用 Function 4 的 `do_exit()`。

---

### 4.2.7 `load_icode()`

`load_icode()` 是用户程序装载过程中的核心函数。

主要流程包括：

```text
创建 mm_struct
    ↓
建立用户页表
    ↓
解析 ELF
    ↓
遍历 Program Header
    ↓
建立 VMA
    ↓
分配物理页
    ↓
建立虚拟地址映射
    ↓
加载代码段和数据段
    ↓
建立用户栈
    ↓
设置 TrapFrame
```

主要涉及：

```text
elfhdr
proghdr

mm_struct
vma_struct

mm_map()
page_insert()
pgdir_alloc_page()

USTACKTOP
USTACKSIZE

PTE_U
PTE_R
PTE_W
PTE_X
```

实现先检查 ELF 头、Program Header 大小与边界、入口地址，然后遍历 `ELF_PT_LOAD` 段。`ELF_PF_R/W/X` 分别转换为 `VM_READ/WRITE/EXEC` 和 `PTE_R/W/X`，所有用户映射增加 `PTE_U`。

装载从 `ROUNDDOWN(p_va, PGSIZE)` 起逐页分配，每页先清零，再只复制与 `[p_va, p_va+p_filesz)` 相交的部分，因此可处理非页对齐地址、跨页数据和 `p_memsz > p_filesz` 的 BSS。

用户栈 VMA 使用 `[USTACKTOP-USTACKSIZE, USTACKTOP)`，并在栈顶初始分配 4 个可读写用户页。完成后设置 `current->mm/current->pgdir`，更新 `satp` 并刷新 TLB。

---

### 4.2.8 第一次进入用户态

完成用户程序加载后，需要为用户程序准备正确的 TrapFrame。

最终：

```text
TrapFrame
    ↓
RESTORE_ALL
    ↓
sret
    ↓
U Mode
    ↓
用户程序入口
```

需要正确设置：

```text
epc
sp
sstatus
```

等状态。

实际 TrapFrame 整体清零，`epc` 设为 ELF `e_entry`，`sp` 设为 `USTACKTOP`，`sstatus` 清除 `SSTATUS_SPP` 和 `SSTATUS_SIE`、置位 `SSTATUS_SPIE`。这与当前 `RESTORE_ALL` 通过 `SPP` 判断用户返回并设置 `sscratch` 的逻辑一致。

---

### 4.2.9 `do_wait()`

`do_wait()` 用于父进程等待子进程结束。

基本过程：

```text
查找目标子进程
    ↓
检查是否已经退出
    ↓
已退出
→ 进行回收

尚未退出
→ 当前进程等待
→ schedule()
```

`do_wait()` 先检查非空 `code_store` 是否为可写用户区间。`pid != 0` 时只接受属于当前进程的指定子进程；`pid == 0` 时沿 `cptr/optr` 查找任意子进程。

对 `PROC_ZOMBIE` 子进程，先取退出码，在关中断区间用 `unhash_proc()` 和 `remove_links()` 移除进程，再用 `put_kstack()` 和 `kfree()` 回收资源。如果子进程仍在运行，父进程进入 `PROC_SLEEPING/WT_CHILD` 并调用 `schedule()`，醒来后重新检查。

---

### 4.2.10 主要涉及知识

- 用户进程；
- `proc_struct`；
- 父子进程关系；
- `mm_struct`；
- `vma_struct`；
- ELF；
- Program Header；
- 用户虚拟地址空间；
- 用户栈；
- TrapFrame；
- `sstatus`；
- `sret`；
- `exec`；
- `wait`。

---

### 4.2.11 实现过程

先扫描所有 Function 2 标记，保留 Lab4 已有的内核线程、`copy_thread()` 和 `proc_run()` 基础。`alloc_proc()` 显式初始化 Lab5 父子关系与等待字段。`kernel_thread()` 继续设置 `SPP=1`，保证普通内核线程返回 S Mode。

随后完成 `init_main/user_main/kernel_execve/do_execve/load_icode/do_wait`，并在 `proc_init()` 中创建 `initproc`。没有修改 Function 3 的 `do_fork/copy_mm/dup_mmap/copy_range`，也没有实现 Function 4 主体。

---

### 4.2.12 关键代码

进入统一 Trap 返回路径：

```c
int ret = do_execve(name, strlen(name), binary, size);
if (ret == 0) {
    forkrets(current->tf);
}
```

用户态 TrapFrame 的关键状态：

```c
tf->gpr.sp = USTACKTOP;
tf->epc = (uintptr_t)elf->e_entry;
tf->status = (read_csr(sstatus) | SSTATUS_SPIE) &
             ~(SSTATUS_SPP | SSTATUS_SIE);
```

---

### 4.2.13 Prompt

完整 Prompt 见：

```text
report/prompt.md
```

---

### 4.2.14 测试与验证

实际先执行 `make obj/kern/process/proc.o`，`proc.c` 单文件编译通过。随后执行：

```bash
cd ~/os-lab/nku-os/code
make clean
make -j4
```

Function 2、Function 1、用户库和全部用户程序均成功编译。最终链接阶段已不再报告 `do_execve()` 或 `do_wait()` 缺失，但仍因 Function 4 未完成的 `do_exit()`、`do_yield()`、`do_kill()` 和 starter 仅声明未定义的 `print_pgdir()` 而失败。

因内核未能链接，本轮没有运行 QEMU 或 grade，也没有修改 Function 3/4 或增加临时桩强行通过。

---

## 4.3 功能模块三：复制用户进程

**负责人：2413581-黄奕浩**

### 4.3.1 任务要求

本功能主要完成用户进程复制。

主要函数包括：

```text
do_fork()
copy_mm()
dup_mmap()
copy_range()
```

主要文件：

```text
kern/process/proc.c
kern/mm/vmm.c
kern/mm/pmm.c
```

整体关系：

```text
fork()
    ↓
do_fork()
    ↓
copy_mm()
    ↓
dup_mmap()
    ↓
copy_range()
    ↓
建立新的用户子进程
```

---

### 4.3.2 主要涉及知识

- `fork()`；
- `proc_struct`；
- `mm_struct`；
- `vma_struct`；
- 用户地址空间复制；
- 页表；
- TrapFrame；
- 父子进程关系。

---

### 4.3.3 实现过程

待负责人完成功能后补充。

---

### 4.3.4 关键代码

待负责人完成功能后补充。

---

### 4.3.5 调用关系

待负责人完成后补充。

---

### 4.3.6 Prompt

完整 Prompt 见：

```text
report/prompt.md
```

---

### 4.3.7 测试与验证

待负责人完成后补充。

---

## 4.4 功能模块四：进程退出、让出与终止

**负责人：2411302-王乐之**

### 4.4.1 任务要求

本功能主要实现：

```text
do_exit()
do_yield()
do_kill()
```

主要文件：

```text
kern/process/proc.c
```

分别用于：

```text
do_exit()
→ 当前进程退出

do_yield()
→ 当前进程主动让出 CPU

do_kill()
→ 请求终止指定进程
```

---

### 4.4.2 主要涉及知识

- 进程退出；
- `PROC_ZOMBIE`；
- `wait`；
- 父子进程；
- `need_resched`；
- 进程状态；
- 资源回收；
- 调度。

---

### 4.4.3 实现过程

待负责人完成功能后补充。

---

### 4.4.4 关键代码

待负责人完成功能后补充。

---

### 4.4.5 调用关系

待负责人完成后补充。

---

### 4.4.6 Prompt

完整 Prompt 见：

```text
report/prompt.md
```

---

### 4.4.7 测试与验证

待负责人完成后补充。

---

## 4.5 拓展任务

### 4.5.1 用户程序预加载与普通操作系统程序加载方式分析

**负责人：2412771-谭锦睿**

在完成 Function 2 的过程中，我注意到 Lab5 中的用户程序并不是等系统启动以后再从磁盘读取，而是在编译内核时就已经提前放进了内核镜像中。

例如 `user_main()` 中可以通过：

```text
KERNEL_EXECVE(hello)
```

直接取得 `hello` 用户程序对应的数据，然后交给：

```text
kernel_execve()
    ↓
do_execve()
    ↓
load_icode()
```

继续处理。

结合 Makefile 可以看出，`user/` 目录下的程序会先被编译成独立的 ELF 文件，之后在链接内核时作为二进制数据一起放进最终的 kernel 中。

所以这里需要区分两个过程：

```text
编译阶段
用户 ELF 被放进内核镜像

运行阶段
load_icode() 再把这个 ELF
真正加载到用户进程地址空间
```

也就是说，“预加载”并不是说用户程序一开始就已经位于自己的用户虚拟地址空间中，而只是它的 ELF 文件内容已经提前保存在内核镜像里。

真正运行用户程序时，`load_icode()` 还需要重新解析 ELF，并完成：

```text
建立 mm_struct
    ↓
建立 VMA
    ↓
分配和映射物理页
    ↓
加载代码段和数据段
    ↓
处理 BSS
    ↓
建立用户栈
    ↓
构造 TrapFrame
```

之后才能通过：

```text
RESTORE_ALL
    ↓
sret
```

进入用户态执行。

普通操作系统中的程序加载方式和这里不太一样。正常情况下，可执行程序一般保存在磁盘和文件系统中。执行程序时，内核需要先根据文件路径找到对应 ELF 文件，再从文件系统读取程序内容，然后建立地址空间并进行加载。

可以简单比较为：

```text
Lab5：

用户 ELF
    ↓
编译内核时提前嵌入 kernel
    ↓
运行时直接取得 ELF 数据
    ↓
load_icode()
```

而普通操作系统：

```text
可执行文件
    ↓
保存在文件系统
    ↓
exec 时查找并读取文件
    ↓
加载 ELF
```

我认为 Lab5 采用这种方式主要是因为当前实验还没有实现完整的文件系统。如果现在就按照普通操作系统的方式加载程序，还需要先完成磁盘、文件查找和文件读取等内容，会让本实验依赖后面的文件系统功能。

因此这里先把用户程序提前放进内核镜像，就可以把实验重点放在：

```text
用户进程创建
ELF 加载
用户地址空间建立
用户态切换
系统调用
```

这些 Lab5 本身需要学习的内容上。

通过这部分分析，我对“用户程序已经在内核镜像中”和“用户程序已经加载到用户地址空间中”这两个概念的区别更加清楚。前者发生在编译链接阶段，后者才真正发生在运行时的 `load_icode()` 中。

---

### 4.5.2 Copy on Write / Dirty COW

**负责人：2413581-黄奕浩**

主要任务：

```text
Copy on Write
Dirty COW
```

待负责人完成后补充：

- 实现思路；
- 修改文件；
- 关键代码；
- 页面状态变化；
- Page Fault 处理；
- 测试结果；
- Dirty COW 分析。

---

### 4.5.3 GDB 调试系统调用及返回

**负责人：2411302-王乐之**

主要任务是使用 GDB 跟踪：

```text
用户态
    ↓
ecall
    ↓
Trap
    ↓
内核态
    ↓
系统调用处理
    ↓
sret
    ↓
用户态
```

重点观察：

```text
scause
sepc
sstatus
sscratch
sp
a0 ~ a5
```

待负责人完成后补充：

- GDB 断点；
- 调试命令；
- 寄存器变化；
- 用户态进入内核态过程；
- 内核态返回用户态过程；
- 调试截图。

---

# 五、关键知识理解

## 5.1 用户态与内核态

待学习和实验完成后补充。

重点包括：

```text
U Mode
S Mode
特权级
ecall
sret
```

---

## 5.2 系统调用

待学习和实验完成后补充。

重点流程：

```text
用户程序
    ↓
系统调用封装
    ↓
ecall
    ↓
Trap
    ↓
syscall()
    ↓
具体系统调用
```

---

## 5.3 TrapFrame 与用户态返回

待学习和实验完成后补充。

重点包括：

```text
TrapFrame
sepc
sstatus
sscratch
sp
RESTORE_ALL
sret
```

---

## 5.4 用户进程地址空间

待学习和实验完成后补充。

主要关系：

```text
proc_struct
    ↓
mm_struct
    ↓
vma_struct
    ↓
页表
    ↓
PTE
    ↓
Page
```

---

## 5.5 ELF 程序加载

待学习和实验完成后补充。

重点包括：

```text
ELF Header
Program Header
代码段
数据段
BSS
用户栈
程序入口
```

---

## 5.6 fork

待学习和实验完成后补充。

---

## 5.7 exec

待学习和实验完成后补充。

---

## 5.8 exit 与 wait

待学习和实验完成后补充。

---

## 5.9 Copy on Write

待拓展任务完成后补充。

---

# 六、测试与验证

## 6.1 编译验证

待代码完成后执行：

```bash
cd ~/os-lab/nku-os/code

make clean
make -j4
```

记录实际编译结果。

---

## 6.2 QEMU 运行验证

执行：

```bash
make qemu
```

待完成后记录实际运行结果。

---

## 6.3 官方测试

执行：

```bash
make grade
```

待完成后根据实际测试输出填写：

```text
Function 1：
待测试

Function 2：
待测试

Function 3：
待测试

Function 4：
待测试

Total Score：
待测试
```

不得在正式测试完成前填写具体分数。

---

## 6.4 拓展任务验证

### 用户程序预加载分析

结合本实验的 Makefile、`user_main()`、`KERNEL_EXECVE`、`kernel_execve()` 和 `load_icode()`，确认了 Lab5 中用户程序的基本加载方式。

用户程序首先在编译阶段生成 ELF，并在链接内核时作为二进制数据放进 kernel。系统运行后，`user_main()` 通过 `KERNEL_EXECVE` 取得对应程序的数据，再由 `kernel_execve()`、`do_execve()` 和 `load_icode()` 建立真正的用户地址空间并加载程序。

因此 Lab5 中“用户程序提前进入内核镜像”和“程序真正进入用户地址空间”是两个不同阶段。

和普通操作系统相比，本实验省略了从文件系统查找和读取 ELF 的步骤，主要是为了在还没有实现完整文件系统的情况下，先学习用户进程、ELF 加载和用户态切换机制。

### Copy on Write / Dirty COW

待完成后填写。

### GDB 系统调用调试

待完成后填写。

---

# 七、实验结果

待四个功能模块全部完成并经过实际测试后填写。

主要记录：

1. 用户程序能否通过 `ecall` 正确进入内核；
2. 系统调用能否正确分发和返回；
3. 第一个用户程序能否正确加载；
4. CPU 能否正确进入用户态；
5. 用户地址空间是否正确建立；
6. `fork()` 是否能够正确创建子进程；
7. 用户地址空间复制是否正确；
8. `exit()`、`wait()`、`yield()`、`kill()` 是否正常；
9. 最终官方测试结果。

---

# 八、实验总结

待实验全部完成后结合实际实现和测试结果填写。

可围绕以下整体流程进行总结：

```text
内核线程
    ↓
创建用户进程
    ↓
加载 ELF
    ↓
建立用户地址空间
    ↓
进入用户态
    ↓
系统调用
    ↓
fork / exec
    ↓
exit / wait
    ↓
进程生命周期管理
```

同时结合：

```text
Lab2
→ 物理内存和页表

Lab3
→ Trap 和特权级切换

Lab4
→ 进程创建、调度和上下文切换

Lab5
→ 用户进程、系统调用和进程生命周期
```

说明各实验之间的联系。