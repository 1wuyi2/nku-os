任务：Lab3 功能一——Trap 入口与上下文恢复  
负责人：谭锦睿 2412771

[PROMPT]

当前仓库：`/home/tjr/os-lab/nku-os`  
实验分支：`lab3`  
代码目录：`code/`

阅读当前工程中的：

```text
kern/trap/trapentry.S
kern/trap/trap.h
kern/trap/trap.c
```

完成 `kern/trap/trapentry.S` 中功能一对应的两个部分：

```text
RESTORE_ALL
__alltraps
```

并将对应位置的 `YOUR CODE` 改为学号 `2412771`。

[RELY]

实现时以当前代码中已有的 `SAVE_ALL`、`struct trapframe` 和 `trap()` 为基础。

先分析 `SAVE_ALL` 中各寄存器保存到栈中的位置，再按照相同的 TrapFrame 布局完成恢复。注意当前为 RISC-V 64 位环境，结合 `REGBYTES`、`LOAD`、`STORE` 等已有宏实现。

[GUARANTEE]

原则上只修改：

```text
kern/trap/trapentry.S
```

不要修改已有的 `SAVE_ALL`、`__trapret` 和 TrapFrame 定义。

另外不要修改其他组员负责的：

```text
interrupt_handler()
exception_handler()
```

即使当前完整 `make` 因这两个函数未实现而失败，也不要添加临时空函数。

[SPECIFICATION]

`RESTORE_ALL` 中需要：

- 恢复 `sstatus` 和 `sepc`；
- 恢复 Trap 前的通用寄存器；
- `x0` 不需要恢复；
- `sp/x2` 最后恢复，避免提前改变 TrapFrame 的访问基址。

`__alltraps` 中需要：

- 先执行 `SAVE_ALL` 保存现场；
- 此时 `sp` 指向当前 TrapFrame；
- 将 `sp` 放入 `a0`，作为 `trap(struct trapframe *tf)` 的参数；
- 调用 `trap()`，返回后进入已有的 `__trapret`。

完成后检查：

```bash
git diff --check
git diff -- code/kern/trap/trapentry.S
git diff --name-only
```

并单独编译：

```bash
cd /home/tjr/os-lab/nku-os/code
make obj/kern/trap/trapentry.o V=
```

最后说明实际修改内容、TrapFrame 的保存恢复关系，以及 `__alltraps → trap() → __trapret → sret` 的执行流程。

---

任务：Lab3 功能二——中断处理
负责人：黄奕浩 2413581

[PROMPT]

当前仓库：`/mnt/d/360MoveData/Users/huangyihao/Desktop/OS实验/lab3/nku-os`
实验分支：`lab3`
代码目录：`code/`

阅读当前工程中的：

```text
kern/trap/trap.c
kern/trap/trap.h
kern/trap/trapentry.S
kern/driver/clock.c
kern/driver/clock.h
kern/driver/intr.c
libs/riscv.h
libs/sbi.h
libs/sbi.c
```

完成 `kern/trap/trap.c` 中功能二对应的中断处理函数：

```c
void interrupt_handler(struct trapframe *tf);
```

并将本人实现对应位置的 `YOUR CODE` 改为学号 `2413581`，保留异常处理部分的待实现标记。

[RELY]

实现时以当前代码中已有的 `struct trapframe`、`trap_dispatch()`、`print_ticks()` 和 `print_trapframe()` 为基础。

先分析 `tf->cause` 的中断标志位和中断编号，再结合 `libs/riscv.h` 中的 IRQ 宏完成分类。注意当前为 RISC-V 64 位环境，原因最高位为中断标志，比较中断编号前需要清除该位。

时钟处理使用已有的 `clock_set_next_event()`、`ticks` 和 `TICK_NUM`；关机使用 `libs/sbi.h` 中声明的 `sbi_shutdown()`。结合 `clock_init()`、`intr_enable()` 和 Trap 返回流程理解持续时钟中断的执行过程。

[GUARANTEE]

原则上只修改：

```text
kern/trap/trap.c
```

不要修改已有的 `trap_dispatch()`、`trap()`、`print_ticks()`、`print_trapframe()` 和 TrapFrame 定义。可以增加必要的头文件、宏定义和函数声明。

另外不要修改其他组员负责的：

```text
RESTORE_ALL
__alltraps
exception_handler()
```

即使当前完整 `make` 因异常处理函数未实现而失败，也不要在正式工程中添加临时空函数。保留官方测试、Makefile 和启动代码。

[SPECIFICATION]

`interrupt_handler()` 中需要：

- 根据 `tf->cause` 去掉最高中断标志位后的编号，区分不同类型的中断；
- 对 U/S/M 软件中断、U/M 时钟中断和 U/S/M 外部中断输出对应类型提示；
- 未知中断打印原因编号，并调用 `print_trapframe(tf)` 输出完整现场；
- 不修改 `tf->epc` 和已保存的通用寄存器，处理完成后交回原有 Trap 返回流程。

`IRQ_S_TIMER` 分支中需要：

- 先调用 `clock_set_next_event()` 设置下一次时钟事件；
- 每次时钟中断只执行一次 `ticks++`；
- 当 `ticks` 达到 `TICK_NUM` 的整数倍时，调用 `print_ticks()`；
- 保留 `TICK_NUM = 100`，补充 `SHUTDOWN_AFTER = 10`，累计打印十次后调用 `sbi_shutdown()`；
- 说明关机阈值10是本次选择的参数，不将其写成指导书明确指定的数值。

完成后检查：

```bash
git -c core.autocrlf=true diff --check
git -c core.autocrlf=true diff -- code/kern/trap/trap.c
git -c core.autocrlf=true diff --name-only
```

并单独编译：

```bash
cd '/mnt/d/360MoveData/Users/huangyihao/Desktop/OS实验/lab3/nku-os/code'
make obj/kern/trap/trap.o V=
```

进一步在仓库外的独立副本验证：构造非时钟中断现场，检查分类输出、未知原因兜底、`ticks` 和 `epc` 保持不变；在真实 QEMU 中检查持续时钟、十行 `100 ticks` 和自动关机；使用原注入器和 `GRADE_LAB3_TIMER` 运行框架时钟用例，检查200次中断验证通过。副本可以增加遇到异常立即 panic 的测试支撑，但不得将临时支撑或测试修改回填正式工程，也不得把时钟用例通过写成完整实验评分通过。

最后说明实际修改内容、中断原因的识别方式，以及 `时钟事件 → Trap 入口 → trap_dispatch() → interrupt_handler() → 重设事件并计数 → 恢复现场返回` 的执行流程。同步补充本人报告和真实截图方法，未实际运行或截图的内容不得提前写成已完成。
