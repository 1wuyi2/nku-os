# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|---|---|
| **实验名称** | Lab3：中断与异常处理 |
| **小组成员** | 2412771-谭锦睿、2413581-黄奕浩、2411302-王乐之 |
| **完成日期** | 2026-11-06 |
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

- [x] 功能一：Trap 入口与上下文恢复 —— 谭锦睿
- [x] 功能二：中断处理 —— 黄奕浩（本轮整体联调通过）
- [x] 功能二：异常处理 —— 王乐之
- [x] 完成实验代码整合
- [x] 完成编译与运行验证
- [x] 完成实验报告
- [x] 整理实际使用的 Prompt

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
| 2411302-王乐之 | ChatGPT / Codex | 异常处理实现与检查 |

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

功能一独立验证当时，完整 `make` 在 `trap.c` 中报：

```text
implicit declaration of function 'interrupt_handler'
implicit declaration of function 'exception_handler'
```


因此，目前已经完成了功能一的代码实现、修改范围检查和独立编译验证；完整 QEMU 运行和 Lab3 联调将在功能二代码合入后统一进行。


## 4.2 功能二：中断处理

**负责人：2413581-黄奕浩**

### 4.2.1 任务要求与规格

实现 `kern/trap/trap.c` 中的 `void interrupt_handler(struct trapframe *tf)`。输入是汇编入口保存的 TrapFrame；处理完成后交回统一恢复流程。中断处理不跳过指令，不修改 `tf->epc`、寄存器现场或其他组员的异常处理逻辑。

时钟中断必须重新设置下一次事件，每次仅增加一次 `ticks`，每累计 `TICK_NUM` 次打印统计，累计打印达到 `SHUTDOWN_AFTER` 后调用 SBI 关机。其他已知中断按类型提示；未知原因输出原因号及完整 TrapFrame。中断原因比较前应去掉 RV64 `scause` 的最高中断标志位。

### 4.2.2 主要涉及文件和调用关系

修改文件为 `kern/trap/trap.c`，依赖 `trap.h` 的现场布局、`libs/riscv.h` 的 IRQ 编码、`kern/driver/clock.c` 的 `ticks` 与 `clock_set_next_event()`、`libs/sbi.h` 的 `sbi_shutdown()`。保留 `trapentry.S`、`SAVE_ALL`、`RESTORE_ALL` 和框架测试。

调用链为 `__alltraps → SAVE_ALL → trap(tf) → trap_dispatch(tf) → interrupt_handler(tf) → __trapret → RESTORE_ALL → sret`。`trap_dispatch()` 已通过原因最高位区分中断与异常；本函数清除该位后用 `switch` 区分中断编号。

### 4.2.3 实现过程

处理 `IRQ_S_TIMER` 时首先调用 `clock_set_next_event()`，再执行 `ticks++`。定时器接口设置的是一次性未来事件，不重新设置就无法持续收到后续时钟中断。`clock.c` 使用 `rdtime` 读取时间计数，在当前 QEMU 的时间基准下，100000 个计数约为 10 ms。

沿用框架的 `TICK_NUM=100`，新增 `SHUTDOWN_AFTER=10`。前者是每次打印对应的中断数量，后者是打印次数上限，因此累计1000次中断、打印十行后关机。10是本组在当前实现中选择的参数；它允许框架的200次中断检查先完成。

其他分支覆盖 U/S/M 软件中断、U/M 定时器中断和 U/S/M 外部中断，输出对应提示。未识别原因打印 `Unknown interrupt: cause ...` 并调用 `print_trapframe(tf)`。

### 4.2.4 关键代码

```c
#define TICK_NUM 100
#define SHUTDOWN_AFTER 10
```

```c
uintptr_t cause = tf->cause & ~((uintptr_t)1 << 63);
```

```c
case IRQ_S_TIMER:
    clock_set_next_event();
    ticks++;
    if (ticks % TICK_NUM == 0) {
        print_ticks();
        if (ticks / TICK_NUM >= SHUTDOWN_AFTER) {
            sbi_shutdown();
        }
    }
    break;
```

```c
default:
    cprintf("Unknown interrupt: cause %lu\n", (unsigned long)cause);
    print_trapframe(tf);
    break;
```


### 4.2.5 Prompt 与 spec 的设计

本任务采用连续对话逐步明确和验证需求，提示词整理到 `prompt.md` 的相关部分，包含任务、负责人、仓库与文件范围，以及 `[PROMPT]`、`[RELY]`、`[GUARANTEE]`、`[SPECIFICATION]` 四段。内容依次说明实现目标、已有接口、成员分工边界、处理规则和验证命令。

spec 先定义输入、允许的修改范围和不变量，再约定每种中断的行为、打印与关机边界、失败兜底和验证标准。Prompt 提供实际源码与接口作为依赖，要求保留异常处理与汇编入口，并且只在独立副本中补充测试支撑。

### 4.2.6 测试与验证

```bash
make obj/kern/trap/trap.o
```

本文件单独编译成功。

为验证本人模块，在仓库之外的独立副本加入遇到异常立即 panic 的临时异常处理支撑，以及非时钟分支的构造现场测试。其作用是让真实时钟路径可以链接运行。

| 验证内容 | 方法与判断 | 实际结果 |
|---|---|---|
| 非时钟分支 | 构造原因0、1、3、4、7、8、9、11、15，调用处理函数；检查相应输出、`ticks`仍为0、`epc`不变 | PASS |
| 未知中断 | 原因15打印原因号与完整TrapFrame | PASS |
| 持续真实时钟与返回 | 启动真实QEMU，反复进入和退出Trap；统计独立的`100 ticks`输出行 | 恰好10行 |
| 自动关机 | 不使用超时终止作为成功条件，等待SBI关机导致QEMU自行退出 | 退出码0 |
| 框架时钟用例 | 在同一独立副本中用原`tools/inject.py`注入原测试，编译`DEFS=-DGRADE_LAB3_TIMER` | `PASS (40/40, ticks=200)` |

框架时钟测试输出发生在第二行`100 ticks`后；其等待循环仍允许时钟中断继续触发，因此该次运行最终同样打印十行并自动关机。

真实输出摘要：

```text
[isolated] non-timer cases PASS
++ setup timer interrupts
100 ticks
100 ticks
[Test 3] Sustained supervisor timer: PASS (40/40, ticks=200)
```

此后还输出八行`100 ticks`，QEMU自行退出；自动校验确认总行数为10、退出码为0。

### 4.2.7 截图与复现

以下两张图片由我实际运行终端后截取，保留原始截图内容。

![图1：中断模块——持续时钟与自动关机（独立测试副本）](./images/huangyihao-timer-shutdown.png)

图1：终端显示时钟初始化完成，累计200次中断时，框架时钟用例输出 `PASS (40/40, ticks=200)`。画面共有十行 `100 ticks`，之后QEMU退出并返回shell，执行 `echo $?` 得到0。结合代码中的 `SHUTDOWN_AFTER=10`，验证持续重设事件、每百次打印和达到阈值后自动关机。

![图2：中断模块——非时钟分类与未知中断兜底（独立测试副本）](./images/huangyihao-interrupt-fallback.png)

图2：画面展示S态和M态外部中断提示，以及 `Unknown interrupt: cause 15` 后的完整TrapFrame。构造的现场中 `epc=0x12345678`，`cause=0x0000000f`，末尾输出 `[isolated] non-timer cases PASS`。测试断言检查非时钟分支不改变 `ticks` 与 `epc`。

在WSL进入该目录，运行已注入的时钟配置：

```bash
cd './work/lab3-interrupt-check-47ccmamf'
make qemu DEFS=-DGRADE_LAB3_TIMER QEMU=/mnt/d/Tools/qemu-4.1.1/bin/qemu-system-riscv64
echo $?
```

若要从头重建本机验证副本，可在PowerShell执行：

```powershell
python '.\work\verify_lab3_interrupt.py'
```

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

`exception_handler()` 接收汇编入口保存的 TrapFrame。`trap_dispatch()` 根据原因最高位区分中断与异常，再由本函数按 `tf->cause` 分类：原因 2 为非法指令，原因 3 为断点，分别输出 `Illegal instruction at`、`Breakpoint at` 及原始 EPC。其他原因调用 `print_trapframe(tf)` 后直接返回，保持 EPC 和其余现场不变。

EPC 指向触发同步异常的指令。处理完非法指令和断点后，需要跳过该指令，否则恢复后会再次陷入。通过 `const volatile uint16_t *` 读取 EPC 处首个半字，低两位为 `11` 时增加 4，否则增加 2，分别对应本实验的普通指令和压缩指令。半字读取支持四字节边界偏移 2 的普通指令，避免用未按四字节对齐的指针读取。取指只发生在两类已知异常中，未知异常不会读取可能无效的 EPC。

本实现针对实验现有可读内核代码映射和 16/32 位指令，没有扩展更长指令或通用访存故障恢复。保留 `lab3 function2 exception_handler` 标记，学号填写 `2411302`。

执行关系为 `__alltraps → SAVE_ALL → trap → trap_dispatch → exception_handler → RESTORE_ALL → sret`。函数更新栈中 TrapFrame 的 EPC，恢复代码据此写回 sepc，sret 从新地址继续执行。已有上下文恢复、中断处理及分发逻辑均保持不变。

### 4.3.4 关键代码

```c
//lab3 function2 exception_handler 2411302
void exception_handler(struct trapframe *tf) {
    switch (tf->cause) {
    case CAUSE_ILLEGAL_INSTRUCTION:
        cprintf("Illegal instruction at 0x%016lx\n", (unsigned long)tf->epc);
        break;
    case CAUSE_BREAKPOINT:
        cprintf("Breakpoint at 0x%016lx\n", (unsigned long)tf->epc);
        break;
    default:
        print_trapframe(tf);
        return;
    }

    /* Read one aligned halfword; a 32-bit instruction may start at EPC % 4 == 2. */
    uint16_t instruction = *(const volatile uint16_t *)tf->epc;
    tf->epc += (instruction & 0x3) == 0x3 ? 4 : 2;
}
```

### 4.3.5 Prompt

完整 Prompt 已追加到 [prompt.md](./prompt.md) 的异常处理部分。以下为精简规格：

```text
[PROMPT]
实现 exception_handler(struct trapframe *tf)，保留 lab3 标记并填写 2411302。
[RELY]
使用现有 TrapFrame、异常原因宏、cprintf()、print_trapframe() 和汇编保存恢复流程。
[GUARANTEE]
只修改本人函数，不修改中断、分发、汇编入口、结构定义、测试或 Makefile。
[SPECIFICATION]
原因 2、3 打印对应提示及原 EPC；读取首半字判断长度，EPC 增加 2 或 4。其他原因仅打印现场。实际编译、评分，并在独立副本补测压缩指令和半字对齐。
```

### 4.3.6 测试与验证

2026-11-06 在 WSL2 Ubuntu、RISC-V GCC 13.2.0、QEMU 4.1.1 中完成验证。执行 `make clean`、`make -j4` 编译成功；原始 `make grade` 三项分别得到 35/35、25/25、40/40，总分 100/100。命令和公共结果见第六节。

官方测试覆盖寄存器现场恢复、断点返回、32 位非法指令返回、原因 20 的完整现场输出及 200 次持续时钟中断。另在仓库外的独立副本增加如下测试，未将临时代码写回正式工程：

| 补充内容 | 实际检查 | 结果 |
|---|---|---|
| 16 位断点 | 执行 `.2byte 0x9002`，真实 Trap 返回后继续执行 | PASS |
| 32 位断点 | 在地址模 4 等于 2 处执行 `.4byte 0x00100073` | PASS |
| 16 位非法指令 | 执行 `.2byte 0x0000` 后跳过两字节 | PASS |
| 32 位非法指令 | 在地址模 4 等于 2 处执行 `.4byte 0x0000007b` 后跳过四字节 | PASS |
| 未知异常 | 构造 cause=20、epc=1，返回后逐字节比较整个 TrapFrame | 完全不变 |
| 中断兼容 | 异常测试后继续真实时钟，等待正常关机 | 十行 `100 ticks`，退出码 0 |

独立副本两次断点的实际地址为 `0xffffffffc020006c`、`0xffffffffc020006e`，两次非法指令为 `0xffffffffc0200072`、`0xffffffffc0200076`。它们属于本轮测试镜像，重新编译时地址可能变化。

本人的异常函数首次编译与测试即通过，没有遇到算法失败。运行评分时发现脚本直接从 PATH 调用 QEMU，通过仓库外启动器选定课程版本与固件，未改评分脚本。没有新增或虚构截图，临时代码、输出和构建产物未纳入提交。

# 五、关键知识理解

## 5.1 Trap、Interrupt 与 Exception

Trap 包括异步中断和同步异常。时钟中断由定时器事件触发，处理后继续原执行位置；非法指令和断点由当前指令触发。RV64 scause 的最高位指示中断，低位指示原因，trap_dispatch() 据此分发。

## 5.2 TrapFrame

TrapFrame 对应栈上的保存现场，包含 32 个通用寄存器槽及 status、epc、badvaddr、cause，RV64 下共 288 字节。C 结构与汇编偏移一致，异常函数更新其中的 epc，其余现场交由统一恢复流程。

## 5.3 与 Trap 有关的 CSR

stvec 指向入口，scause 保存原因，sepc 保存返回位置，sstatus 保存特权级和中断状态，stval（代码中的旧名为 sbadaddr）保存相关异常信息。sscratch 暂存原栈指针。SAVE_ALL 保存现场，RESTORE_ALL 将 status、epc 写回 CSR。

## 5.4 Trap 入口和返回过程

硬件保存 Trap 状态并跳到 stvec；入口保存寄存器、建立 TrapFrame，通过 a0 传给 C 函数。处理完成后恢复 status、epc 和通用寄存器，最后恢复 sp 并执行 sret。时钟中断保持 EPC；本实验的非法指令、断点按实际长度推进 EPC。

## 5.5 时钟中断

时钟初始化开启` sie.STIE`、设置第一次事件并清零`ticks`，之后由`intr_enable()`开启S态全局中断。硬件交给Trap入口，分发到`interrupt_handler()`后重新设置下一事件并计数，最后恢复现场返回原执行位置。时钟中断不应像某些同步异常那样增加`epc`。本实现每100次打印一次，十次打印后关机；时间源是`rdtime`而不是CPU指令周期。`volatile`保证对`ticks`的访问不被不适当地缓存或消除，但不提供多核同步。当前实验在单核启动场景下验证，没有实现调度器或通用外部设备驱动。

---

# 六、测试与验证

## 6.1 编译验证

在 `lab3` 仓库的 `code` 目录执行：

```bash
make clean
make -j4
```

编译与链接成功，生成 `bin/kernel`、`bin/ucore.img`，没有编译错误。本轮仅修改异常函数和本人报告、Prompt。

## 6.2 QEMU 运行验证

使用 6.3 节的课程 QEMU 启动器执行：

```bash
make qemu QEMU=/home/wlz/.cache/lab3-work/bin/qemu-system-riscv64
```

默认内核完成初始化，连续打印十行 `100 ticks` 后通过 SBI 正常关机，退出码 0。默认启动没有主动触发断点或非法指令；相应功能由官方测试和 4.3.6 节的独立副本验证。

## 6.3 实验测试

原评分脚本直接从 PATH 调用 `qemu-system-riscv64`，不使用 Makefile 的 QEMU 覆盖值。本机通过仓库外启动器固定使用 QEMU 4.1.1 与对应固件：

```bash
mkdir -p "$HOME/.cache/lab3-work/bin"
cat > "$HOME/.cache/lab3-work/bin/qemu-system-riscv64" <<'SH'
#!/bin/sh
exec /home/wlz/.local/opt/qemu-4.1.1/bin/qemu-system-riscv64 -L /home/wlz/.local/opt/qemu-4.1.1/share/qemu "$@"
SH
chmod +x "$HOME/.cache/lab3-work/bin/qemu-system-riscv64"
PATH="$HOME/.cache/lab3-work/bin:$PATH" make grade
```

评分脚本、注入器和官方用例均未修改，实际结果为：

```text
trap entry, register restore, and breakpoint EPC: PASS (35/35)
illegal instruction EPC advance and return: PASS (25/25)
re-armed supervisor timer and repeated trap return: PASS (40/40)
Total Score: 100/100
```

4.1、4.2 节保留两位组员当时的独立验证记录。本轮补齐异常函数后，正式整体联调覆盖现场恢复、断点和非法指令返回、未知异常打印及持续时钟。

# 七、实验过程中遇到的问题

## 黄奕浩：中断处理实现与独立验证

网页引用`SHUTDOWN_AFTER`，框架却只有`TICK_NUM=100`。补充阈值10并明确其单位为打印次数，避免误写成十次时钟中断就关机。原工程的异常处理尚未完成，直接链接不能形成完整内核；通过仓库外的临时异常panic支撑验证真实时钟路径，原工程仅声明异常函数，未代替同学完成。WSL Git与Windows Git对CRLF处理不同会产生整文件差异和尾随空白误报；本次使用换行兼容配置核对实际差异，没有为消除误报而批量重写其他文件。



# 八、实验结果

三位成员的 Trap 入口、中断和异常处理已整合。正式编译通过，官方总分 100/100，默认内核运行十次百中断统计后正常关机。独立副本补测了 16/32 位异常指令、半字对齐和未知异常现场保持。交付仍为 code/、report/，提示词统一放在 report/prompt.md。

# 九、实验总结

本实验将硬件 Trap 状态、汇编现场保存与 C 函数分发连接起来。异常处理根据原因选择处理策略，通过 TrapFrame 的 EPC 控制返回位置。兼容压缩指令和半字对齐，能够避免固定跳过四字节导致的错误；时钟中断则保持 EPC 并重设下一事件。实际评分及补充边界验证说明本人的实现能够与其他成员模块共同运行。
