# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|---|---|
| 实验名称 | Lab4：进程管理 |
| 小组成员 | 2412771-谭锦睿、2413581-黄奕浩、2411302-王乐之 |
| 完成日期 | 2026-11-20 |
| 实验分支 | `lab4` |
| 代码目录 | `code/` |
| 报告目录 | `report/` |

---

## 小组分工

### 练习分工

| 成员 | 负责内容 |
|---|---|
| 2412771-谭锦睿 | Function 1：页面映射相关操作。负责 `page_insert()`、`page_remove()` 及其相关页表、页引用计数、TLB 刷新逻辑的实现和分析。 |
| 2413581-黄奕浩 | Function 2：进程 / 内核线程创建相关内容。 |
| 2411302-王乐之 | Function 3：进程调度与上下文切换相关内容。 |
| 三人共同完成 | 最终代码整合、编译运行、QEMU 测试、官方测试以及实验报告整理。 |

### 拓展练习分工

| 成员 | 拓展练习 |
|---|---|
| 2412771-谭锦睿 | Challenge 2：分析 Sv32、Sv39、Sv48 分页模式及 `get_pte()` 的设计。 |
| 2411302-王乐之 | Challenge 1：分析 `local_intr_save()` / `local_intr_restore()` 如何实现中断状态保存、关闭和恢复。 |
| 2413581-黄奕浩 | 无单独 Challenge，主要完成本人 Function 2 及对应报告。 |

### 任务完成情况

- [x] 恢复 Lab2 / Lab3 前置依赖
- [x] Function 1：完成 `page_insert()` / `page_remove()`
- [x] Function 2：完成六个创建函数
- [x] Function 3：完成进程调度与上下文切换及中断保护 Challenge
- [x] 三人代码整合
- [x] 完整编译
- [x] QEMU 运行验证
- [x] 官方测试
- [x] 完善实验报告
- [x] 整理完整 Prompt

### 报告分工

| 成员 | 报告负责内容 |
|---|---|
| 2412771-谭锦睿 | 页面映射、`page_insert()`、`page_remove()`、分页模式及 `get_pte()` Challenge |
| 2413581-黄奕浩 | Function 2 实际实现、代码分析、测试结果 |
| 2411302-王乐之 | Function 3 实际实现、中断保护 Challenge、测试结果 |
| 三人共同完成 | 整体测试、实验问题、最终总结 |

实际使用的 Prompt 统一整理在：

```text
report/prompt.md
```

---

# 一、实验目的

本实验在前面物理内存管理、页表以及 Trap 机制的基础上，进一步学习操作系统中的进程管理机制。

主要目标包括：

1. 理解页面映射建立、替换和删除过程；
2. 理解物理页引用计数的作用；
3. 理解进程控制块及进程创建的基本过程；
4. 理解进程调度与上下文切换机制；
5. 理解 Lab2、Lab3 中已经实现的内存管理和 Trap 机制如何继续为 Lab4 服务；
6. 通过编译和运行测试验证 Lab4 实现。

---

# 二、实验环境

| 项目 | 环境 |
|---|---|
| 主机环境 | Windows + WSL2 Ubuntu 24.04 |
| 目标架构 | RISC-V 64 |
| 交叉编译工具链 | `riscv64-unknown-elf-*` |
| 虚拟机 | QEMU |
| 固件 | OpenSBI |
| 版本管理 | Git |
| 实验分支 | `lab4` |

---

# 三、实验前置依赖恢复

Lab4 依赖 Lab2 和 Lab3 中已经实现的部分代码。

按照实验框架中的 `lab2` / `lab3` 注释进行了扫描，共找到：

```text
Lab2：2 处
Lab3：3 处
共涉及 4 个源码文件
```

涉及：

```text
code/kern/mm/pmm.c
code/kern/mm/default_pmm.c
code/kern/trap/trap.c
code/kern/trap/trapentry.S
```

恢复内容包括：

```text
Lab2：
page_init()
物理页 Best-Fit 分配器

Lab3：
interrupt_handler()
exception_handler()
RESTORE_ALL
__alltraps
```

迁移过程中保留了实验代码中原有的 Lab2 / Lab3 注释，没有修改 Lab4 新 Function。

恢复完成后：

```text
make clean
make
```

源码编译阶段可以通过。

此时剩余链接错误为：

```text
page_insert
page_remove
proc_init
cpu_idle
```

均属于 Lab4 尚未完成的 Function。

---

# 四、Function 1：页面映射管理

**负责人：2412771-谭锦睿**

## 4.1 任务要求

本部分主要实现：

```c
page_insert()
page_remove()
```

主要文件：

```text
code/kern/mm/pmm.c
```

需要正确处理：

- 建立新的虚拟地址到物理页的映射；
- 替换已有映射；
- 删除已有映射；
- 维护页面引用计数；
- 必要时释放物理页；
- 修改页表后刷新 TLB。

---

## 4.2 page_remove_pte()

`page_remove_pte()` 用于删除一个已经存在的页表映射。

核心代码：

```c
static void
page_remove_pte(pde_t *pgdir, uintptr_t la, pte_t *ptep)
{
    if (*ptep & PTE_V)
    {
        struct Page *page = pte2page(*ptep);
        if (page_ref_dec(page) == 0)
        {
            free_page(page);
        }
        *ptep = 0;
        tlb_invalidate(pgdir, la);
    }
}
```

主要流程：

```text
检查 PTE 是否有效
        ↓
找到对应物理页
        ↓
减少引用计数
        ↓
引用计数为 0 时释放物理页
        ↓
清除 PTE
        ↓
刷新 TLB
```

只有引用计数降为 0 时才能释放物理页，因为同一个物理页可能仍然被其他虚拟地址映射。

---

## 4.3 page_remove()

实现：

```c
void
page_remove(pde_t *pgdir, uintptr_t la)
{
    pte_t *ptep = get_pte(pgdir, la, 0);
    if (ptep != NULL)
    {
        page_remove_pte(pgdir, la, ptep);
    }
}
```

这里调用：

```c
get_pte(pgdir, la, 0)
```

其中 `create = 0`。

原因是该函数只是删除映射，如果页表本来不存在，就不应该为了执行删除操作而重新分配新的页表页。

---

## 4.4 page_insert()

实现：

```c
int
page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm)
{
    pte_t *ptep = get_pte(pgdir, la, 1);
    if (ptep == NULL)
    {
        return -E_NO_MEM;
    }

    page_ref_inc(page);

    if (*ptep & PTE_V)
    {
        struct Page *old_page = pte2page(*ptep);

        if (old_page == page)
        {
            page_ref_dec(page);
        }
        else
        {
            page_remove_pte(pgdir, la, ptep);
        }
    }

    *ptep = pte_create(page2ppn(page), perm);
    tlb_invalidate(pgdir, la);

    return 0;
}
```

首先：

```c
get_pte(pgdir, la, 1)
```

查找目标 PTE。

这里 `create = 1`，说明如果中间页表不存在，可以分配新的页表页。

如果创建失败：

```c
return -E_NO_MEM;
```

---

## 4.5 page_insert() 的三种情况

### 情况一：原地址没有映射

直接：

```text
page_ref_inc(page)
        ↓
写入新的 PTE
        ↓
刷新 TLB
```

新页面增加一个引用。

### 情况二：原地址已经映射到同一个页面

首先：

```c
page_ref_inc(page);
```

然后发现：

```c
old_page == page
```

此时执行：

```c
page_ref_dec(page);
```

抵消刚刚增加的引用。

因此引用计数保持不变，只更新新的页表权限。

### 情况三：原地址映射到另一个页面

首先增加新页面引用计数，然后：

```c
page_remove_pte(pgdir, la, ptep);
```

删除旧映射。

旧页面的引用计数随之减少，如果引用数为 0，则释放旧页面。

之后再写入新页面映射。

---

## 4.6 为什么需要刷新 TLB

CPU 访问虚拟地址时，并不一定每次都重新查询页表，而可能直接使用 TLB 中缓存的地址转换结果。

如果已经修改了页表，但 TLB 中仍保存旧映射，则 CPU 可能继续使用旧的物理地址。

因此建立、替换或者删除页表映射后需要执行：

```c
tlb_invalidate(pgdir, la);
```

使旧地址转换失效。

---

## 4.7 Function 1 调用关系

插入：

```text
page_insert()
    ↓
get_pte()
    ↓
page_ref_inc()
    ↓
检查旧映射
    ↓
page_remove_pte()（必要时）
    ↓
写入新 PTE
    ↓
tlb_invalidate()
```

删除：

```text
page_remove()
    ↓
get_pte()
    ↓
page_remove_pte()
    ↓
page_ref_dec()
    ↓
free_page()（引用计数为 0）
    ↓
清除 PTE
    ↓
tlb_invalidate()
```

---

# 五、Challenge 2：分页模式与 get_pte()

**负责人：2412771-谭锦睿**

## 5.1 Sv32、Sv39、Sv48

RISC-V 支持多种分页模式。

### Sv32

主要用于 RV32：

```text
VPN[1] | VPN[0] | page offset
```

使用二级页表。

### Sv39

用于 RV64：

```text
VPN[2] | VPN[1] | VPN[0] | page offset
```

使用三级页表。

每级 VPN 为 9 位，页内偏移为 12 位：

```text
9 + 9 + 9 + 12 = 39 bit
```

当前实验使用的是 Sv39。

代码中可以看到三级索引：

```text
PDX1
PDX0
PTX
```

同时启动页表使用：

```text
boot_page_table_sv39
```

因此可以确定当前工程采用 Sv39。

### Sv48

Sv48 在 Sv39 基础上增加一级页表：

```text
VPN[3] | VPN[2] | VPN[1] | VPN[0] | page offset
```

使用四级页表。

---

## 5.2 为什么 get_pte() 中存在两段相似代码

`get_pte()` 的主要任务是逐级遍历多级页表。

每一级中间页表的处理过程基本相同：

```text
根据 VPN 获得当前页表项
        ↓
检查页表项是否有效
        ↓
如果无效且 create = 1
        ↓
分配新的页表页
        ↓
初始化新的页表页
        ↓
将页表页地址写入当前页表项
        ↓
进入下一级页表
```

因此多级页表中不同层级的代码形式会非常相似。

这种相似并不是偶然的，而是因为不同层级的页表使用了类似的数据结构和遍历方法。

---

## 5.3 get_pte() 是否应该拆分

当前接口：

```c
get_pte(pgdir, la, create)
```

同时支持查询和创建。

例如：

```c
get_pte(pgdir, la, 0);
```

表示只查找。

而：

```c
get_pte(pgdir, la, 1);
```

表示页表不存在时允许创建。

这种设计的优点是：

```text
接口统一
调用简单
复用了页表遍历逻辑
```

但问题是查询函数同时可能执行：

```text
内存分配
页表修改
```

因此存在一定副作用。

另一种设计可以拆成：

```text
find_pte()
create_pte()
```

这样职责更加清晰，但会增加接口数量，而且可能产生重复的页表遍历逻辑。

因此两种设计并没有绝对优劣。

当前 uCore 使用 `create` 参数，是在接口简洁性和职责单一性之间做出的设计权衡。

---

## 5.4 Function 1 编译与验证

执行：

```bash
cd ~/os-lab/nku-os/code
make
```

结果：

```text
pmm.c 编译通过。

page_insert 不再出现 undefined reference。
page_remove 不再出现 undefined reference。
```

此时最终链接仍然存在：

```text
proc_init
cpu_idle
```

未定义。

这两个函数属于其他成员负责的 Function，因此本部分没有继续修改。

同时执行：

```bash
git diff --check
```

结果通过。

工程中已经存在：

```text
check_pgdir()
check_boot_pgdir()
test_lab4_pmm_basic()
```

等页面映射测试代码。

但由于完整内核尚未链接成功，目前无法启动 QEMU 实际运行这些测试。

因此当前 Function 1 的验证结果为：

```text
代码编译通过
符号链接问题已经解决
完整运行测试等待三人代码合并后完成
```

---

# 六、Function 2：进程 / 内核线程创建

**负责人：2413581-黄奕浩**

## 6.1 任务要求

按照[指导书功能模块二](http://oslab.mobisys.top/lab2026/_book/lab4/lab4_2_1_exercises.html)，实现 `alloc_proc()`、`proc_init()`、`kernel_thread()`、`do_fork()`、`copy_thread()` 和 `copy_mm()`，使内核建立 PID 为 0 的 idleproc 与 PID 为 1 的 initproc，并能够继续创建、登记可运行的内核线程。本模块准备首次运行所需的数据；实际调度、切换和执行属于 Function 3。

## 6.2 主要涉及文件

| 文件 | 本人修改内容 |
|---|---|
| `code/kern/process/proc.c` | 在原 Function 2 注释下补全上述六个函数 |
| `code/tools/test_function2.py` | 新增独立副本的构建、QEMU 运行与结果检查脚本 |
| `code/tools/test_function2.inc` | 新增创建功能与失败回滚测试，仅在副本中使用 |
| `report/prompt.md` | 记录本人实现与验证要求 |

没有修改 Function 1、Function 3、公共结构体或原有评分测试。

## 6.3 实现过程

`alloc_proc()` 使用 `kmalloc()` 申请 PCB，先整体清零，再设置 `state = PROC_UNINIT`、`pid = -1`、`pgdir = boot_pgdir_pa`，并初始化两个链表节点。清零保证运行次数、调度标志、父进程、mm、trapframe、context、flags 和名称没有未定义初值。页表字段保存的是物理地址，不是 `boot_pgdir_va` 或带 MODE 的 satp 完整值。

`proc_init()` 初始化全局进程链表和 PID 哈希桶。idleproc 直接复用启动阶段已经使用的 `bootstack`，设置 PID 0、RUNNABLE、need_resched 为真以及名称 idle，再令 `current = idleproc`。它不通过 fork 创建，也不加入普通进程链表和 PID 哈希表；`nr_process` 仍包含 idleproc。随后调用 `kernel_thread(init_main, "Hello world!!", 0)`，从哈希表找到返回 PID 对应的 initproc，确认 PID 为 1，并命名为 init。

`kernel_thread()` 构造临时 trapframe，将线程函数放入 s0、参数放入 s1，将 epc 指向 `kernel_thread_entry`。status 在当前 sstatus 基础上设置 SPP、SPIE 并清除 SIE：SPP 使 sret 返回 S 态，SPIE 使返回后的 SIE 恢复为开，保存帧中的 SIE 则保持关闭。随后通过 `do_fork(clone_flags | CLONE_VM, 0, &tf)` 创建线程。

`do_fork()` 先检查进程数上限与必要参数，再依次申请 PCB、设置父进程、申请两页内核栈、调用 `copy_mm()`、调用 `copy_thread()`。准备成功后，在保存并关闭中断的临界区内分配唯一 PID、加入 PID 哈希表和进程链表、增加计数、调用 `wakeup_proc()`，最后恢复进入临界区前的中断状态并返回子线程 PID。准备未完成的 PCB 不会提前对调度器可见。

`copy_mm()` 在当前内核线程场景下保留 `proc->mm = NULL`。如果父线程出现非空 mm，返回 `-E_INVAL`，明确拒绝尚未实现的用户地址空间复制。它不改变父进程的 mm，也不增加不存在的地址空间引用计数。

`copy_thread()` 把临时 trapframe 按值复制到新内核栈顶部，设置子线程 a0 为 0；传入栈地址非零时使用指定地址，否则把恢复后的 sp 指向栈上的 trapframe 起始位置。context.ra 指向 `forkret`，context.sp 指向这个 trapframe。临时变量 tf 在函数返回后失效，不影响新线程独立保存的副本。

如果 PCB 分配失败，返回 `-E_NO_MEM`；内核栈申请失败，释放 PCB；`copy_mm()` 失败，先释放内核栈再释放 PCB；达到 `MAX_PROCESS` 时返回 `-E_NO_FREE_PROC`。失败路径不分配 PID、不加入链表或哈希表，也不增加进程计数。

## 6.4 关键代码

以下节选来自本人实际实现，完整代码见 `code/kern/process/proc.c`：

```c
proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1;
*proc->tf = *tf;
proc->tf->gpr.a0 = 0;
proc->tf->gpr.sp = esp != 0 ? esp : (uintptr_t)proc->tf;
proc->context.ra = (uintptr_t)forkret;
proc->context.sp = (uintptr_t)proc->tf;
```

```c
local_intr_save(intr_flag);
proc->pid = get_pid();
hash_proc(proc);
list_add(&proc_list, &proc->list_link);
nr_process++;
wakeup_proc(proc);
local_intr_restore(intr_flag);
return proc->pid;
```

## 6.5 关键数据结构分析

`proc_struct` 是管理对象，包含 PID、状态、父进程、内核栈、页表物理地址和两个链表节点；`context` 保存上下文切换所需的 ra、sp 与 s0～s11，由 Function 3 的 `switch_to()` 使用；`trapframe` 保存完整通用寄存器与 status、epc、badvaddr、cause，由 `forkrets()`、`__trapret` 和 `RESTORE_ALL` 使用。两种现场承担不同工作，不能只设置其中一种。

每个普通内核线程拥有独立的两页内核栈，但共享内核页表，`mm == NULL`。trapframe 放在内核栈高地址端，首次恢复后 sp 位于该帧起始位置；后续函数调用向低地址使用栈。创建测试验证了帧地址与布局，还没有验证实际切换后的指令执行。

## 6.6 Function 2 调用关系

```text
kern_init -> proc_init
               |-> alloc_proc -> 设置 idleproc/current
               |-> kernel_thread -> 构造临时 trapframe
                                      |-> do_fork
                                            |-> alloc_proc
                                            |-> setup_kstack
                                            |-> copy_mm
                                            |-> copy_thread
                                            |-> get_pid/hash_proc/list_add
                                            |-> wakeup_proc
               |-> find_proc -> 设置 initproc

Function 3 完成后预计的首次运行路径（当前未执行验证）：
schedule -> proc_run -> switch_to -> forkret -> forkrets
         -> __trapret -> RESTORE_ALL -> sret -> kernel_thread_entry
         -> s0 指定的函数(s1 指定的参数) -> do_exit
```

`do_exit()` 目前仍是框架中的 panic 占位，不属于此次创建功能实现，不能据此声称已经实现正常退出和回收。

## 6.7 实现中遇到的问题

Function 3 尚未实现，原工程 `make -j2` 实际编译到最终链接后报 `undefined reference to cpu_idle`，因此不能直接运行完整内核或把 `make grade` 记为通过。

为验证自己的创建实现，新增脚本复制工程到 D 盘同级独立目录，在副本中以测试入口替代尚不存在的 `cpu_idle()`。测试入口只检查创建状态并关机，不调用调度器、不运行新线程。副本中的两个分配调用还被包装以注入 PCB/内核栈分配失败，正常情况下仍调用真实的 kmalloc/alloc_pages；正式源码没有故障注入开关或临时空实现。

## 6.8 测试与验证

在 WSL Ubuntu 中实际执行：

```bash
cd '/mnt/d/360MoveData/Users/huangyihao/Desktop/OS实验/lab4/nku-os/code'
python3 tools/test_function2.py
```

首次运行默认将独立副本、build.log 与 run.log 保存到 `/mnt/d/360MoveData/Users/huangyihao/Desktop/OS实验/lab4/function2-check`。已存在的副本不会被覆盖；再次测试请指定新目录：

```bash
python3 tools/test_function2.py --output "../../function2-check-$(date +%Y%m%d-%H%M%S)"
echo $?
```

脚本默认调用 PATH 中的 `qemu-system-riscv64`，本次实际使用 `/mnt/d/Tools/qemu-4.1.1/bin/qemu-system-riscv64`。可用 `--qemu /实际路径/qemu-system-riscv64` 指定其他可执行文件。测试脚本只接受 QEMU 正常退出、包含 ALL TESTS PASS 且没有 kernel panic/FAIL 的结果；超时或断言失败均返回非零。

实际输出的关键部分：

```text
[Test Process Create] PASS
[Function 2] PCB initialization PASS
[Function 2] PID/list/hash and kernel trapframe PASS
[Function 2] explicit stack and interrupt restoration PASS
[Test Process Create] PASS
[Function 2] allocation failures/rollback/process limit PASS
[Function 2] ALL TESTS PASS (creation only; scheduling excluded)
```

脚本实际退出码为 0。`test_lab4_process_create()` 是工程提供的创建检查，测试入口直接调用；其余断言是本人补充的验证。

| 测试范围 | 验证依据 |
|---|---|
| idle/init 初始状态 | 原有创建测试；PID、状态、父进程、启动栈、mm、context |
| PCB 初始化 | 未初始化状态、PID -1、清零字段、内核页表和链表节点 |
| 多线程创建 | PID 不重复、独立内核栈、哈希查找、普通链表长度、current 不变 |
| 首次运行现场 | s0/s1、epc、a0、SPP/SPIE/SIE、context.ra/sp 与栈顶帧地址 |
| 指定栈地址 | do_fork 的非零 stack 分支、原始模板不被改写 |
| 中断状态 | 创建前开中断和关中断两种情形，调用后均保持原状态 |
| 失败回滚 | PCB 失败、栈失败、非空父 mm 拒绝后空闲页数和进程计数不变 |
| 边界输入 | 进程数达到上限、空 trapframe 返回对应错误且没有残留登记 |

上限测试是在独立测试副本中临时把 `nr_process` 设为 `MAX_PROCESS` 来检查拒绝分支，随后恢复原值，没有实际创建 4096 个线程；该结果不等同于满负载压力测试。PCB 和栈申请失败也通过副本中的分配包装进行定点注入。

构建仍有原有 kmalloc 的整数转指针警告，以及原有 test_lab4.c 调度测试函数类型不匹配警告。

截图保留本人测试结果，文件名为 `report/images/huangyihao-lab4-function2-test.png`。

---

# 七、Function 3：进程调度与上下文切换

**负责人：2411302-王乐之**

## 7.1 任务要求

实现 `schedule()`、`proc_run()`、`cpu_idle()` 和 `switch_to()`，从 RUNNABLE 进程中循环选择执行对象，完成内核线程首次运行和后续现场恢复。保留原 LAB4 function3 标记，将本人的 `YOUR CODE` 替换为学号 2411302；不修改 Function 1、Function 2 或原有评分测试。

## 7.2 主要涉及文件

| 文件 | 实现 |
|---|---|
| `code/kern/schedule/sched.c` | `schedule()`：循环扫描、跳过不可运行进程、idle 回退、维护 runs 和 need_resched |
| `code/kern/process/proc.c` | `proc_run()`：中断保护、current 与 satp 更新、TLB 刷新；`cpu_idle()`：检查调度请求 |
| `code/kern/process/switch.S` | `switch_to()`：保存和恢复 ra、sp、s0～s11 |

## 7.3 实现过程

`schedule()` 首先保存中断状态并关闭本地中断，再清除当前进程的 `need_resched`。idleproc 没有加入普通进程链表，所以从 idle 出发时以 `proc_list` 表头为起点；普通进程则从自己的 `list_link` 后方开始扫描，遇到表头时跳过它并继续循环。只选择 `PROC_RUNNABLE`，扫描一圈仍未找到时选择 idleproc。这样同时覆盖空链表、全部休眠以及只有当前进程可运行的情况。

选定对象后增加 `runs`。若仍是当前进程，则不执行汇编切换；否则调用 `proc_run()`。`proc_run()` 再次保存中断状态，先记录旧进程再更新 `current`，以 `8 << 60 | pgdir >> 12` 构造 RV64 Sv39 的 satp，并执行 `sfence.vma` 刷新 TLB，最后切换两个 context。PCB 的 pgdir 保存页表物理地址，不能直接写入 satp，也不能使用当前头文件中面向 RV32 的 `lsatp(unsigned int)`。

`switch_to()` 按 `struct context` 的顺序保存旧线程的 ra、sp、s0～s11，再从目标结构恢复同一组寄存器，最后执行 `ret`。RV64 每项占 8 字节，最后一个 s11 位于偏移 104。ra、sp 决定恢复后的返回位置和内核栈；s0～s11 是调用约定要求被调用者保存的寄存器。a0～a7 和 t0～t6 可以被普通函数调用破坏，不属于这里的 context；完整异常现场由 trapframe 保存。gp、tp 在当前内核线程模型中共享，并未作为每线程状态切换。

首次调度 initproc 时，预先设置的 context.ra 指向 `forkret`，context.sp 指向其内核栈中的 trapframe；随后经 `forkrets`、`RESTORE_ALL`、`sret` 到达 `kernel_thread_entry`，再调用 s0 中的线程函数并传入 s1 中的参数。之后再次选中旧线程时，`switch_to()` 从它原来的调用点返回，继续执行该线程栈上尚未完成的中断恢复代码。

`cpu_idle()` 持续检查 `current->need_resched`，有请求就调用调度器。当前实现是合作式循环调度；原时钟中断没有新增抢占调度入口，不能把本实验描述成已实现时间片抢占、用户进程或完整退出回收。

## 7.4 关键代码

```c
local_intr_save(intr_flag);
struct proc_struct *prev = current;
current = proc;
write_csr(satp, ((uintptr_t)8 << 60) | (proc->pgdir >> RISCV_PGSHIFT));
flush_tlb();
switch_to(&prev->context, &proc->context);
local_intr_restore(intr_flag);
```

```c
if (next == NULL)
{
    next = idleproc;
}
next->runs++;
if (next != current)
{
    proc_run(next);
}
```

汇编对 ra、sp、s0～s11 分别执行 `STORE reg, offset(a0)` 和 `LOAD reg, offset(a1)`。完整实现位于原 `switch.S`，使用框架 `REGBYTES`、`STORE`、`LOAD` 宏与结构体布局对应。

## 7.5 调用关系

```text
kern_init -> proc_init（Function 2 准备 idleproc/initproc）
          -> cpu_idle -> schedule -> proc_run -> switch_to
                                                   | 首次运行
                                                   v
                         forkret -> forkrets -> __trapret
                         -> RESTORE_ALL -> sret -> kernel_thread_entry
                         -> init_main("Hello world!!")

线程主动 schedule -> 下一个 RUNNABLE 线程
                 -> 再次被选中后返回原 switch_to 调用点
```

## 7.6 Challenge 1：中断状态保存与恢复

实际实现位于 `code/kern/sync/sync.h` 和 `code/kern/driver/intr.c`：

```c
static inline bool __intr_save(void) {
    if (read_csr(sstatus) & SSTATUS_SIE) {
        intr_disable();
        return 1;
    }
    return 0;
}
static inline void __intr_restore(bool flag) {
    if (flag) {
        intr_enable();
    }
}
```

`local_intr_save(intr_flag)` 将 `__intr_save()` 返回值写入调用者的局部变量。进入前 SIE 为 1 时，`intr_disable()` 通过 `clear_csr(sstatus, SSTATUS_SIE)` 关闭 S 态中断并返回真；原来已经关闭则返回假。退出时只在标志为真时调用 `intr_enable()`，用 `set_csr` 设置 SIE。这里保存的是 SIE 开关，不是整个 sstatus；恢复函数不会覆盖其他状态位。

嵌套调用必须各自使用独立变量，并按相反顺序退出：外层保存 1 并关中断，内层看到 0；内层恢复 0 时保持关闭，外层最后恢复 1 才开启。原来就是关中断时，整对调用不会误开中断。严格说 `__intr_restore(false)` 是不操作，并非强制清零；临界区内部必须遵守不擅自开启中断的约定。

`schedule()` 和 `proc_run()` 的嵌套保护就是这种情形。线程切换后各自栈上的 `intr_flag` 仍保留，返回原调用链时再恢复各自进入时的中断状态；新线程首次运行则通过 trapframe 中的 SPIE 和 `sret` 恢复中断。此机制保护单核本地中断临界区，不能单独提供多核之间的互斥，也不能阻止同步异常。

## 7.7 实现中遇到的问题

1. idleproc 不在普通进程链表中，不能直接使用它的链表节点作为普通循环起点；采用表头起点和显式跳过表头，补测空链表与全休眠回退。
2. 工程头文件的 `lsatp()` 使用 RV32 模式及 32 位参数；本模块按已有 RV64 Sv39 页表初始化逻辑构造 satp，避免模式和地址截断错误。
3. 本地只有 `gdb-multiarch`，旧 QEMU 与默认架构自动识别不兼容，初次评分没有串口输出。在仓库外配置启动包装器，显式选择 `riscv:rv64` 后重新执行原评分脚本，获得 30/30；没有修改 Makefile 或评分检查条件。
4. init_main 返回后，框架 `do_exit()` 仍会输出 `process exit!!.` 并进入调试监视器；这是当前退出占位路径，保留原实现，不将其描述为正常线程退出与资源回收。

## 7.8 测试与验证

实际执行完整构建、默认 QEMU 和仓库评分测试，命令与整体输出见第九节。评分中的两个测试线程各自反复主动调度，原 `[Test Schedule] PASS` 检查通过。

另在仓库外独立副本中添加临时断言和汇编测试调用者，未向正式代码插入测试分支。实际输出：

```text
[Function 3] empty/all-sleeping list selects idle: PASS
[Function 3] nested interrupt restore and self selection: PASS
[Function 3] 64 ordered worker visits, stacks, satp, SIE: PASS
[Function 3] s0-s11 sentinels and sp/ra return paths: PASS
[Function 3] ALL EXTRA TESTS PASSED
```

检查内容包括：空表及全休眠时选择 idle；仅当前线程可运行时 runs 增加而不切换；need_resched 清零；开/关中断入口及嵌套恢复；两个新线程共 64 次有序运行；各线程内核栈范围与 satp；真实切换前后全部 s0～s11 哨兵值保持，并通过原 sp/ra 返回调用链。调用线程以关中断状态调度，新线程以自身 trapframe 恢复为开中断，返回调用线程后仍为关中断。补充测试主动调用 SBI 关机，QEMU 返回 0 且无 panic。

---

# 八、三个 Function 的关系

三个 Function 最终共同组成 Lab4 的进程管理框架。

目前可以简单表示为：

```text
Function 1
页面映射管理
    ↓
为后续进程管理提供内存基础

Function 2
进程 / 内核线程创建
    ↓
建立 idleproc/initproc、分配栈和 PID、准备 trapframe/context、登记并设为 RUNNABLE

Function 3
调度与上下文切换
    ↓
选择 RUNNABLE 进程、切换页表和内核栈、恢复 context，首次运行再通过 trapframe 进入线程函数
```

三个 Function 已完成整合并通过仓库评分测试。第四至六节保留队友各阶段记录，其中“等待整合”等描述反映当时状态；本次最终集成结果以第九节为准。

---

# 九、实验测试

## 9.1 编译测试

在 WSL Ubuntu 中执行：

```bash
cd /home/wlz/nku-os-lab4-git/code
make clean
make -j4
```

实际成功生成 `bin/kernel` 和 `bin/ucore.img`。Function 3 没有新增编译警告。框架 `kmalloc.c` 原有整数转指针警告、评分测试线程返回类型警告仍存在，未修改队友或原测试源码。

## 9.2 QEMU 测试

本机使用 QEMU 4.1.1 与其自带 OpenSBI 0.4；为评分脚本和默认启动共用同一环境，在仓库外配置 PATH 包装器：

```bash
mkdir -p /home/wlz/.cache/lab4-work/bin
cat > /home/wlz/.cache/lab4-work/bin/qemu-system-riscv64 <<'EOF'
#!/bin/sh
exec /home/wlz/.local/opt/qemu-4.1.1/bin/qemu-system-riscv64 -L /home/wlz/.local/opt/qemu-4.1.1/share/qemu "$@"
EOF
cat > /home/wlz/.cache/lab4-work/bin/riscv64-unknown-elf-gdb <<'EOF'
#!/bin/sh
exec /usr/bin/gdb-multiarch -ex "set architecture riscv:rv64" "$@"
EOF
chmod +x /home/wlz/.cache/lab4-work/bin/*
export PATH="/home/wlz/.cache/lab4-work/bin:$PATH"
make qemu
```

实际关键输出：

```text
check_alloc_page() succeeded!
check_pgdir() succeeded!
check_boot_pgdir() succeeded!
kmalloc_init() succeeded!
check_vmm() succeeded.
this initproc, pid = 1, name = "init"
To U: "Hello world!!".
To U: "en.., Bye, Bye. :)"
kernel panic at kern/process/proc.c:208:
    process exit!!.
```

最后一段为框架 do_exit 占位输出，进入原调试监视器后由其中的 `sbi_shutdown()` 关机。已验证首次切换和线程函数运行，没有实现完整退出和回收功能。

## 9.3 官方测试

沿用上述 PATH，执行仓库原有评分入口（自动复制到临时目录注入原测试，不修改正式源码）：

```bash
cd /home/wlz/nku-os-lab4-git/code
make grade
```

实际输出：

```text
  -check PMM:                                OK
  -check Process Create:                     OK
  -check Schedule:                           OK
Total Score: 30/30
```

分别为页面映射 10 分、创建 10 分、调度 10 分。评分脚本同时要求 initproc 的三条问候输出。补充寄存器和边界测试结果见 7.8。

---

## 9.4 Git 修改检查

```bash
git status
git diff --check
git diff --stat
git diff
```

用于检查：

```text
修改文件范围
格式问题
意外修改
三人代码整合结果
```

---

# 十、实验结果

## 10.1 Function 1

已在 `code/kern/mm/pmm.c` 完成：

```text
page_remove_pte()
page_remove()
page_insert()
```

实现能够处理：

```text
无原映射
同一页面重映射
不同页面替换
```

并正确维护：

```text
物理页引用计数
旧映射删除
物理页释放
PTE 更新
TLB 刷新
```

Function 1 当前已经完成源码编译验证。

`page_insert()` 和 `page_remove()` 已不再产生链接错误。

---

## 10.2 Function 2

**负责人：2413581-黄奕浩**

已完成六个创建函数，独立 QEMU 验证通过原有 Process Create 检查及补充分配失败、进程数上限、初始现场、中断状态等检查。
---

## 10.3 Function 3

**负责人：2411302-王乐之**

已实现 `schedule()`、`proc_run()`、`cpu_idle()` 和 `switch_to()`，完成 Challenge 1 分析。实际通过原调度评分以及空表、全休眠、自身选择、双线程轮转、寄存器和中断状态补充检查。

---

## 10.4 整体运行结果

三个 Function 已整合：完整编译成功，QEMU 能运行 initproc 并输出问候，原评分 `Total Score: 30/30`。进程退出仍为框架占位，未扩展用户态或时钟抢占。

---

# 十一、实验中遇到的问题及解决方法

## 问题一：Lab4 依赖 Lab2 / Lab3 代码

现象：

```text
Lab4 初始代码中存在多个 lab2 / lab3 占位位置，
部分前序实验函数没有实现，导致当前实验无法正常编译。
```

解决：

首先扫描所有：

```text
lab2
lab3
```

注释位置。

然后从对应 Lab2 / Lab3 分支读取已经完成的实现，并迁移到 Lab4。

最终共恢复：

```text
page_init()
物理页分配器
interrupt_handler()
exception_handler()
RESTORE_ALL
__alltraps
```

恢复后这些前序依赖代码均可以正常编译。

---

## 问题二：Function 1 完成后仍无法完整链接

现象：

完成：

```text
page_insert()
page_remove()
```

之后执行 `make`，Function 1 已经不再出现 undefined reference，但仍然存在：

```text
proc_init
cpu_idle
```

未定义。

原因：

这两个函数属于其他成员负责的 Lab4 Function，当前尚未实现。

因此没有为了强行通过链接而修改其他成员代码。

解决：

等待 Function 2 / Function 3 完成后再进行完整整合和测试。

---

## 其他问题

本次 Function 3 的链表边界、Sv39 satp 和 GDB 环境问题及解决方法见 7.7。

---

# 十二、重要知识点总结

## 12.1 页面映射不仅是修改一个 PTE

`page_insert()` 和 `page_remove()` 除了修改页表项，还需要同时处理：

```text
页面引用计数
旧映射
页面释放
TLB
```

因此页面映射实际上是一套完整的资源管理过程。

---

## 12.2 get_pte() 的 create 参数

```c
get_pte(pgdir, la, 0);
```

表示：

```text
只查询，不创建页表
```

适合删除映射。

而：

```c
get_pte(pgdir, la, 1);
```

表示：

```text
必要时分配页表
```

适合建立新的页面映射。

---

## 12.3 页面引用计数

一个物理页可能被多个虚拟地址引用。

因此不能在删除一个映射时直接释放物理页。

必须：

```text
page_ref_dec()
    ↓
判断引用计数
    ↓
ref == 0
    ↓
free_page()
```

这样才能避免仍被其他映射使用的页面被提前释放。

---

## 12.4 TLB 与页表一致性

修改页表后，如果 TLB 中还缓存旧的地址转换结果，则 CPU 仍可能继续使用旧映射。

因此修改映射后需要：

```c
tlb_invalidate(pgdir, la);
```

保持 TLB 与页表状态一致。

---

## 12.5 Function 2 / Function 3

Function 2 将普通函数和参数组织成新线程的初始现场：context 使第一次 switch_to 返回 forkret，trapframe 再使 sret 到达 kernel_thread_entry。RUNNABLE 只表示线程具备被调度条件；Function 3 负责选择并实际切换线程。创建现场已通过 Function 3 实际恢复并执行，调度与创建的集成检查通过。

---

# 十三、Lab2、Lab3 与 Lab4 的联系

Lab4 并不是完全独立的实验。

Function 1 可以直接看到 Lab2 中：

```text
Page
PTE
页表
物理页分配
```

等机制继续被使用。

同时 Lab4 后续进程管理还会继续使用 Lab3 中已经建立的：

```text
TrapFrame
Trap 入口
中断控制
```

等机制。

因此 Lab4 是前面实验功能进一步组合的阶段。

Function 2 使用 Lab2 的页分配器建立每线程独立内核栈，使用 Lab3 的 trapframe、RESTORE_ALL 和 sret 组织首次运行路径，并通过保存/恢复中断状态保护进程登记。Function 3 通过调度和 context 切换连接这些现场，使新线程真正开始执行，并在后续调度中恢复各自的内核栈。

---

# 十四、实验总结

本组已完成 Lab4 前置 Lab2 / Lab3 依赖恢复及三个 Function 的实现和整合。

Function 1 中主要实现：

```text
page_insert()
page_remove()
```

完成了页面映射建立、替换、删除、引用计数维护以及 TLB 刷新。

同时通过分析 Sv32、Sv39 和 Sv48，进一步理解了 RISC-V 多级页表结构，以及 `get_pte()` 中多级页表遍历代码相似的原因。

Function 2 中主要实现：

```text
alloc_proc()
proc_init()
kernel_thread()
do_fork()
copy_thread()
copy_mm()
```

完成了进程控制块初始化、idleproc 与 initproc 创建、内核栈分配、初始中断帧和上下文设置，以及 PID 分配和进程登记。

同时通过独立 QEMU 创建测试，进一步理解了内核线程首次运行现场的组织方式，以及资源分配失败时回收内核栈和进程控制块的处理过程。

王乐之负责的 Function 3 完成循环调度、页表和上下文切换、idle 调度入口，并结合实际宏分析中断保护。完整构建、默认 QEMU 及原评分测试均已执行，评分为 30/30；独立补充检查覆盖寄存器保存和中断状态边界。当前框架尚未实现正常退出回收及抢占调度。
