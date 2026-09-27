# Lab1 实验报告：比麻雀更小的麻雀（最小可执行内核）

## 一、实验基本信息

- 实验名称：Lab1 比麻雀更小的麻雀（最小可执行内核）
- 小组组号：96
- 组长：谭锦睿，学号 2412771
- 组员：黄奕浩，学号 2413581
- 组员：王乐之，学号 2411302
- 实验分支：`lab1`
- 代码目录：`code/`
- 报告目录：`report/`

---

## 二、实验目的

本次实验主要围绕“最小可执行内核”和 RISC-V 启动流程展开。实验的重点并不是实现进程、虚拟内存或文件系统，而是先理解一个操作系统内核在没有其他操作系统帮助的情况下，如何从机器加电开始，一步步获得 CPU 控制权并执行到 C 语言代码。

本次实验主要完成了以下内容：

1. 使用 RISC-V 交叉编译工具链编译、链接 uCore 内核；
2. 理解链接脚本、入口符号和内核地址 `0x80200000` 的关系；
3. 理解 QEMU、OpenSBI 和 uCore 之间的启动关系；
4. 分析 `kern/init/entry.S` 中内核栈的建立和对 `kern_init()` 的跳转；
5. 使用 GDB 从 CPU 加电后的 `0x1000` 开始调试，验证启动代码、OpenSBI 和内核入口之间的控制权转移；
6. 验证内核能够最终输出 `(THU.CST) os is loading ...`。

---

## 三、AI 环境

本实验过程中使用 ChatGPT 辅助理解实验要求、分析调试结果和检查实验思路，使用 OpenCode 读取当前真实项目代码并完成指定的代码分析或最小修改。

AI 的使用方式主要是：

- 先根据实验要求编写四段式提示词；
- 要求 AI 以当前项目中的真实文件为依据分析，避免凭空生成与项目不一致的代码；
- 对涉及代码修改的任务限制修改范围，并要求展示 `git diff`；
- 所有修改和分析结果都由我重新通过编译、QEMU 运行和 GDB 调试验证；
- 实际使用过的完整提示词统一记录在 `report/prompt.md`。

本次提示词均按照 `[PROMPT]`、`[RELY]`、`[GUARANTEE]`、`[SPECIFICATION]` 四段式组织。

---

## 四、实验整体逻辑分析

我对 Lab1 的整体理解可以概括成下面这条启动链：

```text
加电复位
    ↓
CPU 从 0x1000 开始执行 MROM 启动代码
    ↓
跳转到 0x80000000 的 OpenSBI
    ↓
OpenSBI 完成初始化并准备下一阶段
    ↓
跳转到 0x80200000
    ↓
执行 kern/init/entry.S 中的 kern_entry
    ↓
设置 uCore 自己的内核栈
    ↓
跳转到 kern_init()
    ↓
执行 C 语言代码
    ↓
输出 (THU.CST) os is loading ...
```

其中三个地址是理解本实验的关键：

- `0x1000`：QEMU 模拟的 RISC-V CPU 的复位入口；
- `0x80000000`：OpenSBI 固件运行位置；
- `0x80200000`：uCore 内核入口地址。

`tools/kernel.ld` 中通过：

```ld
ENTRY(kern_entry)
BASE_ADDRESS = 0x80200000;
```

指定了内核入口和链接基地址，因此当 OpenSBI 将控制权交给 `0x80200000` 后，就会进入 `kern_entry`。

---

## 五、实验内容与实现

### 5.1 练习1：理解内核入口操作

本部分使用的最终提示词为 Prompt 1，完整内容见 `report/prompt.md`。提示词要求 AI 只基于当前项目真实代码进行分析，不修改文件，并重点结合 `entry.S`、`memlayout.h`、`mmu.h`、`init.c` 和 `kernel.ld`。

#### 5.1.1 `la sp, bootstacktop`

`sp` 是 RISC-V 的栈指针寄存器。C 函数调用需要栈来保存局部变量、返回地址和临时数据，因此在进入 `kern_init()` 前必须先建立一个可用的内核栈。

相关宏定义为：

```c
#define PGSIZE 4096
#define KSTACKPAGE 2
#define KSTACKSIZE (KSTACKPAGE * PGSIZE)
```

所以启动栈大小为：

```text
KSTACKSIZE = 2 × 4096 = 8192 B = 8 KiB
```

`entry.S` 中通过 `bootstack` 预留这 8 KiB 的空间，`bootstacktop` 表示这块空间的高地址端。RISC-V 的栈向低地址方向增长，因此：

```asm
la sp, bootstacktop
```

的作用就是把 `sp` 设置到 `bootstacktop`，为后续 C 函数调用建立内核自己的启动栈。

GDB 中实际观察到刚进入 `kern_entry` 时：

```text
sp = 0x80046eb0
```

同时：

```text
&bootstacktop = 0x80203000
```

执行 `la sp, bootstacktop` 对应的真实机器指令后：

```text
sp = 0x80203000
```

说明内核栈初始化确实完成。

![GDB 单步验证内核栈初始化](images/image4.png)

#### 5.1.2 `tail kern_init`

`tail kern_init` 表示直接将控制权交给 `kern_init()`，而不需要像普通函数调用那样等待返回。

`kern_init()` 被声明为：

```c
int kern_init(void) __attribute__((noreturn));
```

并且最后进入无限循环，因此它不会返回到 `kern_entry`，这里使用尾调用是合理的。

实际反汇编得到：

```asm
0x80200000 <kern_entry>:   auipc   sp,0x3
0x80200004 <kern_entry+4>: mv      sp,sp
0x80200008 <kern_entry+8>: j       0x8020000a <kern_init>
```

可以看到，源码中的 `la` 和 `tail` 都是伪指令，最终会被汇编器展开或转换为 CPU 真正执行的机器指令。

#### 实现迭代过程

本练习没有修改源码。AI 先根据相关宏和入口代码进行静态分析，我随后又在练习2的 GDB 调试中通过观察 `sp` 和单步执行，对练习1的结论进行了动态验证。

---

### 5.2 QEMU 启动方式的兼容性修改

本部分使用的最终提示词为 Prompt 2，完整内容见 `report/prompt.md`。

最初 `Makefile` 中的 `qemu` 目标使用：

```makefile
-device loader,file=$(UCOREIMG),addr=0x80200000
```

在当前环境下运行后，OpenSBI 显示：

```text
Domain0 Next Address : 0x0000000000000000
```

此时没有进入内核，也没有出现 `(THU.CST) os is loading ...`。

为了判断问题位置，我先手工测试：

```bash
qemu-system-riscv64 \
  -machine virt \
  -nographic \
  -bios default \
  -kernel bin/ucore.img
```

这时可以正常看到：

```text
Domain0 Next Address : 0x0000000080200000
(THU.CST) os is loading ...
```

因此提示 OpenCode 只对 `Makefile` 中的 `qemu` 和 `debug` 目标做最小修改，将加载方式调整为：

```makefile
-kernel $(UCOREIMG)
```

同时 `debug` 仍然保留：

```text
-s -S
```

其中 `-s` 用于开启 GDB 调试端口，`-S` 用于让 CPU 在启动时暂停。

#### 实现迭代过程

本部分的实际过程为：

```text
原 make qemu
    ↓
Domain0 Next Address = 0
    ↓
内核没有输出
    ↓
手工测试 -kernel bin/ucore.img
    ↓
Next Address = 0x80200000，内核输出成功
    ↓
使用 Prompt 2 要求 OpenCode 做最小修改
    ↓
检查 git diff
    ↓
重新 make qemu 验证
    ↓
启动成功
```

这次修改没有改动任何内核源码，只调整了当前环境下 QEMU 的启动参数。

---

### 5.3 练习2：使用 GDB 验证启动流程

本部分使用的最终提示词为 Prompt 3，完整内容见 `report/prompt.md`。提示词要求只分析当前 `Makefile`、`kernel.ld` 和 `entry.S`，不修改任何代码。

#### 5.3.1 启动调试环境

终端 1 执行：

```bash
cd ~/os-lab/nku-os/code
make debug
```

当前 `debug` 目标使用：

```text
-s -S
```

其中：

- `-S`：QEMU 创建 CPU 后先暂停，不立即执行；
- `-s`：在 1234 端口开放 GDB 调试接口。

终端 2 执行：

```bash
cd ~/os-lab/nku-os/code
make gdb
```

GDB 加载的是带调试符号的 `bin/kernel`，并通过：

```text
target remote localhost:1234
```

连接到 QEMU。

连接成功后可以看到：

```text
0x0000000000001000 in ?? ()
```

说明 CPU 此时停在 `0x1000`。

![GDB 成功连接并停在 0x1000](images/image2.png)

#### 5.3.2 观察 `0x1000` 附近的启动代码

执行：

```gdb
info registers pc
x/10i $pc
```

得到：

```asm
0x1000: auipc   t0,0x0
0x1004: addi    a2,t0,40
0x1008: csrr    a0,mhartid
0x100c: ld      a1,32(t0)
0x1010: ld      t0,24(t0)
0x1014: jr      t0
```

说明 RISC-V 加电后最初执行的指令确实位于 `0x1000` 附近。这几条代码主要完成启动参数准备，并取得下一阶段固件入口地址。

继续执行：

```gdb
x/4gx 0x1018
```

得到：

```text
0x1018: 0x0000000080000000
0x1020: 0x0000000087e00000
0x1028: 0x000000004942534f
0x1030: 0x0000000000000002
```

其中 `0x1018` 中保存的值为：

```text
0x80000000
```

结合：

```asm
ld t0,24(t0)
jr t0
```

可以确认启动代码会把 `0x80000000` 装入 `t0`，然后通过 `jr t0` 将 CPU 控制权交给 OpenSBI。

#### 5.3.3 跟踪到 `0x80200000`

继续设置断点：

```gdb
break *0x80200000
continue
```

实际得到：

```text
Breakpoint 1, kern_entry () at kern/init/entry.S:7
7           la sp, bootstacktop
```

再执行：

```gdb
info registers pc
info symbol $pc
```

得到：

```text
pc = 0x80200000 <kern_entry>
kern_entry in section .text
```

这说明 OpenSBI 已经把控制权交给 uCore，CPU 正式进入内核入口 `kern_entry`。

![从 0x1000 跟踪到 0x80200000](images/image3.png)

#### 5.3.4 单步验证进入 `kern_init()`

进入 `kern_entry` 时先观察：

```text
sp = 0x80046eb0
&bootstacktop = 0x80203000
```

反汇编结果为：

```asm
0x80200000 <kern_entry>:   auipc   sp,0x3
0x80200004 <kern_entry+4>: mv      sp,sp
0x80200008 <kern_entry+8>: j       0x8020000a <kern_init>
```

执行第一条真实机器指令后：

```text
pc = 0x80200004
sp = 0x80203000
```

随后继续单步，最终到达：

```text
pc = 0x8020000a <kern_init>
sp = 0x80203000
```

GDB 显示：

```text
kern_init () at kern/init/init.c:8
8           memset(edata, 0, end - edata);
```

因此实际验证了：

```text
0x80200000 kern_entry
    ↓
sp = bootstacktop
    ↓
tail kern_init
    ↓
kern_init()
```

![单步执行 kern_entry 并进入 kern_init](images/image4.png)

#### 实现迭代过程

练习2没有修改代码。AI 根据当前项目的 `debug` 和 `gdb` 目标给出调试步骤，我实际运行后依次验证：

1. `PC = 0x1000`；
2. `0x1000` 附近的启动指令；
3. `0x1018` 中保存 `0x80000000`；
4. 在 `0x80200000` 设置断点并命中 `kern_entry`；
5. 单步验证 `sp` 被设置为 `bootstacktop`；
6. 最终进入 `kern_init()`。

---

## 六、重要知识点及其与 OS 原理的联系

### 6.1 CPU、PC 与启动过程

CPU 总是根据 PC 指向的地址取指执行。实验中实际观察到 CPU 加电后 `PC = 0x1000`，随后经过固件初始化，最终变为 `0x80200000` 并进入内核。

这让我把“CPU 执行程序”从抽象概念具体到了一个实际的地址变化过程。

### 6.2 链接脚本与内存布局

`kernel.ld` 决定内核各个 section 的布局，并指定入口和链接地址。源码中的函数名本身并不能决定程序最终放在什么物理地址，真正的地址布局需要经过编译和链接过程确定。

### 6.3 内核栈与 C 语言运行环境

在进入 C 代码之前，需要先准备一个可用的栈。`entry.S` 中设置 `sp` 的操作说明，即使一个非常小的内核，在调用 C 函数之前也需要满足基本的 ABI 和运行环境要求。

### 6.4 OpenSBI 与内核

OpenSBI 不是 uCore 内核的一部分，而是启动内核前运行的固件。实验中 OpenSBI 位于 `0x80000000`，完成早期初始化后再把控制权交给位于 `0x80200000` 的 uCore。

### 6.5 ELF、内核镜像和调试符号

`bin/kernel` 是带调试符号的 ELF 文件，因此 GDB 能把 `0x80200000` 显示为 `kern_entry`；`bin/ucore.img` 是用于启动的内核镜像。两者用途不同。

---

## 七、OS 原理中重要但本实验尚未体现的内容

Lab1 主要解决“内核如何启动”这个问题，因此很多操作系统核心机制还没有真正实现，例如：

- 进程与线程；
- CPU 调度；
- 中断和异常处理；
- 系统调用；
- 用户态与内核态切换；
- 物理内存管理；
- 页表与虚拟内存；
- 文件系统；
- 并发与同步。

这些内容需要在后续实验中继续实现。Lab1 更像是后续所有功能的运行基础。

---

## 八、测试与验证

### 8.1 编译验证

在 `code/` 目录执行：

```bash
make
```

能够正常生成：

```text
bin/kernel
bin/ucore.img
```

说明当前代码可以成功编译、链接并生成内核镜像。

### 8.2 QEMU 运行验证

执行：

```bash
make qemu
```

OpenSBI 输出中可以看到：

```text
Firmware Base        : 0x80000000
Domain0 Next Address : 0x0000000080200000
Domain0 Next Mode    : S-mode
```

最后出现：

```text
(THU.CST) os is loading ...
```

说明完整启动链已经运行成功。

![QEMU 最终启动结果](images/image5.png)

### 8.3 GDB 验证

GDB 调试实际验证了：

```text
0x1000
    ↓
0x80000000 OpenSBI
    ↓
0x80200000 kern_entry
    ↓
sp = 0x80203000
    ↓
kern_init()
```

其中 `image2.png`、`image3.png` 和 `image4.png` 分别记录了连接后的初始位置、进入 `kern_entry` 以及单步进入 `kern_init()` 的过程。

---

## 九、实验总结与收获

本次实验最重要的收获，是把以前比较抽象的“操作系统启动”过程具体到了 CPU 实际执行的地址和指令上。

一开始只运行 `make qemu` 时，看到 `(THU.CST) os is loading ...` 很容易把它当作普通程序输出。但通过阅读链接脚本、`entry.S` 以及 GDB 调试后，我能够把这行输出之前发生的过程连起来：CPU 从 `0x1000` 开始执行启动代码，跳到 `0x80000000` 的 OpenSBI，再由 OpenSBI 把控制权交给 `0x80200000` 的 uCore。进入 `kern_entry` 后，内核先建立自己的栈，再进入 `kern_init()` 执行 C 语言代码。

练习1中对 `la sp, bootstacktop` 和 `tail kern_init` 的分析，在练习2中又通过 GDB 得到了实际验证。尤其是看到 `sp` 从 OpenSBI 留下的地址变成 `bootstacktop = 0x80203000`，让我更直观地理解了为什么进入 C 函数前必须先准备栈。

另外，本次实验还遇到了当前 QEMU/OpenSBI 环境下原有启动方式不能正确进入内核的问题。我先通过手工命令验证 `-kernel bin/ucore.img` 可以成功启动，再限制 OpenCode 只对 `Makefile` 的 `qemu` 和 `debug` 目标做最小修改，最后重新运行验证。这一过程也让我体会到，使用 AI 修改代码时不能只看生成结果，而应该先明确问题、限制修改范围，再通过真实运行结果确认修改是否正确。

总体来看，Lab1 虽然代码量很小，但它建立了后续实验最基础的一条链：机器启动、固件初始化、进入内核、建立栈、进入 C 代码。后续的内存管理、中断、页表和进程等功能，都需要建立在这条启动链能够正确工作的基础上。
