# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|---|---|
| **实验名称** | Lab2：物理内存和页表 |
| **小组成员** | 2412771-谭锦睿、2413581-黄奕浩、2411302-王乐之 |
| **完成日期** | 2026-XX-XX |
| **实验分支** | `lab2` |
| **代码目录** | `code/` |
| **报告目录** | `report/` |

---

## 小组分工

### 练习分工

| 成员 | 负责内容 |
|---|---|
| **2412771-谭锦睿** | **功能模块一全部内容 + 功能模块二任务一、任务二**。负责物理内存探测与页描述符初始化；理解并分析 First-Fit；设计 Prompt 并完成完整 Best-Fit 分配器实现及 `pmm_manager` 切换；负责整体代码整合与基础测试。 |
| **2413581-黄奕浩** | **功能模块二任务三：Buddy System（伙伴系统）**。负责设计两份 Prompt，生成 Buddy System 分配模块及对应测试函数，完成接入、运行和结果分析。 |
| **2411302-王乐之** | **功能模块二扩展任务 Challenge：SLUB 分配算法**。负责设计两份 Prompt，生成简化 SLUB 内存对象分配模块及对应测试函数，完成接入、运行和结果分析。 |
| **三人共同完成** | **分支任务：GDB 调试页表查询过程**。共同完成 Sv39 页表查询过程的调试、地址拆分、PTE 逐级查看、物理地址计算和截图记录。 |

### 任务覆盖检查

根据实验指导书，Lab2 当前需要覆盖的内容如下：

- [x] 功能模块一任务一：实现物理内存探测与页描述符初始化功能 —— 谭锦睿
- [ ] 功能模块一 Challenge：硬件可用物理内存范围的获取方法 —— 谭锦睿负责思考题整理；Challenge 为选做
- [ ] 功能模块二任务一：理解 First-Fit 连续物理内存分配算法 —— 谭锦睿
- [ ] 功能模块二任务二：实现 Best-Fit 连续物理内存分配算法 —— 谭锦睿
- [ ] 功能模块二任务三：Buddy System 分配算法 —— 黄奕浩
- [ ] 功能模块二扩展任务 Challenge：任意大小内存单元 SLUB 分配算法 —— 王乐之；Challenge 为选做
- [ ] 分支任务：GDB 调试页表查询过程 —— 三人共同完成

> 说明：功能模块一 Challenge 与功能模块二 SLUB Challenge 均属于加分项。为避免报告结构遗漏，本报告仍保留对应章节；若最终未完成，应在报告中明确写“未选做”，不要删除任务记录后造成任务覆盖不清。

### 报告分工

| 成员 | 报告负责内容 |
|---|---|
| **2412771-谭锦睿** | 实验整体逻辑、功能模块一、功能模块一 Challenge、功能模块二任务一、任务二、First-Fit 与 Best-Fit 对比、基础测试与 Git 整合 |
| **2413581-黄奕浩** | 功能模块二任务三 Buddy System：两份 Prompt、算法实现、测试设计、运行结果与分析 |
| **2411302-王乐之** | 功能模块二 SLUB Challenge：两份 Prompt、算法实现、测试设计、运行结果与分析 |
| **三人共同完成** | GDB 页表查询分支任务、Sv39 地址转换分析、调试记录与截图整理 |

> 最终报告以真实代码、实际 Git commit、QEMU/GDB 运行结果和实际使用的 Prompt 为准，不虚构尚未完成的实验结果。

---

# 一、实验目的

本实验主要围绕物理内存的发现、初始化、页级分配以及页表查询展开。实验目标如下：

1. 理解 RISC-V Sv39 页表和虚拟地址到物理地址的转换过程。
2. 理解操作系统如何通过 DTB 获取机器的物理内存范围。
3. 理解 `struct Page`、`pages`、`npage`、`nbase` 和 `va_pa_offset` 等数据结构与地址之间的关系。
4. 完成物理内存探测、页描述符初始化和初始空闲页区间建立。
5. 理解 First-Fit 连续物理页分配算法的初始化、分配、释放和空闲块合并过程。
6. 在 First-Fit 基础上实现 Best-Fit 连续物理页分配器。
7. 通过 Buddy System 与 SLUB 扩展任务进一步理解页级分配和任意大小内存对象分配。
8. 使用 QEMU、GDB、`make grade` 和 Git 对实现进行调试、验证与协作开发。
9. 在 AI 辅助开发过程中学习将实验要求写成明确的 Prompt/Spec，并对 AI 生成代码进行人工检查与验证。

---

# 二、实验环境

## 2.1 系统与开发环境

| 项目 | 环境 |
|---|---|
| 主机环境 | Windows + WSL2 Ubuntu 24.04 |
| 目标架构 | RISC-V 64 |
| 交叉编译工具链 | `riscv64-unknown-elf-*` |
| 虚拟机 | QEMU RISC-V `virt` |
| 固件 | OpenSBI |
| 调试工具 | RISC-V GDB |
| 版本管理 | Git + GitHub |
| 实验分支 | `lab2` |

## 2.2 AI 工具

| 成员 | AI 编程工具 | 底层模型 | 用途 |
|---|---|---|---|
| 2412771-谭锦睿 | [待填写] | [待填写] | 物理内存初始化、First-Fit 分析、Best-Fit Prompt 与代码检查 |
| 2413581-黄奕浩 | [待填写] | [待填写] | Buddy System 模块与测试 |
| 2411302-王乐之 | [待填写] | [待填写] | SLUB 模块与测试 |

---

# 三、实验整体逻辑分析

## 3.1 实验主线

本实验的整体流程可以概括为：

```text
QEMU / OpenSBI
        ↓
entry.S 保存 hartid / DTB 地址并建立启动地址映射
        ↓
kern_init()
        ↓
dtb_init()
        ↓
从设备树获取物理内存范围
        ↓
pmm_init()
        ↓
init_pmm_manager()
        ↓
page_init()
        ↓
建立 Page[]、保留已用区域、登记初始空闲页
        ↓
First-Fit 代码阅读与理解
        ↓
Best-Fit 实现并切换 pmm_manager
        ↓
Buddy System 扩展实现
        ↓
SLUB Challenge
        ↓
QEMU / make grade / 自定义测试
        ↓
三人共同完成 GDB Sv39 页表查询
```

实验指导书给出的核心调用链为：

```text
pmm_init()
    ↓
init_pmm_manager()
    ↓
page_init()
    ↓
init_memmap()
    ↓
alloc_pages() / free_pages()
```

这条链把“发现物理内存”“组织物理页”“登记空闲页”“分配页面”“释放页面”连接成一个完整的物理内存管理过程。

## 3.2 关键数据结构

- `struct Page`：描述一个物理页的元数据。
- `pages`：`Page` 描述符数组的起始地址。
- `npage`：当前可管理物理内存上界对应的物理页号。
- `nbase`：DRAM 起始物理页号。
- `va_pa_offset`：内核虚拟地址与物理地址之间的偏移。
- `free_area_t`：保存空闲块链表和空闲页总数。
- `free_list`：连续空闲物理页块的链表。
- `nr_free`：当前空闲物理页总数。
- `property`：对空闲块头而言，表示该连续空闲块包含的页数。
- `PG_reserved`：页面是否被保留、不可由普通分配器使用。
- `PG_property`：该 `Page` 是否为空闲连续块的块头。
- `pmm_manager`：通过函数指针统一抽象不同物理页面分配算法。

---

# 四、实验内容与实现

# 4.1 功能模块一：物理内存初始化功能

## 4.1.1 任务一：实现物理内存探测与页描述符初始化功能

**负责人：2412771-谭锦睿**

### 任务要求

实现：

```c
static void page_init(void);
```

所在文件：

```text
kern/mm/pmm.c
```

该函数需要：

1. 从设备树获取物理内存起始地址和大小；
2. 设置虚实地址映射偏移；
3. 计算可管理物理页范围；
4. 在内核 `end` 之后建立 `Page` 描述符数组；
5. 保证 OpenSBI、内核镜像以及 `Page[]` 描述符数组占用的空间不会被作为空闲页分配；
6. 对页面边界进行正确的向上/向下对齐；
7. 将剩余完整物理页通过 `init_memmap()` 交给当前 `pmm_manager`；
8. 当设备树信息无效时终止初始化；当不存在完整可用页时不调用 `init_memmap()`。

### 主要涉及文件

```text
kern/driver/dtb.c
kern/driver/dtb.h
kern/mm/pmm.c
kern/mm/pmm.h
kern/mm/memlayout.h
kern/mm/mmu.h
tools/kernel.ld
```

### 实现思路

当前实现的主要流程为：

```text
get_memory_base() / get_memory_size()
        ↓
得到 DTB 提供的 DRAM 范围
        ↓
设置 va_pa_offset
        ↓
计算并向下对齐 managed_end
        ↓
计算 npage 与 Page 描述符数量
        ↓
pages = ROUNDUP(end, PGSIZE)
        ↓
PADDR(pages) 得到描述符数组对应的物理地址
        ↓
初始化 Page[] 并默认全部设为 Reserved
        ↓
跳过 Page[] 实际占用空间
        ↓
向上对齐得到 free_start
        ↓
init_memmap(pa2page(free_start), free_pages)
```

当前代码还增加了对无效内存范围、地址加法溢出、描述符数组越界等情况的检查。

### 关键理解

`Page[]` 并不是真实物理页本身，而是“每个物理页对应的一条管理记录”。例如，一个 `Page` 描述符可以记录该物理页是否保留、引用计数、空闲块长度和链表节点。

`Page[]` 本身也存放在实际内存中，所以它占用的物理空间不能再次被交给页面分配器。初始可分配空间必须从描述符数组结束位置之后的下一个完整页开始。

### 最终 Prompt

> 此处在最终整理报告时放入实际成功使用的精简版四段式 Prompt。开发阶段使用的完整 Prompt 统一保存在 `report/prompt.md`。

```markdown
[PROMPT]
实现 kern/mm/pmm.c 中的 page_init()，完成 DTB 物理内存探测结果到 Page 描述符数组和初始空闲页区间的建立，只修改功能模块一相关代码。

[RELY]
使用当前工程真实的 DTB 接口、struct Page、pages/npage/nbase/va_pa_offset、PADDR/pa2page、PGSIZE、页面对齐宏和 init_memmap()。

[GUARANTEE]
正确设置 va_pa_offset、npage、pages；保留内核和 Page[] 占用区域；只将完整可用页交给当前 pmm_manager。

[SPECIFICATION]
正确处理 DTB 范围、地址转换、页对齐、Page 状态初始化、无效内存范围和不存在完整空闲页等情况。
```

### 当前验证状态

已完成：

- [x] `page_init()` 实现
- [x] `git diff --check`
- [x] `pmm.o` 单独编译通过
- [x] 与 `pmm.h` 中 `pa2page()` / `page2pa()` 的索引关系一致
- [ ] 完整 Lab2 工程最终运行验证
- [ ] `[Test 1] Page Init: PASS`
- [ ] `make grade` 最终验证

> 说明：完整运行验证需要等 Best-Fit 等后续任务接入后统一进行，未完成前不提前填写 PASS。

---

## 4.1.2 Challenge：硬件的可用物理内存范围的获取方法

**负责人：2412771-谭锦睿**

该 Challenge 为选做思考题：

> 如果 OS 无法提前知道当前硬件的可用物理内存范围，有什么办法让 OS 获取可用物理内存范围？

### 思考方向

可以结合以下方法分析：

- 固件或 Bootloader 向内核传递内存布局；
- Device Tree（DTB）描述硬件资源；
- BIOS/UEFI 提供内存映射；
- 平台特定的固件接口；
- 启动阶段进行硬件探测。

### 最终回答

[若选做，在实验完成后填写；若未选做，明确写“本组未选做该 Challenge”。]

---

# 4.2 功能模块二：物理内存分配功能

## 4.2.1 任务一：理解 First-Fit 连续物理内存分配算法

**负责人：2412771-谭锦睿**

### 任务性质

本任务是**思考与代码阅读任务，不需要修改源码**。

重点阅读：

```text
kern/mm/default_pmm.c
kern/mm/pmm.c
kern/mm/pmm.h
kern/mm/memlayout.h
libs/list.h
```

重点函数：

```c
default_init()
default_init_memmap()
default_alloc_pages()
default_free_pages()
```

### First-Fit 初始化

`default_init()` 初始化空闲链表并将空闲页数设为 0：

```text
free_list = 空链表
nr_free = 0
```

`default_init_memmap(base, n)` 接收功能模块一传来的连续空闲页，将这些页的状态初始化，并把 `base` 作为连续空闲块头：

```text
base->property = n
PG_property(base) = 1
nr_free += n
```

块头通过 `page_link` 接入 `free_list`。

### First-Fit 分配过程

`default_alloc_pages(n)` 从 `free_list` 开头顺序扫描，找到第一个满足：

```c
p->property >= n
```

的空闲块后立即停止，因此称为 First-Fit。

若该块大小大于 `n`，则：

```text
原空闲块
    ↓
前 n 页被分配
剩余页重新形成一个空闲块
```

随后更新：

```text
property
PG_property
free_list
nr_free
```

### First-Fit 释放与合并

`default_free_pages(base, n)` 将释放区域重新初始化为空闲状态，再按物理地址顺序插回链表。

如果释放块与前一个空闲块满足：

```text
previous + previous->property == base
```

说明两者物理连续，可以合并。

如果与后一个空闲块满足：

```text
base + base->property == next
```

同样进行合并。

### First-Fit 是否还有改进空间

[实验完成后整理正式答案。]

建议从以下角度回答：

1. 顺序链表查找最坏情况下需要扫描多个空闲块；
2. 长期运行后可能形成外部碎片；
3. 可以改用 Best-Fit、Worst-Fit、Buddy System 等策略；
4. 可以使用按大小分类的链表、树等数据结构提高查找效率；
5. 任何改进都需要在查找开销、碎片率、合并效率和实现复杂度之间权衡。

---

## 4.2.2 任务二：实现 Best-Fit 连续物理内存分配算法

**负责人：2412771-谭锦睿**

### 任务要求

参考：

```text
kern/mm/default_pmm.c
```

在：

```text
kern/mm/best_fit_pmm.c
```

中 `lab2` / `YOUR CODE` 位置实现完整 Best-Fit 物理内存管理模块。

该文件只提供：

```c
basic_check()
best_fit_check()
```

两个框架测试函数，不提供完整管理函数声明和 `best_fit_pmm_manager` 注册结构。

需要自行补充符合 `struct pmm_manager` 接口的完整 Best-Fit 管理器，包括：

- 内部空闲区域状态；
- 初始化；
- 初始空闲区登记；
- 连续页面分配；
- 页面释放；
- 相邻空闲块合并；
- 空闲页计数；
- `best_fit_pmm_manager` 注册结构。

同时在：

```text
kern/mm/pmm.c
```

的 `init_pmm_manager()` 中将：

```c
default_pmm_manager
```

切换为：

```c
best_fit_pmm_manager
```

否则运行和检查的仍然是 First-Fit。

### Best-Fit 核心策略

与 First-Fit 的区别：

```text
First-Fit：
找到第一个能够满足请求的块立即使用

Best-Fit：
扫描所有满足请求的候选块
选择其中大小最小的那个
```

例如：

```text
空闲块：2 页 → 8 页 → 5 页 → 20 页
申请：4 页
```

First-Fit 选择 8 页块，而 Best-Fit 应选择 5 页块。

### 必须维护的状态

实现过程中需要正确维护：

```text
free_list
nr_free
Page.property
PG_property
page_link
```

释放页面时，还需要正确实现前后相邻空闲块合并。

### 最终 Prompt

> 最终报告中放入实际成功使用并通过测试的精简版 Prompt。完整开发 Prompt 保存在 `report/prompt.md`。

```markdown
[PROMPT]
读取当前工程真实代码，在 best_fit_pmm.c 的 YOUR CODE 位置实现完整 Best-Fit PMM，并在 pmm.c 中切换当前管理器为 best_fit_pmm_manager。不得修改 basic_check() 与 best_fit_check()。

[RELY]
参考 default_pmm.c、pmm.h、memlayout.h、list.h，以及现有 basic_check()/best_fit_check() 的状态约定。

[GUARANTEE]
实现 Best-Fit 所需 init/init_memmap/alloc_pages/free_pages/nr_free_pages/check 相关功能和 best_fit_pmm_manager 注册结构，并正确接入 pmm_manager。

[SPECIFICATION]
分配时选择所有可用候选块中尺寸最小者；必要时分割剩余块；释放时按地址插回并合并相邻块；正确维护 property、PG_property、free_list 和 nr_free。
```

### 验证要求

- [ ] `best_fit_pmm.c` 编译通过
- [ ] 不修改 `basic_check()`
- [ ] 不修改 `best_fit_check()`
- [ ] `pmm_manager` 已切换为 `best_fit_pmm_manager`
- [ ] 页面分配符合 Best-Fit 策略
- [ ] 页面释放与相邻块合并正确
- [ ] `nr_free` 正确
- [ ] `[Test 2] Best-Fit: PASS`
- [ ] `make grade` 通过

### Best-Fit 是否还有改进空间

[完成后填写。]

建议讨论：

- 线性扫描全部空闲块导致查找开销较大；
- 可以使用按块大小排序的数据结构；
- Best-Fit 可能产生较多很小的碎片；
- 可以使用平衡树、分级链表或 Buddy System 等结构改善查找和合并效率。

---

## 4.2.3 任务三：Buddy System（伙伴系统）分配算法

**负责人：2413581-黄奕浩**

### 任务要求

本任务没有现成 Buddy System 代码框架和测试函数。

需要设计**两份 Prompt**：

1. Prompt A：让 AI 参考现有 uCore PMM 框架，从零生成 Buddy System 分配模块并接入内核；
2. Prompt B：针对 Prompt A 生成的实现设计测试函数。

最终代码需要正常编译运行。

### Buddy System 核心思想

Buddy System 将空闲内存划分成大小为：

```text
1, 2, 4, 8, 16, 32, ...
```

页的 2 的幂次块。

分配时：

```text
寻找最小可满足块
        ↓
若过大则不断二分
        ↓
直到得到合适大小
```

释放时：

```text
找到对应 buddy
        ↓
若 buddy 同样空闲且大小相同
        ↓
合并成更大块
        ↓
继续向上尝试合并
```

### Prompt A：Buddy 实现

```markdown
[待填写最终实际使用 Prompt]
```

### Prompt B：Buddy 测试

```markdown
[待填写最终实际使用 Prompt]
```

### 测试覆盖

测试至少应覆盖：

- 初始化；
- 单页分配；
- 多页分配；
- 非 2 的幂大小请求如何处理；
- 块分裂；
- buddy 合并；
- 多次分配和释放；
- 内存不足；
- 释放后的再次分配；
- 空闲页统计是否正确。

### 最终结果

- [ ] Buddy 模块生成
- [ ] 接入现有 PMM 框架
- [ ] 测试函数完成
- [ ] 编译通过
- [ ] 关键分裂行为正确
- [ ] 关键合并行为正确
- [ ] 测试结果记录完成

---

## 4.2.4 Challenge：任意大小的内存单元 SLUB 分配算法

**负责人：2411302-王乐之**

### 任务要求

本 Challenge 要求实现两层结构：

```text
第一层：页级物理内存分配
        ↓
第二层：在页面内部划分任意大小对象
```

允许进行简化，只需能够体现 SLUB 的主体思想。

同样需要设计两份 Prompt：

1. Prompt A：从零生成简化 SLUB 分配模块并接入现有内核；
2. Prompt B：针对该实现生成测试函数。

### Prompt A：SLUB 实现

```markdown
[待填写最终实际使用 Prompt]
```

### Prompt B：SLUB 测试

```markdown
[待填写最终实际使用 Prompt]
```

### 测试覆盖

至少应考虑：

- 不同对象大小；
- 同一 slab 中多对象分配；
- 多 slab 情况；
- 对象释放；
- 释放后重新利用；
- 页级资源的申请和归还；
- 边界大小；
- 内存不足；
- 多次交错分配与释放。

### 最终结果

- [ ] SLUB 模块生成
- [ ] 接入现有页级分配器
- [ ] 测试函数完成
- [ ] 不同大小对象分配正确
- [ ] 对象释放正确
- [ ] 测试结果记录完成

> 若最终未选做，应在本节明确记录“本组未选做该 Challenge”。

---

# 4.3 First-Fit 与 Best-Fit 对比分析

**负责人：2412771-谭锦睿**

实验完成后结合 `default_pmm.c` 和最终 `best_fit_pmm.c` 分析：

| 对比项 | First-Fit | Best-Fit |
|---|---|---|
| 选择策略 | 第一个满足请求的空闲块 | 所有满足请求块中尺寸最小者 |
| 搜索范围 | 找到后立即停止 | 通常需要继续扫描所有候选 |
| 查找开销 | 通常较低 | 线性链表下通常更高 |
| 剩余空间 | 可能留下较大的剩余块 | 单次分配留下较小剩余块 |
| 碎片特点 | 可能产生外部碎片 | 也可能产生大量很小的碎片 |
| 改进方向 | 更好的索引结构/其他策略 | 按大小组织结构、Buddy 等 |

[最终实验完成后结合实际代码进一步补充。]

---

# 4.4 分支任务：GDB 调试页表查询过程

**负责人：三人共同完成**

本任务使用 QEMU + GDB 实际观察 RISC-V Sv39 页表查询过程。

## 4.4.1 分工建议

为了保证“三人共同完成”在报告和 Git 记录中有实际体现，可按以下方式协作：

- 谭锦睿：负责调试环境、`satp`、根页表地址和目标虚拟地址选择；
- 黄奕浩：负责 VPN2/VPN1/VPN0 拆分、逐级 PTE 查询和 GDB 命令记录；
- 王乐之：负责最终物理地址计算、结果验证和截图整理；
- 三人共同检查完整查询链并统一报告内容。

> 上述仅作为协作建议，最终以三人实际调试记录为准。

## 4.4.2 调试目标

需要记录：

1. QEMU Debug 模式启动方法；
2. GDB 连接方法；
3. `satp` 寄存器值；
4. 如何根据 `satp.MODE` 判断 Sv39；
5. 根页表物理地址；
6. 选择一个代表性虚拟地址；
7. 拆分 VPN2、VPN1、VPN0 和 page offset；
8. 一级一级读取 PTE；
9. 根据 PTE 得到下一级页表地址或最终物理页；
10. 得到最终物理地址；
11. 与 QEMU/MMU 实际转换结果进行验证。

## 4.4.3 调试记录

```text
[填写实际 GDB 命令和输出]
```

## 4.4.4 结果截图

```markdown
![satp与根页表](./images/gdb_satp.png)

![三级页表查询过程](./images/gdb_page_walk.png)

![最终地址验证](./images/gdb_pa_verify.png)
```

## 4.4.5 最终结果

- [ ] 获取并解释 `satp`
- [ ] 确认 Sv39
- [ ] 找到根页表
- [ ] 拆分 VPN2/VPN1/VPN0/offset
- [ ] 完成三级页表查询
- [ ] 得到最终物理地址
- [ ] 验证地址转换结果
- [ ] 保存必要截图
- [ ] 三名成员均参与并理解完整过程

---

# 五、测试与验证

## 5.1 编译

```bash
cd ~/os-lab/nku-os/code
make clean
make
```

记录：

```text
[最终实际编译输出]
```

## 5.2 QEMU 运行

```bash
make qemu
```

需要关注：

- DTB 探测到的物理内存范围；
- 当前物理内存管理器名称；
- `check_alloc_page()` 是否成功；
- Page Init 测试结果；
- Best-Fit 测试结果；
- 页表虚拟/物理地址；
- Buddy/SLUB 若接入后的运行结果。

## 5.3 自动评分

```bash
make grade
```

基础评分重点：

```text
[Test 1] Page Init: PASS
[Test 2] Best-Fit: PASS
```

最终截图：

```markdown
![make grade结果](./images/grade_result.png)
```

## 5.4 Buddy System 自定义测试

```text
[填写测试命令、输出和结论]
```

## 5.5 SLUB 自定义测试

```text
[若选做，填写测试命令、输出和结论]
```

---

# 六、Git 提交与协作

Lab2 三名成员统一在 `lab2` 分支协作。

每次开始工作前：

```bash
git pull --ff-only origin lab2
```

完成一个独立任务后进行小粒度提交。

## 6.1 主要 Commit

| 成员 | 主要 Commit | 内容 |
|---|---|---|
| 谭锦睿 | [待填写] | 功能模块一 `page_init()` |
| 谭锦睿 | [待填写] | 功能模块二任务二 Best-Fit |
| 黄奕浩 | [待填写] | Buddy System 模块与测试 |
| 王乐之 | [待填写] | SLUB Challenge 模块与测试 |
| 三人共同 | [待填写] | GDB 页表查询记录 / 报告整合 |

> 功能模块二任务一属于阅读和思考题，不一定产生代码 commit，应在报告中体现分析成果。

---

# 七、实验总结与收获

## 7.1 物理内存初始化

[实验完成后填写。]

重点总结：

- DTB 如何向内核提供物理内存信息；
- `Page` 描述符与实际物理页的关系；
- 内核、`Page[]` 和真正空闲页之间的边界；
- 为什么页边界处理非常重要。

## 7.2 页面分配算法

[实验完成后填写。]

重点总结：

- First-Fit；
- Best-Fit；
- Buddy System；
- 三种算法在碎片、搜索效率和合并行为上的区别。

## 7.3 SLUB 与页级分配的关系

[若选做 SLUB Challenge 后填写。]

重点说明：

```text
页级分配器
    ↓
提供若干页面
    ↓
SLUB 在页面内部进一步划分对象
```

以及为什么操作系统既需要页级分配器，也需要小对象分配器。

## 7.4 Sv39 页表理解

[完成 GDB 分支任务后填写。]

重点总结：

- `satp`；
- VPN2 / VPN1 / VPN0；
- PTE；
- 页表逐级查询；
- page offset；
- 最终物理地址计算；
- TLB 与页表的关系。

## 7.5 AI 协作开发经验

结合真实实验过程总结：

- 如何把指导书要求转换成四段式 Prompt；
- 为什么 `[RELY]` 必须来自当前真实工程；
- 为什么 `[GUARANTEE]` 要明确交付接口；
- 为什么 `[SPECIFICATION]` 应描述行为而不是直接给代码答案；
- 为什么 AI 修改后还要使用 `git diff`、编译、测试和 GDB 人工验证；
- 三人协作时如何划分代码边界，避免覆盖他人修改。

---

# 八、最终任务检查清单

## 功能模块一

- [x] `page_init()` 已实现
- [ ] Page Init 最终运行验证
- [ ] Challenge 思考题（选做）

## 功能模块二

### 任务一 First-Fit

- [ ] 阅读 `default_init`
- [ ] 阅读 `default_init_memmap`
- [ ] 阅读 `default_alloc_pages`
- [ ] 阅读 `default_free_pages`
- [ ] 完成 First-Fit 报告分析
- [ ] 回答 First-Fit 改进空间

### 任务二 Best-Fit

- [ ] 完整 Prompt
- [ ] `best_fit_pmm.c` 完整实现
- [ ] `best_fit_pmm_manager`
- [ ] `pmm.c` 切换到 Best-Fit
- [ ] `basic_check()` / `best_fit_check()` 不修改
- [ ] 回答 Best-Fit 改进空间
- [ ] `[Test 2] Best-Fit: PASS`

### 任务三 Buddy System

- [ ] Buddy 实现 Prompt
- [ ] Buddy 测试 Prompt
- [ ] Buddy 模块
- [ ] Buddy 测试函数
- [ ] 测试覆盖说明
- [ ] 编译运行通过

### SLUB Challenge

- [ ] SLUB 实现 Prompt
- [ ] SLUB 测试 Prompt
- [ ] SLUB 模块
- [ ] SLUB 测试函数
- [ ] 测试覆盖说明
- [ ] 编译运行通过

## 分支任务

- [ ] 三人共同完成 GDB 调试
- [ ] `satp` 分析
- [ ] 根页表定位
- [ ] VPN 拆分
- [ ] 三级页表逐级查询
- [ ] 最终物理地址验证
- [ ] 调试截图
