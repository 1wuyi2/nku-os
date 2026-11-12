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