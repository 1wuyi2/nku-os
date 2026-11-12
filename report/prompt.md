# 2412771-谭锦睿 AI Prompt 记录

## Lab4 Function 1

[PROMPT]

完成本人负责的 Lab4 Function 1，只实现 `page_insert()` 和 `page_remove()`。

要求：

1. 先阅读当前工程中的 `struct Page`、PTE/PDE、`get_pte()`、引用计数、`pte2page()`、`free_page()`、TLB 和错误码定义。
2. 正确处理：
   - 无原映射；
   - 同一物理页重映射；
   - 替换为不同物理页。
3. 页表创建失败时返回当前框架规定的 `-E_NO_MEM`。
4. 保留 `//LAB4 function1 YOUR CODE` 等原有注释，不修改 Function 2 / Function 3。
5. 完成后编译并确认 `page_insert/page_remove` 不再出现 undefined reference。
6. 根据实际结果补充本人 `report.md`、`prompt.md` 以及 Sv32/Sv39/Sv48、`get_pte()` 相关 Challenge。

[RELY]

主要依据当前工程：

- `code/kern/mm/pmm.c`
- `code/kern/mm/pmm.h`
- 页表和 RISC-V 相关头文件
- 当前已恢复的 Lab2 / Lab3 代码

所有接口以当前 Lab4 实际代码为准，不直接套用其他版本。

[GUARANTEE]

- 只修改 Function 1。
- 不修改 `proc_init()`、`cpu_idle()`、进程创建和调度代码。
- 不删除或修改原实验注释。
- 不做无关重构和格式化。
- 不 commit、不 push。
- 未实际运行的测试不得伪造结果。

[SPECIFICATION]

最终完成：

1. `page_insert()`、`page_remove()` 实现；
2. 引用计数、旧映射删除、页面释放和 TLB 刷新逻辑分析；
3. 编译和 `git diff --check` 验证；
4. 记录 Function 1 的真实测试状态；
5. 完成 Sv32、Sv39、Sv48 和 `get_pte()` Challenge；
6. 更新本人负责的 `report.md` 和 `prompt.md` 内容。

---

# 2413581-黄奕浩 AI Prompt 记录

## Lab4 Function 2

[PROMPT]

完成本人负责的 Lab4 Function 2，补全进程 / 内核线程创建。

要求：

1. 先阅读指导书，以及当前工程的 `proc_struct`、`context`、`trapframe`、内核栈分配、PID 分配、链表、哈希表、中断控制和线程入口。
2. 实现 `alloc_proc()`、`proc_init()`、`kernel_thread()`、`do_fork()`、`copy_thread()`、`copy_mm()`。
3. 正确初始化 idleproc 和 initproc，准备新线程的 trapframe 和 context，保证创建后可被后续调度器使用。
4. 使用当前工程的 `boot_pgdir_pa`、`bootstack`、`kernel_thread_entry`、`forkret`、`forkrets` 和 `wakeup_proc()`，不套用其他体系结构的寄存器或页表接口。
5. 处理 PCB / 内核栈分配失败、进程数上限与失败回滚；在登记 PID、链表、哈希表和进程计数时保存并恢复中断状态。
6. 保持当前阶段 `mm == NULL`。
7. 运行必要测试，记录实际结果并更新报告。

[RELY]

主要依据：

- 指导书功能模块二、idleproc 与 initproc 创建部分；
- `code/kern/process/proc.c`、`proc.h`、`entry.S`；
- `code/kern/trap/trapentry.S`、`trap.h`；
- `code/kern/mm/pmm.h`、`memlayout.h`、`kmalloc.h`；
- `code/kern/sync/sync.h` 与 `code/libs/riscv.h`；
- `code/tools/test_lab4.c` 中现有的进程创建检查。

所有接口以当前 Lab4 实际代码为准。

[GUARANTEE]

- 只补全 Function 2 及本人测试、报告和 Prompt，不修改 Function 1 / Function 3 实现。
- 保留实验原有注释，不做无关格式化，不 commit、不 push。
- 创建失败后不遗留 PCB、内核栈或进程登记。
- 独立测试副本放在 D 盘实验目录；故障注入包装和测试版 cpu_idle 只存在于副本。

[SPECIFICATION]

最终完成：

1. 六个创建函数及 PCB、内核栈、trapframe/context 的说明；
2. idleproc/initproc、父子关系、PID/链表/哈希表登记与错误路径分析；
3. 新增可复现的独立创建测试，覆盖初值、多个线程、指定栈、中断状态、分配失败和进程数上限；
4. 在 QEMU 中调用现有 `test_lab4_process_create()` 并执行补充断言，保存真实日志；
5. 记录原工程完整构建仍被 Function 3 的 `cpu_idle` 未定义阻塞；
