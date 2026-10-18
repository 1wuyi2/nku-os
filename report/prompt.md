# Lab2 AI Prompt 记录

本文件记录谭锦睿负责的两个代码实现任务所使用的 Prompt。First-Fit 任务为源码阅读与思考题，不单独记录 Prompt。

## 1. 功能模块一任务一：`page_init()`

### [PROMPT]

基于当前 Lab2 工程的真实代码，实现 `kern/mm/pmm.c` 中的 `page_init()`。该函数需要根据 DTB 提供的物理内存范围，建立物理页描述符数组并把可用的完整物理页登记给当前 PMM 管理器。只修改完成该任务所必需的代码，不重写无关模块。

### [RELY]

先阅读 `kern/mm/pmm.c`、`kern/mm/pmm.h`、`kern/mm/memlayout.h`、`kern/mm/mmu.h`、DTB 接口和 `tools/kernel.ld`。所有函数名、全局变量、地址转换宏和页面对齐宏均以当前源码为准，重点使用 `get_memory_base()`、`get_memory_size()`、`pages`、`npage`、`nbase`、`va_pa_offset`、`PADDR`、`pa2page()` 与 `init_memmap()`。

### [GUARANTEE]

不得修改框架测试和评分脚本。实现后应正确设置 `va_pa_offset`、`npage` 和 `pages`，初始化对应的 `Page` 描述符，保留固件、内核镜像与 `Page[]` 自身占用的区域，并只把真正可用的完整物理页交给 `pmm_manager`。

### [SPECIFICATION]

从 DTB 内存起始地址和容量计算可管理范围，正确处理虚实地址转换及上下页边界对齐；将描述符数组放在内核 `end` 之后并初始化为保留状态；从描述符数组结束后的首个完整页开始计算空闲区并调用 `init_memmap()`。同时处理无效内存范围、地址越界以及没有完整空闲页的情况。

## 2. 功能模块二任务二：Best-Fit

### [PROMPT]

参考 `kern/mm/default_pmm.c` 的 First-Fit 实现，补全 `kern/mm/best_fit_pmm.c` 中标有 `lab2` 和 `YOUR CODE` 的 Best-Fit 物理内存管理模块，并修改 `kern/mm/pmm.c` 的 `init_pmm_manager()`，使系统选择 `best_fit_pmm_manager`。

### [RELY]

所有实现必须以当前工程的 `default_pmm.c`、`pmm.h`、`memlayout.h`、`libs/list.h` 以及 `best_fit_pmm.c` 中现有测试为准。使用项目已有的 `struct pmm_manager`、`struct Page`、`free_area_t`、`free_list`、`nr_free`、`property`、`PG_property`、`page_link` 和链表操作接口，不自行创造替代接口。

### [GUARANTEE]

完整实现管理器初始化、初始空闲区登记、连续页分配、页面释放、相邻空闲块合并、空闲页数量查询和 `best_fit_pmm_manager` 注册。不得修改 `basic_check()`、`best_fit_check()`、评分脚本或无关源码；切换 PMM 管理器后应能够通过现有测试。

### [SPECIFICATION]

分配连续 `n` 页时遍历全部可满足 `property >= n` 的空闲块，并选择其中尺寸最小者。若块大于请求，则从低地址侧分配并把剩余部分重建为空闲块。释放时恢复页面状态，按物理地址顺序插回 `free_list`，再合并前后相邻空闲块。全过程正确维护块头的 `property`、`PG_property`、链表结构和表示空闲页总数的 `nr_free`。
