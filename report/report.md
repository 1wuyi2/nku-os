# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|---|---|
| 实验名称 | Lab4：进程管理 |
| 小组成员 | 2412771-谭锦睿、2413581-黄奕浩、2411302-王乐之 |
| 完成日期 | 2026-11-XX |
| 实验分支 | `lab4` |
| 代码目录 | `code/` |
| 报告目录 | `report/` |

---

## 小组分工

### 练习分工

| 成员 | 负责内容 |
|---|---|
| 2412771-谭锦睿 | Function 1：页面映射相关操作。负责 `page_insert()`、`page_remove()` 及其相关页表、页引用计数、TLB 刷新逻辑的实现和分析。 |
| 2413581-黄奕浩 | Function 2：进程 / 内核线程创建。负责进程控制块初始化、内核栈分配、`do_fork()`、`copy_thread()` 等进程创建相关代码的实现和分析。 |
| 2411302-王乐之 | Function 3：进程调度与上下文切换。负责 `schedule()`、`proc_run()`、`switch_to()` 以及相关调度和上下文保存恢复逻辑的实现和分析。 |
| 三人共同完成 | 最终代码整合、编译运行、QEMU 测试、官方测试以及实验报告整理。 |

### 拓展练习分工

| 成员 | 拓展练习 |
|---|---|
| 2412771-谭锦睿 | Challenge 2：深入理解不同分页模式的工作原理。结合 Sv32、Sv39、Sv48 的区别，分析 `get_pte()` 中两段形式相似代码的原因；讨论当前 `get_pte()` 将页表项查找和页表页分配合并在一个函数中的设计是否合理，以及是否有必要拆分。 |
| 2411302-王乐之 | Challenge 1：说明 `local_intr_save(intr_flag); ... local_intr_restore(intr_flag);` 如何实现中断状态的保存、关闭和恢复。 |
| 2413581-黄奕浩 | 无单独 Challenge，负责 Function 2：进程 / 内核线程创建部分及对应实验报告。 |

### 任务完成情况

- [ ] Function 1：完成 `page_insert()` / `page_remove()`
- [ ] Function 2：完成进程 / 内核线程创建相关功能
- [ ] Function 3：完成调度与上下文切换
- [ ] 完成三个 Function 的代码整合
- [ ] 完成编译
- [ ] 完成 QEMU 运行验证
- [ ] 完成官方测试
- [ ] 完成拓展练习分析
- [ ] 完成实验报告
- [ ] 整理实际使用的 Prompt

### 报告分工

| 成员 | 报告负责内容 |
|---|---|
| 2412771-谭锦睿 | 页面映射、`page_insert()`、`page_remove()` 以及页表相关拓展练习 |
| 2413581-黄奕浩 | 进程控制块、进程创建、`do_fork()`、`copy_thread()` 及新线程首次运行流程 |
| 2411302-王乐之 | `schedule()`、`proc_run()`、`switch_to()`、中断保护及上下文切换 |
| 三人共同完成 | 整体流程分析、实验结果、测试记录、问题分析和最终报告整理 |

实际使用的 Prompt 统一整理在：

```text
report/prompt.md
```

---

# 一、实验目的

本实验主要学习操作系统中的进程管理机制，在前面实验已经实现的物理内存管理、页表和 Trap 机制基础上，进一步加入内核线程、进程控制块、调度以及上下文切换等功能。

通过本实验，需要理解操作系统如何表示一个进程、如何创建新的执行实体、如何保存和恢复进程执行状态，以及如何通过调度机制让不同进程共享 CPU。

主要目标如下：

1. 理解进程和内核线程的基本概念；
2. 理解 `proc_struct` 进程控制块的结构和作用；
3. 理解页面映射建立、替换和删除的过程；
4. 理解物理页引用计数的作用；
5. 理解 `do_fork()` 创建新进程的整体流程；
6. 理解 `copy_thread()` 如何构造新线程第一次运行所需的执行现场；
7. 理解进程从创建到进入可运行状态的过程；
8. 理解 `schedule()` 的基本调度过程；
9. 理解 `proc_run()` 和 `switch_to()` 的作用；
10. 理解上下文切换过程中寄存器保存与恢复机制；
11. 理解调度过程中关闭和恢复中断的原因；
12. 通过编译、QEMU 运行以及测试验证实验实现。

---

# 二、实验环境

## 2.1 系统与开发环境

| 项目 | 环境 |
|---|---|
| 主机环境 | Windows + WSL2 Ubuntu 24.04 |
| 目标架构 | RISC-V 64 |
| 交叉编译工具链 | `riscv64-unknown-elf-*` |
| 虚拟机 | QEMU |
| 固件 | OpenSBI |
| 调试工具 | RISC-V GDB |
| 版本管理 | Git |
| 实验分支 | `lab4` |

## 2.2 AI 工具

| 成员 | AI 工具 | 用途 |
|---|---|---|
| 2412771-谭锦睿 | ChatGPT / Codex | 实验要求分析、页表和页面映射代码理解、实现与检查 |
| 2413581-黄奕浩 | ChatGPT / Codex | 进程创建代码理解、实现与调用关系分析 |
| 2411302-王乐之 | ChatGPT / Codex | 调度、上下文切换代码理解与检查 |

完整 Prompt 见：

```text
report/prompt.md
```

---

# 三、实验整体逻辑分析

## 3.1 Lab4 与前面实验的关系

Lab4 并不是从头建立一个新的子系统，而是在前面实验提供的机制上继续加入进程管理。

整体关系可以表示为：

```text
Lab1
最小可执行内核
    ↓
Lab2
物理内存管理 + 页表
    ↓
Lab3
Trap + 中断 + 异常
    ↓
Lab4
进程管理 + 调度 + 上下文切换
```

Lab4 中的进程管理需要同时依赖：

```text
物理内存管理
      +
虚拟内存与页表
      +
Trap 机制
      +
内核栈
      +
进程上下文
      +
调度机制
```

因此 Lab4 将前面较为独立的内核机制进一步组合起来。

---

## 3.2 Lab4 主要内容

本实验可以大致分为三个部分：

```text
Function 1
页面映射管理
page_insert / page_remove

        ↓

Function 2
进程和内核线程创建
do_fork / copy_thread

        ↓

Function 3
调度和上下文切换
schedule / proc_run / switch_to
```

三部分之间并不是完全独立的。

新的进程首先需要正确的内存管理和运行环境，然后由进程创建函数建立 PCB、内核栈以及执行上下文，最后进入可运行状态，由调度器决定什么时候真正获得 CPU。

---

## 3.3 从创建到运行的整体流程

一个新线程并不是创建之后立即开始执行。

大致流程为：

```text
创建线程请求
    ↓
do_fork()
    ↓
分配并初始化 proc_struct
    ↓
分配内核栈
    ↓
建立内存管理信息
    ↓
copy_thread()
    ↓
建立 TrapFrame 和 Context
    ↓
分配 PID
    ↓
加入进程管理结构
    ↓
设置为 PROC_RUNNABLE
    ↓
等待调度
    ↓
schedule()
    ↓
proc_run()
    ↓
switch_to()
    ↓
新线程第一次获得 CPU
```

因此：

```text
创建进程 ≠ 进程已经运行
```

创建阶段只是使进程具备能够被操作系统调度的条件。

---

# 四、Function 1：页面映射管理

**负责人：2412771-谭锦睿**

## 4.1 任务要求

本部分主要完成页面映射相关操作，重点分析：

```c
page_insert()
page_remove()
```

需要理解一个物理页如何被插入到指定虚拟地址的页表映射中，以及删除映射时引用计数、页表项和 TLB 应如何处理。

---

## 4.2 主要涉及文件

主要涉及：

```text
kern/mm/pmm.c
kern/mm/pmm.h
libs/riscv.h
```

具体以实验代码实际位置为准。

---

## 4.3 相关基础

页面映射建立过程的核心关系为：

```text
虚拟地址 la
    ↓
页表查询
    ↓
PTE
    ↓
物理页 Page
```

页表项不仅记录物理页地址，还包含：

```text
PTE_V
PTE_R
PTE_W
PTE_X
PTE_U
```

等权限和有效位。

此外，一个物理页可能同时被多个映射引用，因此需要：

```text
page_ref(page)
```

记录当前页面被引用的次数。

---

## 4.4 page_insert()

### 4.4.1 主要作用

`page_insert()` 的作用是：

> 将物理页 `page` 映射到页表 `pgdir` 中虚拟地址 `la` 对应的位置。

整体过程可以概括为：

```text
page_insert()
    ↓
get_pte(pgdir, la, 1)
    ↓
找到 / 创建对应 PTE
    ↓
增加新页面引用计数
    ↓
检查原映射
    ↓
必要时删除旧映射
    ↓
写入新的 PTE
    ↓
刷新 TLB
```

### 4.4.2 需要注意的问题

如果该虚拟地址原本没有映射，则可以直接建立新映射。

如果该地址已经映射到其他物理页，则需要先删除旧映射并正确处理旧页面的引用计数。

如果原本映射的就是同一个物理页，则需要避免引用计数出现错误。

TODO：完成代码后结合实际实现补充。

---

## 4.5 page_remove()

### 4.5.1 主要作用

`page_remove()` 用于删除指定虚拟地址对应的页面映射。

总体过程：

```text
page_remove()
    ↓
get_pte()
    ↓
查找对应 PTE
    ↓
判断是否存在有效映射
    ↓
找到物理页
    ↓
减少引用计数
    ↓
必要时释放页面
    ↓
清除 PTE
    ↓
刷新 TLB
```

### 4.5.2 为什么需要刷新 TLB

CPU 访问虚拟地址时，并不一定每次都重新查询页表，而可能直接使用 TLB 中缓存的地址转换结果。

如果页表已经改变，但 TLB 中仍然保存旧映射，则可能继续访问旧物理地址。

因此删除或修改页表映射后，需要进行对应的 TLB 刷新操作。

---

## 4.6 关键代码

TODO：实现后填写。

```c
// page_insert()

TODO
```

```c
// page_remove()

TODO
```

---

## 4.7 Function 1 调用关系

```text
page_insert
    ↓
get_pte
    ↓
page_ref_inc
    ↓
page_remove_pte（必要时）
    ↓
写入新 PTE
    ↓
tlb_invalidate
```

删除过程：

```text
page_remove
    ↓
get_pte
    ↓
page_remove_pte
    ↓
page_ref_dec
    ↓
free_page（引用计数为 0）
    ↓
清除 PTE
    ↓
tlb_invalidate
```

具体以实验代码为准。

---

## 4.8 拓展练习一：Sv32、Sv39、Sv48 的区别

RISC-V 根据不同地址宽度支持不同的页表模式。

### Sv32

主要用于 32 位 RISC-V。

采用：

```text
两级页表
```

虚拟地址大致划分为：

```text
VPN[1] | VPN[0] | page offset
```

### Sv39

RV64 中常用的分页模式。

采用：

```text
三级页表
```

虚拟地址划分为：

```text
VPN[2] | VPN[1] | VPN[0] | page offset
```

每级 VPN 为 9 位，页内偏移为 12 位。

因此：

```text
9 + 9 + 9 + 12 = 39 bit
```

### Sv48

采用：

```text
四级页表
```

比 Sv39 再增加一级 VPN。

虚拟地址结构：

```text
VPN[3] | VPN[2] | VPN[1] | VPN[0] | page offset
```

---

## 4.9 拓展练习二：为什么 get_pte() 中两段代码很相似

`get_pte()` 需要逐级查询多级页表。

例如 Sv39 需要：

```text
一级页表
    ↓
二级页表
    ↓
三级页表
```

对于每一级中间页表，处理步骤实际上都类似：

```text
取当前页表项
    ↓
判断是否有效
    ↓
若不存在且 create = 1
    ↓
分配新的页表页
    ↓
初始化
    ↓
写入上一级页表项
    ↓
进入下一级页表
```

因此代码中处理不同级页表的两部分逻辑会高度相似。

这种重复本质上来源于：

> 多级页表的每一级使用相同的数据结构和类似的查询规则。

---

## 4.10 拓展练习三：查找和分配是否应该拆开

当前 `get_pte()` 一般通过 `create` 参数同时支持：

```text
只查找
```

和：

```text
不存在时创建页表
```

优点是接口统一：

```c
get_pte(pgdir, la, 0);
```

表示只查找。

```c
get_pte(pgdir, la, 1);
```

表示必要时创建。

但它也使函数同时承担：

```text
查询
+
内存分配
+
页表修改
```

多个职责。

因此从软件设计角度，也可以拆分成类似：

```text
find_pte()
```

和：

```text
create_pte()
```

两个接口。

拆分后的代码职责更加明确，但会增加接口数量和部分重复代码。

本实验框架使用 `create` 参数属于一种较简洁的实现方式。

---

## 4.11 测试与验证

TODO：Function 1 完成后填写。

编译：

```bash
cd ~/os-lab/nku-os/code

make
```

检查：

```bash
git diff --check
```

运行：

```bash
make qemu
```

测试结果：

```text
TODO
```

---

# 五、Function 2：进程与内核线程创建

**负责人：2413581-黄奕浩**

## 5.1 任务要求

本部分主要分析并实现进程 / 内核线程的创建过程。

重点函数包括：

```text
alloc_proc()
do_fork()
copy_thread()
kernel_thread()
```

需要理解操作系统如何从零建立一个能够被调度运行的新进程。

---

## 5.2 主要涉及文件

主要涉及：

```text
kern/process/proc.c
kern/process/proc.h
```

同时与：

```text
kern/mm/
kern/trap/
```

中的相关数据结构存在联系。

---

## 5.3 proc_struct

`proc_struct` 是 ucore 中用于描述一个进程的核心数据结构，相当于操作系统理论中的 PCB。

主要成员包括：

```text
state
pid
runs
kstack
need_resched
parent
mm
context
tf
cr3
flags
name
list_link
hash_link
```

---

## 5.4 主要成员作用

### state

表示当前进程状态。

例如：

```text
PROC_UNINIT
PROC_SLEEPING
PROC_RUNNABLE
PROC_ZOMBIE
```

### pid

进程唯一编号。

### runs

记录进程被调度运行的次数。

### kstack

该进程的内核栈地址。

### need_resched

表示当前进程是否需要重新调度。

### parent

指向父进程。

### mm

指向进程的内存管理结构。

对于部分内核线程：

```text
mm = NULL
```

### context

保存进程进行上下文切换时需要恢复的寄存器。

### tf

指向该进程内核栈中的 TrapFrame。

### cr3

保存页表相关信息。

---

## 5.5 alloc_proc()

`alloc_proc()` 的主要作用是：

> 分配一个新的 `proc_struct` 并初始化其中各字段。

新申请的进程控制块最开始还不是可运行进程。

整体流程：

```text
kmalloc(sizeof(struct proc_struct))
    ↓
初始化字段
    ↓
state = PROC_UNINIT
    ↓
pid = -1
    ↓
初始化 context / tf / mm 等成员
```

TODO：结合实际代码填写最终初始化值。

---

## 5.6 do_fork()

`do_fork()` 是进程创建过程中的核心函数。

总体过程可以概括为：

```text
do_fork()
    ↓
检查进程数量
    ↓
alloc_proc()
    ↓
建立父子关系
    ↓
setup_kstack()
    ↓
copy_mm()
    ↓
copy_thread()
    ↓
get_pid()
    ↓
加入进程链表 / 哈希表
    ↓
wakeup_proc()
    ↓
返回新进程 PID
```

---

## 5.7 setup_kstack()

每个内核线程需要独立的内核栈。

因此创建进程时需要为其分配：

```text
KSTACKPAGE
```

个页面作为内核栈。

内核栈主要用于：

```text
函数调用
TrapFrame
异常和中断处理
内核态执行
```

---

## 5.8 copy_thread()

`copy_thread()` 负责构造新线程第一次被调度运行时需要的执行现场。

其中需要设置：

```text
TrapFrame
Context
```

典型关系：

```text
proc->tf
    ↓
新线程的 TrapFrame

proc->context
    ↓
第一次 switch_to 后应该从哪里继续执行
```

因此 `copy_thread()` 是：

> “创建一个进程”

和

> “这个进程以后如何第一次开始运行”

之间非常关键的连接点。

TODO：结合实际代码分析各寄存器设置。

---

## 5.9 Context 与 TrapFrame 的区别

两者都保存处理器状态，但用途不同。

### TrapFrame

主要用于：

```text
Trap / Interrupt / Exception
```

发生时保存完整现场。

来自 Lab3。

### Context

主要用于：

```text
进程上下文切换
```

只需要保存按照调用约定必须跨函数调用保持的寄存器以及：

```text
ra
sp
```

等关键寄存器。

因此：

```text
TrapFrame
    ↓
Trap 上下文

Context
    ↓
进程调度上下文
```

---

## 5.10 kernel_thread()

`kernel_thread()` 用于创建一个内核线程。

其核心思想不是直接调用目标函数，而是先构造一个特殊的 TrapFrame，使新进程第一次运行时进入指定的内核线程入口。

流程：

```text
kernel_thread(fn, arg)
    ↓
构造临时 TrapFrame
    ↓
do_fork()
    ↓
copy_thread()
    ↓
建立新进程执行上下文
```

---

## 5.11 idleproc 与 initproc

系统初始化进程管理模块后，会出现两个重要进程。

### idleproc

`idleproc` 对应系统最初的空闲进程。

它的作用主要是：

```text
当没有其他进程需要运行时
保持 CPU 有一个合法执行主体
```

同时：

```text
current = idleproc
```

使系统从一开始就具有合法的当前进程。

### initproc

`initproc` 是通过内核线程创建机制建立的第一个真正的新进程。

因此：

```text
idleproc
    ↓
系统启动时人工建立

initproc
    ↓
通过 kernel_thread / do_fork 创建
```

---

## 5.12 拓展练习：新线程第一次运行的完整链路

一个新内核线程第一次开始运行的大致过程：

```text
kernel_thread()
    ↓
do_fork()
    ↓
alloc_proc()
    ↓
setup_kstack()
    ↓
copy_thread()
    ↓
建立 context
    ↓
进程进入 PROC_RUNNABLE
    ↓
schedule()
    ↓
选中新进程
    ↓
proc_run()
    ↓
switch_to()
    ↓
恢复新进程 context
    ↓
进入 forkret
    ↓
利用 TrapFrame 恢复执行现场
    ↓
进入 kernel_thread_entry
    ↓
执行目标函数 fn(arg)
```

这一流程说明：

> 新线程第一次运行并不是直接由 `do_fork()` 调用目标函数，而是通过人为构造 Context 和 TrapFrame，使调度器第一次切换到该进程时自然进入指定入口。

---

## 5.13 测试与验证

TODO

```bash
make
make qemu
```

测试结果：

```text
TODO
```

---

# 六、Function 3：进程调度与上下文切换

**负责人：2411302-王乐之**

## 6.1 任务要求

本部分主要实现和分析：

```text
schedule()
proc_run()
switch_to()
```

理解操作系统如何：

```text
选择下一个进程
      ↓
切换地址空间
      ↓
切换 CPU 寄存器上下文
```

---

## 6.2 主要涉及文件

主要涉及：

```text
kern/process/proc.c
kern/schedule/
kern/process/switch.S
```

具体文件以实验代码为准。

---

## 6.3 schedule()

`schedule()` 的核心任务是：

> 从可运行进程中选择一个进程作为下一个运行对象。

因此调度本身解决的是：

```text
接下来运行谁？
```

而不是：

```text
具体如何切换 CPU 状态？
```

---

## 6.4 基本流程

Lab4 中的调度过程可以概括为：

```text
schedule()
    ↓
关闭中断
    ↓
current->need_resched = 0
    ↓
寻找 PROC_RUNNABLE 进程
    ↓
选择 next
    ↓
如果没有则选择 idleproc
    ↓
next->runs++
    ↓
proc_run(next)
    ↓
恢复中断
```

TODO：结合实际代码补充当前具体遍历方式。

---

## 6.5 proc_run()

`proc_run()` 负责执行实际的进程切换准备。

大致过程：

```text
proc_run(next)
    ↓
判断 next != current
    ↓
保存当前中断状态
    ↓
关闭中断
    ↓
prev = current
    ↓
current = next
    ↓
切换页表 / 地址空间
    ↓
switch_to(&prev->context, &next->context)
    ↓
恢复中断状态
```

因此：

```text
schedule
```

负责“选择谁”，而：

```text
proc_run
```

负责“切过去”。

---

## 6.6 switch_to()

`switch_to()` 是进程上下文切换的最低层实现。

其核心过程是：

```text
保存 prev 的寄存器
        ↓
恢复 next 的寄存器
        ↓
CPU 从 next 上次停止的位置继续执行
```

在 RISC-V 中，按照函数调用约定，主要需要保存：

```text
ra
sp
s0 ~ s11
```

等需要跨函数调用保持的寄存器。

---

## 6.7 为什么不需要保存全部寄存器

上下文切换由普通函数调用过程触发。

按照 RISC-V ABI：

```text
caller-saved
```

寄存器本身就允许被被调用函数修改。

而：

```text
callee-saved
```

寄存器必须在函数调用前后保持不变。

因此 `switch_to()` 只需要保存真正需要跨调度保持的寄存器。

---

## 6.8 schedule 和 switch_to 的区别

```text
schedule()
```

解决：

> 应该运行哪个进程？

```text
proc_run()
```

解决：

> 如何准备切换到这个进程？

```text
switch_to()
```

解决：

> CPU 寄存器如何真正从旧进程切换到新进程？

因此：

```text
schedule
    ↓
策略

proc_run
    ↓
进程切换控制

switch_to
    ↓
机器级上下文切换
```

---

## 6.9 为什么调度时要关闭中断

进程调度过程中会修改：

```text
current
进程状态
Context
地址空间
CPU 寄存器
```

如果执行到一半突然产生中断，内核可能看到一个不一致的状态。

例如：

```text
current 已经指向 next
```

但是：

```text
CPU 仍然在执行 prev 的上下文
```

此时如果中断处理代码再次访问 `current`，就可能发生错误。

因此调度和上下文切换中的关键操作需要作为一个临界区执行。

---

## 6.10 local_intr_save() / local_intr_restore()

典型使用：

```c
bool intr_flag;

local_intr_save(intr_flag);

/* 临界区 */

local_intr_restore(intr_flag);
```

`local_intr_save()` 不只是简单关闭中断。

它首先：

```text
读取当前中断状态
```

然后：

```text
如果原本开启
    ↓
关闭中断
```

并保存之前的状态。

之后：

```text
local_intr_restore()
```

根据之前保存的状态决定是否重新开启中断。

---

## 6.11 为什么不能简单 intr_disable() + intr_enable()

假设进入临界区前：

```text
中断本来就是关闭的
```

如果直接：

```c
intr_disable();

/* ... */

intr_enable();
```

离开临界区时就会错误地把原本关闭的中断重新打开。

因此正确做法是：

```text
记住原状态
    ↓
关闭中断
    ↓
执行临界区
    ↓
恢复原状态
```

而不是：

```text
无条件关闭
    ↓
无条件打开
```

---

## 6.12 拓展练习：调度中的临界区

调度过程中最重要的共享状态之一是：

```text
current
```

除此之外还有：

```text
进程链表
进程状态
need_resched
Context
页表
```

这些状态在切换过程中必须保持一致。

因此：

```text
local_intr_save()
      ↓
禁止当前 CPU 被中断打断
      ↓
完成关键状态修改
      ↓
switch_to()
      ↓
local_intr_restore()
```

保证切换过程不会被异步中断破坏。

---

## 6.13 测试与验证

TODO

```bash
make
make qemu
```

测试结果：

```text
TODO
```

---

# 七、Lab4 整体运行流程

完成三个 Function 后，可以从系统启动角度重新理解 Lab4。

```text
OpenSBI
    ↓
kern_entry
    ↓
kern_init()
    ↓
物理内存初始化
    ↓
虚拟内存初始化
    ↓
Trap 初始化
    ↓
进程系统初始化
    ↓
proc_init()
    ↓
建立 idleproc
    ↓
kernel_thread()
    ↓
创建 initproc
    ↓
idleproc 运行
    ↓
schedule()
    ↓
选择 initproc
    ↓
proc_run()
    ↓
switch_to()
    ↓
initproc 第一次运行
```

Lab4 的关键就在于：

> 让内核从“只有一条固定执行流”发展为“能够管理多个独立执行实体，并在它们之间进行切换”。

---

# 八、关键数据结构总结

## 8.1 struct Page

表示一个物理页。

重点字段与操作：

```text
ref
flags
page_link
```

主要用于：

```text
物理页面管理
引用计数
空闲链表管理
```

---

## 8.2 页表项 PTE

页表项负责建立：

```text
虚拟页
    ↓
物理页
```

之间的映射。

同时记录访问权限。

---

## 8.3 proc_struct

对应操作系统理论中的：

```text
PCB
```

保存一个进程的：

```text
状态
PID
内核栈
内存空间
上下文
TrapFrame
调度信息
父进程关系
```

---

## 8.4 context

用于进程上下文切换。

保存：

```text
ra
sp
s0 ~ s11
```

等需要保留的寄存器。

---

## 8.5 trapframe

用于保存发生 Trap 时的完整 CPU 状态。

同时也可以被用于：

> 人为构造新线程第一次运行时的执行现场。

这是 Lab3 和 Lab4 之间非常重要的连接。

---

# 九、进程状态

本实验中主要涉及：

```text
PROC_UNINIT
PROC_SLEEPING
PROC_RUNNABLE
PROC_ZOMBIE
```

其中 Lab4 最关键的是：

```text
PROC_UNINIT
      ↓
创建完成
      ↓
PROC_RUNNABLE
      ↓
被 schedule 选中
      ↓
运行
```

后续实验还会继续完善：

```text
睡眠
退出
等待
回收
```

等生命周期状态。

---

# 十、三个 Function 之间的关系

本实验三个部分看似独立，实际上组成了一条完整链路。

```text
Function 1
内存和页面映射
      ↓
为进程提供正确的地址转换基础

Function 2
进程创建
      ↓
建立 PCB、内核栈和执行上下文

Function 3
调度和上下文切换
      ↓
让进程真正获得 CPU
```

最终形成：

```text
内存资源
    ↓
创建进程
    ↓
建立执行现场
    ↓
进入可运行状态
    ↓
调度
    ↓
上下文切换
    ↓
运行
```

---

# 十一、实验测试

## 11.1 编译测试

```bash
cd ~/os-lab/nku-os/code

make clean
make
```

结果：

```text
TODO
```

---

## 11.2 QEMU 运行测试

```bash
make qemu
```

结果：

```text
TODO
```

---

## 11.3 官方测试

按照实验指导书执行对应测试。

```bash
TODO
```

结果：

```text
TODO
```

---

## 11.4 Git 修改检查

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

# 十二、实验结果

## Function 1

TODO

应包括：

```text
page_insert()
page_remove()
页引用计数
TLB 刷新
测试结果
```

## Function 2

TODO

应包括：

```text
proc_struct 初始化
do_fork()
copy_thread()
内核线程创建
测试结果
```

## Function 3

TODO

应包括：

```text
schedule()
proc_run()
switch_to()
上下文切换
测试结果
```

## 整体运行结果

TODO

---

# 十三、实验中遇到的问题及解决方法

## 问题一

现象：

```text
TODO
```

原因：

```text
TODO
```

解决：

```text
TODO
```

---

## 问题二

现象：

```text
TODO
```

原因：

```text
TODO
```

解决：

```text
TODO
```

---

## 问题三

现象：

```text
TODO
```

原因：

```text
TODO
```

解决：

```text
TODO
```

---

# 十四、实验重要知识点总结

## 14.1 页表映射不仅是写一个 PTE

建立一个页面映射时，还需要同时处理：

```text
页表项
页面引用计数
原有映射
页面释放
TLB
```

因此 `page_insert()` 和 `page_remove()` 实际体现的是一套完整的页面映射生命周期管理。

---

## 14.2 PCB 是操作系统管理进程的核心

操作系统不能直接管理一个抽象的“程序”。

它需要将进程运行相关信息保存在：

```text
proc_struct
```

中。

因此：

```text
进程
    ↓
在内核中的表示
    ↓
PCB / proc_struct
```

---

## 14.3 创建进程和运行进程是两个不同阶段

创建阶段：

```text
分配 PCB
分配内核栈
建立内存信息
建立 Context
建立 TrapFrame
```

运行阶段：

```text
schedule
proc_run
switch_to
```

因此：

```text
do_fork()
    ↓
只是创建完成

schedule()
    ↓
决定什么时候真正运行
```

---

## 14.4 Context 和 TrapFrame 作用不同

```text
Context
    ↓
进程切换

TrapFrame
    ↓
Trap 保存 / 恢复
```

但对于新进程第一次运行，两者会配合工作。

---

## 14.5 调度和上下文切换不是同一个概念

调度：

```text
选择谁运行
```

上下文切换：

```text
真正把 CPU 从一个进程切到另一个进程
```

因此：

```text
schedule()
    ↓
proc_run()
    ↓
switch_to()
```

是三个不同层次。

---

## 14.6 local_intr_save / restore 保存的是“原来的状态”

它们并不是单纯地：

```text
关中断
开中断
```

而是：

```text
保存原中断状态
    ↓
关闭中断
    ↓
执行临界区
    ↓
恢复之前的中断状态
```

这样可以正确支持嵌套的临界区。

---

# 十五、Lab2、Lab3 与 Lab4 的联系

Lab4 中可以明显看到之前实验的机制重新被使用。

```text
Lab2
Page / PTE / 页表
    ↓
Lab4 Function 1 页面映射

Lab3
TrapFrame / 中断保护
    ↓
Lab4 新线程首次运行
    ↓
Lab4 调度临界区

Lab4
proc_struct / schedule / switch_to
    ↓
形成真正的多执行流内核
```

因此 Lab4 是前几个实验第一次明显汇合的阶段。

---

# 十六、实验总结

本实验在前面物理内存、虚拟内存以及 Trap 机制的基础上，实现了基本的进程管理框架。

实验首先进一步完善页面映射相关操作，通过 `page_insert()` 和 `page_remove()` 管理虚拟地址和物理页面之间的映射关系。

随后通过 `proc_struct`、`do_fork()`、`copy_thread()` 等机制建立新的进程或内核线程，并为新进程分配内核栈、构造执行上下文，使其具备被调度运行的条件。

最后通过 `schedule()`、`proc_run()` 和 `switch_to()` 完成进程选择和上下文切换，使 CPU 能够在不同执行实体之间切换。

整个 Lab4 的核心过程可以概括为：

```text
页面与地址空间
      ↓
建立 PCB
      ↓
创建进程
      ↓
建立 Context / TrapFrame
      ↓
PROC_RUNNABLE
      ↓
schedule()
      ↓
proc_run()
      ↓
switch_to()
      ↓
进程运行
```

通过本实验，可以更加直观地理解课堂中进程控制块、进程创建、调度、上下文切换、临界区和中断保护等概念如何真正落实到操作系统内核代码中。