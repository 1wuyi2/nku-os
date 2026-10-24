# Lab2 AI Prompt 记录

本文件汇总本实验各成员使用的 Prompt，包含物理内存初始化、Best-Fit、Buddy System 和 SLUB 的实现与测试规格。First-Fit 任务为源码阅读与思考题，不单独记录 Prompt。

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

负责人：2413581-黄奕浩。以下按四段式格式，分别记录分配模块与测试函数的提示词。

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

# Lab2 SLUB 提示词记录

负责人：2411302-王乐之。本次采用一份总任务连续完成实现和测试，以下两份四段式子规格据此整理，不声称存在两次独立的模型调用。

## 1. SLUB 分配模块

### [PROMPT]

请直接在当前 RISC-V 64 uCore Lab2 工程实现简化 SLUB 对象分配器，完成编译、接入和说明。通过现有页级 PMM 获取内存，再在页面内划分小对象。新增 `code/kern/mm/slub.c`、`slub.h`，在内核 PMM 初始化完成后初始化 SLUB。

### [RELY]

先阅读 `code/kern/mm/pmm.c`、`pmm.h`、`memlayout.h`、`mmu.h`、`default_pmm.c`、`best_fit_pmm.c`、`buddy_pmm.c`、`code/kern/init/init.c`、`entry.S`、`code/Makefile`、`code/libs/list.h` 和现有报告。复用 `alloc_pages(n)`、`free_pages(page,n)`、`nr_free_pages()`、`page2pa()`、`va_pa_offset`、`assert()` 和内核链表设施。当前 `Page` 的 flags、property 和 page_link 都由 PMM 使用；Buddy 将页数向二次幂取整，释放接口仍核对原始请求页数。Makefile 自动收集 `kern/mm` 的 C 文件，支持 `DEFS`。

### [GUARANTEE]

保留 `page_init()`、默认 Best-Fit、`USE_BUDDY`、所有现有 PMM 实现及其测试、官方评分脚本、启动汇编和链接脚本。SLUB 作为 PMM 上层，不替换 `pmm_manager`，不借用或修改 `Page` 元数据，不使用用户态分配函数。默认构建不执行 SLUB 自测。保留其他成员报告与 Prompt，不制造 Git 历史、测试结果或截图。实现面向现有单核启动环境，明确并发限制。每个活动对象须正确对齐、互不重叠，失败申请不改变现有对象；所有对象释放后归还全部底层页。

### [SPECIFICATION]

实现 `slub_init()`、`slub_alloc(size_t)`、返回布尔成功状态的 `slub_free(void *)`、`slub_usable_size()`、`slub_get_stats()` 和 `slub_validate()`。初始化可重复调用且不丢失活动对象。采用 16、32、64、128、256、512、1024、2048 字节八种缓存，任意小请求向上匹配，地址至少 16 字节对齐。

每个小 Slab 占一个 PMM 页，在页头放置独立 `struct slab`，保存活动块链表节点、缓存状态链表节点、缓存指针、PMM 页指针、请求页数、实际占用页数、容量、使用计数、空闲对象头和 256 位分配位图。页头按 16 字节向上对齐，剩余空间建立对象槽位，空闲对象首字保存下一节点。缓存分别维护 partial/full 链表和 Slab、对象计数。空闲页是暂态，最后一个对象释放时立即摘除元数据并 `free_pages()`。

超过 2048 字节的请求走独立大对象路径，安全计算页头加请求并向页大小取整，保存原始页数；通过 PMM 前后空闲页差值记录实际扣除页数以兼容 Buddy。没有额外固定 64 KiB 上限，申请能否成功取决于算术范围及连续可用物理内存。检查零长度、整数溢出和空间不足，失败返回 NULL。

释放前遍历可信活动块登记表，以精确槽位地址和位图核对指针；大对象只允许精确起始地址。NULL 返回成功，无效、内部、页头及当前已释放的指针返回失败，拒绝时不修改链表。已释放地址被再次分配后，旧指针与新指针相同的情形属于裸指针接口的限制，应在报告说明。

一致性检查应交叉核对双向链表、活动块登记、各缓存计数、位图与空闲链表的互补性、full/partial 状态、对象边界、块间不重叠、PMM 页状态及实际占用页数。实际编译修复后，再按测试规格执行 QEMU 验证和默认官方评分回归。

## 2. SLUB 内核测试

### [PROMPT]

请为当前工程已经实现的简化 SLUB 编写、运行并分析充分的内核级自测，新增 `code/kern/mm/slub_test.c`，由 `SLUB_SELFTEST` 控制编译和调用。所有 PASS 必须在对应断言通过后打印，全部测试结束后输出 `[SLUB] ALL TESTS PASSED`。在终端执行编译、QEMU 与官方评分，并将实际结果及测试截图整理到报告。

### [RELY]

阅读真实 `slub.c`、`slub.h`、`pmm.c`、`buddy_pmm.c`、初始化调用链和 Makefile。使用 `slub_alloc/free/usable_size/get_stats/validate` 及 `nr_free_pages()`；释放接口返回成功/失败，NULL 为成功无操作。对象最小对齐为 16 字节，小缓存上限为 2048 字节，更大请求带页头申请连续 PMM 页。小对象的空闲 Slab 立即归还 PMM。`slub_validate()` 断言内部状态，`slub_get_stats().backing_pages` 包括 Buddy 的实际二次幂取整页数。

### [GUARANTEE]

不得修改 PMM、官方测试、启动汇编和链接逻辑来迎合自测；默认配置不运行自测。必须在 QEMU 4.1.1 中实际启动内核。各组测试结束后释放所有资源，空闲页计数恢复初值。内存不足场景通过真实借用 PMM 页构造，不伪造分配器内部状态。在报告中说明实际失败与修复，不能用预期输出替代真实运行。切换编译宏前执行 `make clean`，完成后恢复默认 Best-Fit 构建。

### [SPECIFICATION]

实现十一组测试：

1. 初始化：检查八种缓存、空链表、零统计及重复初始化。
2. 八个 16 字节对象：检查地址不同、无重叠、同一物理页，只消耗一页。
3. 多大小：覆盖八种标准大小及 20、100、300 字节，检查向上匹配、有效范围、对齐和活动对象期间重复初始化。
4. 数据隔离：逐对象写入不同模式，检查每个请求字节和完整槽位区间互不重叠。
5. 释放复用：在仍有活动对象的 Slab 中释放一个对象，拒绝再次释放，再申请必须复用该槽位；其他对象数据保持完整。
6. 跨 Slab：申请 600 个最小对象，检查第一个 Slab 装满后新页分配、full→partial→full 转换、多 Slab 空间不重复及全部回收。
7. 边界：零、`SIZE_MAX` 附近溢出、超物理内存请求；借用 PMM 页至真正耗尽，验证新缓存和大对象失败、已有 partial Slab 仍能分配、活动数据不变，最后归还所有借页。
8. 完整回收：八缓存同时分配、逆序释放，检查所有登记表为空、所有统计归零、空闲页恢复初值。
9. 压力：固定种子 2411302，64 个活动槽位，执行 1024 次混合操作，包含 1～8193 字节请求。每次操作前后检查地址、对齐、活动区间不重叠、逐字节数据、内部状态、对象统计及 `nr_free_pages()+backing_pages==初始页数`。
10. 大对象：同时申请 2049、4096、8193、65536 字节，写入并逐字节验证，检查大对象统计、内部指针拒绝、重复释放拒绝及实际页数回收。
11. 非法释放：NULL、低地址、栈地址、独立 PMM 页、槽位内部和页头地址；检查拒绝后页数与活动数据不变，最终全部回收。

分别执行默认 PMM+SLUB_SELFTEST、纯默认启动与 `make grade`、`USE_BUDDY+SLUB_SELFTEST`，要求官方评分维持 20/20、Buddy 原有八组测试及 SLUB 十一组测试全部真实通过。在报告中列出实际 Makefile 命令与结果，测试输出直接显示在终端。按课程格式交付 `report.md`、统一 `prompt.md` 和 `images/`，不增加日志目录、开发文档或用户态运行包装脚本。
