# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|---|---|
| **实验名称** | Lab3：中断与异常处理 |
| **小组成员** | 2412771-谭锦睿、2413581-黄奕浩、2411302-王乐之 |
| **完成日期** | 2026-10-xx |
| **实验分支** | `lab3` |
| **代码目录** | `code/` |
| **报告目录** | `report/` |

---

## 小组分工

### 练习分工

| 成员 | 负责内容 |
|---|---|
| **2412771-谭锦睿** | **功能一：Trap 入口与上下文恢复**。负责 `trapentry.S` 中 `RESTORE_ALL` 和 `__alltraps` 的实现，以及对应代码分析。 |
| **2413581-黄奕浩** | **功能二：中断处理**。负责 `trap.c` 中 `interrupt_handler()` 的实现，包括时钟中断及其他中断处理。 |
| **2411302-王乐之** | **功能二：异常处理**。负责 `trap.c` 中 `exception_handler()` 的实现，包括断点异常、非法指令异常及其他异常处理。 |
| **三人共同完成** | 最终代码整合、编译运行、实验测试和报告整理。 |

### 任务覆盖检查

根据实验指导书，本次实验目前需要完成的主要内容如下：

- [ ] 功能一：Trap 入口与上下文恢复 —— 谭锦睿
- [ ] 功能二：中断处理 —— 黄奕浩
- [ ] 功能二：异常处理 —— 王乐之
- [ ] 完成实验代码整合
- [ ] 完成编译与运行验证
- [ ] 完成实验报告
- [ ] 整理实际使用的 Prompt

### 报告分工

| 成员 | 报告负责内容 |
|---|---|
| **2412771-谭锦睿** | Trap 入口、上下文保存与恢复 |
| **2413581-黄奕浩** | 中断处理部分 |
| **2411302-王乐之** | 异常处理部分 |
| **三人共同完成** | 实验结果、测试记录和最终报告整理 |

> 最终报告内容以实际代码、实际运行结果和实际使用的 Prompt 为准，未完成内容不提前填写结果。

---

# 一、实验目的

本实验主要学习 RISC-V 下的中断与异常处理机制，了解处理器发生 Trap 后进入操作系统内核的过程，并完成基本的中断和异常处理功能。

本实验主要目标如下：

1. 理解 Trap、Interrupt 和 Exception 的基本概念及其关系；
2. 理解 RISC-V 中与 Trap 处理有关的控制状态寄存器；
3. 理解 Trap 发生后处理器进入中断处理程序的基本流程；
4. 理解 TrapFrame 及寄存器现场保存与恢复过程；
5. 完成 Trap 入口和上下文恢复代码；
6. 完成中断处理代码；
7. 完成异常处理代码；
8. 通过实际运行验证实现结果。

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
| 实验分支 | `lab3` |

## 2.2 AI 工具

| 成员 | AI 工具 | 用途 |
|---|---|---|
| 2412771-谭锦睿 | ChatGPT / Codex | 实验要求分析、代码理解、实现与检查辅助 |
| 2413581-黄奕浩 | ChatGPT / Codex | 中断处理实现与检查 |
| 2411302-王乐之 | ChatGPT / Codex| 异常处理实现与检查 |

实际使用的 Prompt 统一整理在：

```text
report/prompt.md
```

---

# 三、实验整体逻辑分析

## 3.1 Lab3 主要内容

本实验在前面实验的基础上加入 Trap 处理功能。

主要需要完成的部分位于：

```text
kern/trap/trapentry.S
kern/trap/trap.c
```

其中：

```text
trapentry.S
    ├── SAVE_ALL
    ├── RESTORE_ALL
    ├── __alltraps
    └── __trapret

trap.c
    ├── trap()
    ├── trap_dispatch()
    ├── interrupt_handler()
    └── exception_handler()
```

框架已经提供部分代码，本实验主要补充其中标记的 Lab3 实现部分。

## 3.2 Trap 基本处理流程

从代码结构上看，Trap 的处理过程主要为：

```text
发生 Trap
    ↓
进入 Trap 入口
    ↓
保存当前执行现场
    ↓
构造 TrapFrame
    ↓
进入 trap()
    ↓
trap_dispatch()
    ↓
根据类型处理中断或异常
    ↓
恢复现场
    ↓
返回原来的执行流程
```

具体实现和运行过程在完成代码后进一步分析。

---

# 四、实验内容与实现

## 4.1 功能一：Trap 入口与上下文恢复

**负责人：2412771-谭锦睿**

### 4.1.1 任务要求

本任务主要涉及：

```text
kern/trap/trapentry.S
```

需要完成的代码位置包括：

```text
RESTORE_ALL
__alltraps
```

现有框架已经提供 `SAVE_ALL` 和 Trap 返回的基本结构，需要在此基础上补充上下文恢复以及进入 C 语言 Trap 处理函数的过程。

### 4.1.2 主要涉及文件

```text
kern/trap/trapentry.S
kern/trap/trap.h
kern/trap/trap.c
```

### 4.1.3 实现过程

本任务在老师已经提供的 `SAVE_ALL` 基础上完成 `RESTORE_ALL` 和 `__alltraps`。

首先分析 `SAVE_ALL` 可以看到，发生 Trap 后程序先将原来的 `sp` 保存到 `sscratch`，随后在当前栈上开辟 `36 * REGBYTES` 的空间保存 TrapFrame。在 RV64 环境下 `REGBYTES = 8`，因此 TrapFrame 共占 288 字节。

其中前 32 个槽位对应 `x0 ~ x31`，第 2 个槽位保存 Trap 前原来的 `sp`，后四个槽位分别保存 `sstatus`、`sepc`、`sbadaddr` 和 `scause`。

`RESTORE_ALL` 按照 `SAVE_ALL` 的布局进行反向恢复。首先从 TrapFrame 中读取 `sstatus` 和 `sepc` 并写回对应 CSR，然后依次恢复通用寄存器。`x0` 不需要恢复，因为 RISC-V 的 `x0` 始终为 0。`sp` 必须最后恢复，因为此前所有寄存器仍需要以当前 `sp` 作为 TrapFrame 的访问基址。

`__alltraps` 中先调用已有的 `SAVE_ALL` 保存现场。此时 `sp` 指向当前 TrapFrame，根据 RISC-V 函数调用约定，将 `sp` 复制到 `a0`，作为 `trap(struct trapframe *tf)` 的第一个参数，然后通过 `jal trap` 进入 C 语言 Trap 处理函数。

`trap()` 返回后程序顺序进入 `__trapret`，执行 `RESTORE_ALL` 恢复现场，最后通过 `sret` 返回 Trap 前的执行流程。

完整过程为：

```text
发生 Trap
    ↓
__alltraps
    ↓
SAVE_ALL
    ↓
sp 指向 TrapFrame
    ↓
a0 = sp
    ↓
trap(tf)
    ↓
trap_dispatch(tf)
    ↓
trap() 返回
    ↓
__trapret
    ↓
RESTORE_ALL
    ↓
sret
```

### 4.1.4 关键代码

`RESTORE_ALL` 首先恢复 Trap 返回所需要的 `sstatus` 和 `sepc`：

```asm
LOAD s1, 32*REGBYTES(sp)
LOAD s2, 33*REGBYTES(sp)
csrw sstatus, s1
csrw sepc, s2
```

随后按照 TrapFrame 中的保存位置恢复通用寄存器。其中 `sp` 最后恢复：

```asm
LOAD x1, 1*REGBYTES(sp)
LOAD x3, 3*REGBYTES(sp)
...
LOAD x31, 31*REGBYTES(sp)
LOAD x2, 2*REGBYTES(sp)
```

Trap 入口的主要代码为：

```asm
__alltraps:
    SAVE_ALL

    #lab3 function1 2412771
    move a0, sp
    jal trap
```

`SAVE_ALL` 完成后，`sp` 就是当前 TrapFrame 的地址，因此直接通过 `a0` 将其传给 `trap()`。

### 4.1.5 Prompt

完整 Prompt 见：

```text
report/prompt.md
```

报告中使用的精简 Prompt：

```markdown
[PROMPT]

读取当前 Lab3 真实工程，在 `kern/trap/trapentry.S` 中完成 `RESTORE_ALL` 和 `__alltraps`，实现 Trap 入口、上下文恢复以及对 `trap()` 的调用。

[RELY]

以当前已有的 `SAVE_ALL`、`struct trapframe`、`trap()`、RISC-V ABI 和 `REGBYTES/LOAD/STORE` 为准。

[GUARANTEE]

只修改 `trapentry.S`，不修改 `SAVE_ALL`、`__trapret`、测试代码以及其他组员负责的中断和异常处理函数。

[SPECIFICATION]

严格按照 TrapFrame 布局恢复 `sstatus`、`sepc` 和通用寄存器，最后恢复 `sp`；`__alltraps` 中将当前 TrapFrame 地址通过 `a0` 传给 `trap()`，返回后进入已有的 Trap 返回流程。
```

### 4.1.6 测试与验证

本任务完成后首先对修改范围进行了检查：

```bash
git diff --check
git diff --name-only
```

`git diff --check` 未发现格式问题，`git diff --name-only` 仅显示：

```text
code/kern/trap/trapentry.S
```

说明本任务没有修改其他组员负责的代码。

随后单独编译 Trap 入口汇编文件：

```bash
cd ~/os-lab/nku-os/code
make obj/kern/trap/trapentry.o V=
```

目标文件可以正常生成，说明当前 `trapentry.S` 能够通过汇编和编译。

进一步通过反汇编检查生成的指令，确认：

- `a0` 被设置为当前 TrapFrame 地址；
- 正确生成对 `trap` 的调用；
- `sstatus` 和 `sepc` 被正确写回；
- `sp` 最后恢复；
- 返回流程最终执行 `sret`。

目前执行完整 `make` 时仍会在 `trap.c` 中报：

```text
implicit declaration of function 'interrupt_handler'
implicit declaration of function 'exception_handler'
```

这是因为功能二中的中断处理和异常处理尚未由其他组员完成，与本任务的 `trapentry.S` 编译无关。

因此，目前已经完成了功能一的代码实现、修改范围检查和独立编译验证；完整 QEMU 运行和 Lab3 联调将在功能二代码合入后统一进行。


## 4.2 功能二：中断处理

**负责人：2413581-黄奕浩**

### 4.2.1 任务要求

本任务主要涉及：

```text
kern/trap/trap.c
```

需要完成：

```c
interrupt_handler(struct trapframe *tf)
```

根据 Trap 原因处理对应的中断。

其中包括实验中使用到的时钟中断以及其他中断情况。

### 4.2.2 主要涉及文件

```text
kern/trap/trap.c
kern/trap/trap.h
kern/driver/clock.c
kern/driver/clock.h
```

### 4.2.3 实现过程

[完成后填写]

### 4.2.4 关键代码

[完成后填写]

### 4.2.5 Prompt

完整 Prompt 见：

```text
report/prompt.md
```

报告中使用的精简 Prompt：

```markdown
[PROMPT]

[完成后填写]

[RELY]

[完成后填写]

[GUARANTEE]

[完成后填写]

[SPECIFICATION]

[完成后填写]
```

### 4.2.6 测试与验证

[完成后填写]

---

## 4.3 功能二：异常处理

**负责人：2411302-王乐之**

### 4.3.1 任务要求

本任务主要涉及：

```text
kern/trap/trap.c
```

需要完成：

```c
exception_handler(struct trapframe *tf)
```

根据 Trap 原因处理对应的异常。

实验中主要涉及断点异常、非法指令异常以及其他异常情况。

### 4.3.2 主要涉及文件

```text
kern/trap/trap.c
kern/trap/trap.h
```

### 4.3.3 实现过程

[完成后填写]

### 4.3.4 关键代码

[完成后填写]

### 4.3.5 Prompt

完整 Prompt 见：

```text
report/prompt.md
```

报告中使用的精简 Prompt：

```markdown
[PROMPT]

[完成后填写]

[RELY]

[完成后填写]

[GUARANTEE]

[完成后填写]

[SPECIFICATION]

[完成后填写]
```

### 4.3.6 测试与验证

[完成后填写]

---

# 五、关键知识理解

## 5.1 Trap、Interrupt 与 Exception

[实验过程中结合代码理解后填写]

## 5.2 TrapFrame

[实验过程中结合代码理解后填写]

## 5.3 与 Trap 有关的 CSR

[实验过程中结合代码理解后填写]

## 5.4 Trap 入口和返回过程

[实验过程中结合代码理解后填写]

## 5.5 时钟中断

[实验过程中结合代码理解后填写]

---

# 六、测试与验证

## 6.1 编译验证

执行：

```bash
make clean
make
```

实际结果：

```text
[完成后填写]
```

## 6.2 QEMU 运行验证

执行：

```bash
make qemu
```

实际结果：

```text
[完成后填写]
```

## 6.3 实验测试

执行命令：

```bash
[完成后填写]
```

实际结果：

```text
[完成后填写]
```

---

# 七、实验过程中遇到的问题



# 八、实验结果

[全部功能完成并实际验证后填写]

---

# 九、实验总结

[实验完成后填写]