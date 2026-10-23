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



# Lab2 Buddy System 提示词记录

负责人：2413581-黄奕浩。本文件按原 `prompt.md` 的四段式格式，分别记录分配模块与测试函数的提示词，原文件保持不变。

## 1. 功能模块二任务三：Buddy System 分配模块

### [PROMPT]

基于当前 Lab2 的真实 uCore 物理内存管理框架，从零实现 Buddy System 分配模块并接入内核。新增 `kern/mm/buddy_pmm.c` 和 `buddy_pmm.h`，只在 `pmm.c` 增加必要的管理器选择。不得修改 Makefile，不新增运行脚本、配置文件、日志目录或自动生成的图片。

### [RELY]

先阅读 `pmm.h`、`memlayout.h`、`default_pmm.c`、`best_fit_pmm.c`、`libs/list.h` 以及现有 `pmm.c`。使用当前 `struct pmm_manager`、`struct Page`、页状态宏、双向链表接口、`pages`、`nbase`、`npage` 和 `page2ppn()`。利用 Makefile 已有的 `DEFS` 编译参数，在 `pmm.c` 根据 `USE_BUDDY` 宏选择 Buddy；未定义时保留 Best-Fit。

### [GUARANTEE]

实现初始化、可用页登记、连续页分配、释放、伙伴合并、空闲页查询和管理器注册。保留现有 `page_init()`、Best-Fit 实现、框架测试与评分脚本，不修改 `struct Page`。初始区间即使不对齐、长度不是二次幂，也不能丢弃其中可用页。申请失败不能改变状态。明确记录原请求数和实际占用数，避免释放不完整或重复释放。

### [SPECIFICATION]

使用按阶分类的链表，第 order 阶管理 `2^order` 页块。按绝对物理页号的对齐和区间剩余长度拆分初始空闲区。申请 n 页向上取二次幂，从能够满足请求的最小非空阶逐级分裂，剩余半块加入对应链表。空闲页计数扣除实际块页数，已分配块头记录原请求 n；释放必须使用原申请地址与原 n，回收整个实际块，不支持部分释放。伙伴页号为 `ppn XOR (1 << order)`，伙伴必须同阶、空闲且在有效范围内才能递归合并。已分配块头使用独立标志，错误释放和重复释放应触发断言。切换算法前执行 `make clean`，Buddy 的编译和运行命令使用 `DEFS=-DUSE_BUDDY`，不修改任何构建规则。

## 2. 功能模块二任务三：Buddy System 测试函数

### [PROMPT]

针对第一份提示词生成的实际 Buddy 分配器，设计并实现 `buddy_check()`，通过 `buddy_pmm_manager.check` 接口在真实内核启动时执行。测试直接输出到 QEMU 终端，供本人截图；不增加用户态测试脚本或证据文件夹。

### [RELY]

使用当前 `alloc_pages()`、`free_pages()`、`nr_free_pages()`、`assert()`、`cprintf()`、`page2pa()` 和内核虚实地址偏移。使用第一份实现的实际块尺寸、原请求记录及分阶链表约定，不能假设 Buddy 与 Best-Fit 支持相同的部分释放方式。

### [GUARANTEE]

测试须在真实 QEMU 中执行，不能把文字输出当作验证。每组 PASS 前必须有对应断言。先借用一块 64 页对齐区域，暂存原空闲链表并屏蔽暂存块头的空闲标志，避免测试区域与外部块发生合并。测试结束恢复原链表、标志及借用区域，确认全部真实空闲页数恢复。不得修改现有官方测试或为通过测试放宽分配器约束。

### [SPECIFICATION]

覆盖八组测试：不对齐起点和 61 页非二次幂长度的登记、耗尽与回收；3 页和 5 页申请的向上取整、实际占用计数及内存写入隔离；单页分裂和递归合并回 64 页；不同阶或伙伴仍占用时不提前合并；交替释放 32 个单页后无法满足 2 页连续请求；0、超大及整数上界请求失败且不改变状态；以学号 2413581 为种子的 512 次混合操作，逐步验证范围、对齐、无重叠及计数；最后恢复全部生产内存。另检查双向链表结构、空闲块覆盖不重复、各阶无未合并伙伴及空闲页总数守恒。输出各组结果和统一成功标记，报告只记录本轮实际运行数据。
