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