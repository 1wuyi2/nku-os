# SLUB 实现 Prompt

本文件从本次总任务整理，为同一轮连续开发的实现规格；没有发生两次独立的模型调用。原始总任务见 `evidence/original-task.txt`，后续调整见 `development.md`。

## [PROMPT]

请直接在当前 RISC-V 64 uCore Lab2 工程实现简化 SLUB 对象分配器，完成编译、接入和说明。通过现有页级 PMM 获取内存，再在页面内划分小对象。新增 `code/kern/mm/slub.c`、`slub.h`，在内核 PMM 初始化完成后初始化 SLUB。

## [RELY]

先阅读 `code/kern/mm/pmm.c`、`pmm.h`、`memlayout.h`、`mmu.h`、`default_pmm.c`、`best_fit_pmm.c`、`buddy_pmm.c`、`code/kern/init/init.c`、`entry.S`、`code/Makefile`、`code/libs/list.h` 和现有报告。复用 `alloc_pages(n)`、`free_pages(page,n)`、`nr_free_pages()`、`page2pa()`、`va_pa_offset`、`assert()` 和内核链表设施。当前 `Page` 的 flags、property 和 page_link 都由 PMM 使用；Buddy 将页数向二次幂取整，释放接口仍核对原始请求页数。Makefile 自动收集 `kern/mm` 的 C 文件，支持 `DEFS`。

## [GUARANTEE]

保留 `page_init()`、默认 Best-Fit、`USE_BUDDY`、所有现有 PMM 实现及其测试、官方评分脚本、启动汇编和链接脚本。SLUB 作为 PMM 上层，不替换 `pmm_manager`，不借用或修改 `Page` 元数据，不使用用户态分配函数。默认构建不执行 SLUB 自测。保留其他成员报告与 Prompt，不制造 Git 历史、测试结果或截图。实现面向现有单核启动环境，明确并发限制。每个活动对象须正确对齐、互不重叠，失败申请不改变现有对象；所有对象释放后归还全部底层页。

## [SPECIFICATION]

实现 `slub_init()`、`slub_alloc(size_t)`、返回布尔成功状态的 `slub_free(void *)`、`slub_usable_size()`、`slub_get_stats()` 和 `slub_validate()`。初始化可重复调用且不丢失活动对象。采用 16、32、64、128、256、512、1024、2048 字节八种缓存，任意小请求向上匹配，地址至少 16 字节对齐。

每个小 Slab 占一个 PMM 页，在页头放置独立 `struct slab`，保存活动块链表节点、缓存状态链表节点、缓存指针、PMM 页指针、请求页数、实际占用页数、容量、使用计数、空闲对象头和 256 位分配位图。页头按 16 字节向上对齐，剩余空间建立对象槽位，空闲对象首字保存下一节点。缓存分别维护 partial/full 链表和 Slab、对象计数。空闲页是暂态，最后一个对象释放时立即摘除元数据并 `free_pages()`。

超过 2048 字节的请求走独立大对象路径，安全计算页头加请求并向页大小取整，保存原始页数；通过 PMM 前后空闲页差值记录实际扣除页数以兼容 Buddy。没有额外固定 64 KiB 上限，申请能否成功取决于算术范围及连续可用物理内存。检查零长度、整数溢出和空间不足，失败返回 NULL。

释放前遍历可信活动块登记表，以精确槽位地址和位图核对指针；大对象只允许精确起始地址。NULL 返回成功，无效、内部、页头及当前已释放的指针返回失败，拒绝时不修改链表。已释放地址被再次分配后，旧指针与新指针相同的情形属于裸指针接口的限制，应在报告说明。

一致性检查应交叉核对双向链表、活动块登记、各缓存计数、位图与空闲链表的互补性、full/partial 状态、对象边界、块间不重叠、PMM 页状态及实际占用页数。实际编译修复后，再按测试规格执行 QEMU 验证和默认官方评分回归。
