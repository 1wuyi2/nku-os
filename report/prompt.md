# Lab1 提示词记录

本文件记录本次 Lab1 中实际使用过的提示词，按使用顺序整理。
---

## Prompt 1：练习1——分析 `kern_entry`

[PROMPT]

**任务**：阅读 `code/kern/init/entry.S`，结合相关代码完成 Lab1 练习1，解释：

1. `la sp, bootstacktop` 完成了什么操作，目的是什么；
2. `tail kern_init` 完成了什么操作，目的是什么。

**操作要求**：基于当前项目真实代码分析，不修改任何文件。

**输出要求**：先简单分析代码，再给出适合写进实验报告的最终答案。


[RELY]

请结合以下文件中的实际定义进行分析：

- `code/kern/init/entry.S`
- `code/kern/mm/memlayout.h`
- `code/kern/mm/mmu.h`
- `code/kern/init/init.c`
- `code/tools/kernel.ld`

需要关注：

```c
#define PGSIZE 4096
#define KSTACKPAGE 2
#define KSTACKSIZE (KSTACKPAGE * PGSIZE)
```

以及：

```asm
kern_entry:
    la sp, bootstacktop
    tail kern_init
```

和：

```c
int kern_init(void) __attribute__((noreturn));
```


[GUARANTEE]

需要回答：

- `la sp, bootstacktop` 的作用和目的；
- `tail kern_init` 的作用和目的；
- `bootstack`、`bootstacktop` 和 `sp` 的关系；
- `KSTACKSIZE` 的实际大小；
- 为什么 `kern_init` 不需要返回到 `kern_entry`。

不修改任何源码。


[SPECIFICATION]

## `la sp, bootstacktop`

**Pre-Condition**：
- 已进入 `kern_entry`；
- 已经为启动栈预留空间。

**Post-Condition**：
- `sp` 指向 `bootstacktop`；
- 后续 C 函数可以使用栈。

**Requirements**：
- 说明 `la` 和 `sp` 的含义；
- 说明栈向低地址增长；
- 根据代码计算启动栈大小。


## `tail kern_init`

**Pre-Condition**：
- 栈已经初始化完成。

**Post-Condition**：
- 程序进入 `kern_init` 执行。

**Requirements**：
- 说明 `tail` 的作用；
- 结合 `kern_init` 的 `noreturn` 属性解释为什么这里不需要返回；
- 与普通 `call` 做简单区别即可。

---
## Prompt 2：练习2——分析 GDB 调试流程

[PROMPT]

**任务**：分析当前 Lab1 的 GDB 调试方式，告诉我如何完成练习2：从 CPU 加电开始调试，观察最初执行地址，并最终跟踪到内核入口 `0x80200000`。

**操作要求**：
- 阅读当前项目中的 `code/Makefile`、`code/tools/kernel.ld` 和 `code/kern/init/entry.S`。
- 不修改任何文件。
- 不执行 commit 或 push。
- 只分析当前项目真实的 `debug` 和 `gdb` 目标。

**输出要求**：
给出两个终端分别应该执行什么命令，并解释主要 GDB 命令的作用。


[RELY]

当前已经确认：

```text
make qemu
```

能够正常运行，并输出：

```text
Domain0 Next Address : 0x0000000080200000
(THU.CST) os is loading ...
```

当前 `debug` 目标已经改为使用：

```makefile
-kernel $(UCOREIMG) \
-s -S
```

其中：

- `-S`：启动后暂停 CPU；
- `-s`：开放 GDB 调试端口。

`gdb` 目标用于加载 `bin/kernel` 并连接 QEMU。


[GUARANTEE]

需要说明：

1. 终端1如何启动 QEMU 调试模式；
2. 终端2如何启动 GDB；
3. 如何查看当前 PC；
4. 如何查看当前地址附近的汇编指令；
5. 如何设置 `0x80200000` 断点；
6. 如何继续运行直到内核入口；
7. 如何确认当前已经进入 `kern_entry`。

不修改任何代码。


[SPECIFICATION]

## 开机位置

**Post-Condition**：
- GDB 连接后能够观察 CPU 当前执行位置；
- 应重点确认最初的 PC 是否位于 `0x1000` 附近。

## 内核入口

**Post-Condition**：
- 在 `0x80200000` 设置断点；
- 继续运行后停在该地址；
- 能结合符号信息确认这里对应 `kern_entry`。

请同时解释 `info registers pc`、`x/10i $pc`、`break *0x80200000`、`continue`、`si` 各自是什么意思。
