# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|---|---|
| 实验名称 | Lab4：进程管理 |
| 小组成员 | 2412771-谭锦睿、2413581-黄奕浩、2411302-王乐之 |
| 完成日期 | 2026-11-XX |
| 实验分支 | `lab4` |
| 代码目录 | `code/` |
| 报告目录 | `report/` |

---

## 小组分工

### 练习分工

| 成员 | 负责内容 |
|---|---|
| 2412771-谭锦睿 | Function 1：页面映射相关操作。负责 `page_insert()`、`page_remove()` 及其相关页表、页引用计数、TLB 刷新逻辑的实现和分析。 |
| 2413581-黄奕浩 | Function 2：进程 / 内核线程创建相关内容。 |
| 2411302-王乐之 | Function 3：进程调度与上下文切换相关内容。 |
| 三人共同完成 | 最终代码整合、编译运行、QEMU 测试、官方测试以及实验报告整理。 |

### 拓展练习分工

| 成员 | 拓展练习 |
|---|---|
| 2412771-谭锦睿 | Challenge 2：分析 Sv32、Sv39、Sv48 分页模式及 `get_pte()` 的设计。 |
| 2411302-王乐之 | Challenge 1：分析 `local_intr_save()` / `local_intr_restore()` 如何实现中断状态保存、关闭和恢复。 |
| 2413581-黄奕浩 | 无单独 Challenge，主要完成本人 Function 2 及对应报告。 |

### 任务完成情况

- [x] 恢复 Lab2 / Lab3 前置依赖
- [x] Function 1：完成 `page_insert()` / `page_remove()`
- [ ] Function 2：待对应成员完成
- [ ] Function 3：待对应成员完成
- [ ] 三人代码整合
- [ ] 完整编译
- [ ] QEMU 运行验证
- [ ] 官方测试
- [ ] 完善实验报告
- [ ] 整理完整 Prompt

### 报告分工

| 成员 | 报告负责内容 |
|---|---|
| 2412771-谭锦睿 | 页面映射、`page_insert()`、`page_remove()`、分页模式及 `get_pte()` Challenge |
| 2413581-黄奕浩 | Function 2 实际实现、代码分析、测试结果 |
| 2411302-王乐之 | Function 3 实际实现、中断保护 Challenge、测试结果 |
| 三人共同完成 | 整体测试、实验问题、最终总结 |

实际使用的 Prompt 统一整理在：

```text
report/prompt.md
```

---

# 一、实验目的

本实验在前面物理内存管理、页表以及 Trap 机制的基础上，进一步学习操作系统中的进程管理机制。

主要目标包括：

1. 理解页面映射建立、替换和删除过程；
2. 理解物理页引用计数的作用；
3. 理解进程控制块及进程创建的基本过程；
4. 理解进程调度与上下文切换机制；
5. 理解 Lab2、Lab3 中已经实现的内存管理和 Trap 机制如何继续为 Lab4 服务；
6. 通过编译和运行测试验证 Lab4 实现。

---

# 二、实验环境

| 项目 | 环境 |
|---|---|
| 主机环境 | Windows + WSL2 Ubuntu 24.04 |
| 目标架构 | RISC-V 64 |
| 交叉编译工具链 | `riscv64-unknown-elf-*` |
| 虚拟机 | QEMU |
| 固件 | OpenSBI |
| 版本管理 | Git |
| 实验分支 | `lab4` |

---

# 三、实验前置依赖恢复

Lab4 依赖 Lab2 和 Lab3 中已经实现的部分代码。

按照实验框架中的 `lab2` / `lab3` 注释进行了扫描，共找到：

```text
Lab2：2 处
Lab3：3 处
共涉及 4 个源码文件
```

涉及：

```text
code/kern/mm/pmm.c
code/kern/mm/default_pmm.c
code/kern/trap/trap.c
code/kern/trap/trapentry.S
```

恢复内容包括：

```text
Lab2：
page_init()
物理页 Best-Fit 分配器

Lab3：
interrupt_handler()
exception_handler()
RESTORE_ALL
__alltraps
```

迁移过程中保留了实验代码中原有的 Lab2 / Lab3 注释，没有修改 Lab4 新 Function。

恢复完成后：

```text
make clean
make
```

源码编译阶段可以通过。

此时剩余链接错误为：

```text
page_insert
page_remove
proc_init
cpu_idle
```

均属于 Lab4 尚未完成的 Function。

---

# 四、Function 1：页面映射管理

**负责人：2412771-谭锦睿**

## 4.1 任务要求

本部分主要实现：

```c
page_insert()
page_remove()
```

主要文件：

```text
code/kern/mm/pmm.c
```

需要正确处理：

- 建立新的虚拟地址到物理页的映射；
- 替换已有映射；
- 删除已有映射；
- 维护页面引用计数；
- 必要时释放物理页；
- 修改页表后刷新 TLB。

---

## 4.2 page_remove_pte()

`page_remove_pte()` 用于删除一个已经存在的页表映射。

核心代码：

```c
static void
page_remove_pte(pde_t *pgdir, uintptr_t la, pte_t *ptep)
{
    if (*ptep & PTE_V)
    {
        struct Page *page = pte2page(*ptep);
        if (page_ref_dec(page) == 0)
        {
            free_page(page);
        }
        *ptep = 0;
        tlb_invalidate(pgdir, la);
    }
}
```

主要流程：

```text
检查 PTE 是否有效
        ↓
找到对应物理页
        ↓
减少引用计数
        ↓
引用计数为 0 时释放物理页
        ↓
清除 PTE
        ↓
刷新 TLB
```

只有引用计数降为 0 时才能释放物理页，因为同一个物理页可能仍然被其他虚拟地址映射。

---

## 4.3 page_remove()

实现：

```c
void
page_remove(pde_t *pgdir, uintptr_t la)
{
    pte_t *ptep = get_pte(pgdir, la, 0);
    if (ptep != NULL)
    {
        page_remove_pte(pgdir, la, ptep);
    }
}
```

这里调用：

```c
get_pte(pgdir, la, 0)
```

其中 `create = 0`。

原因是该函数只是删除映射，如果页表本来不存在，就不应该为了执行删除操作而重新分配新的页表页。

---

## 4.4 page_insert()

实现：

```c
int
page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm)
{
    pte_t *ptep = get_pte(pgdir, la, 1);
    if (ptep == NULL)
    {
        return -E_NO_MEM;
    }

    page_ref_inc(page);

    if (*ptep & PTE_V)
    {
        struct Page *old_page = pte2page(*ptep);

        if (old_page == page)
        {
            page_ref_dec(page);
        }
        else
        {
            page_remove_pte(pgdir, la, ptep);
        }
    }

    *ptep = pte_create(page2ppn(page), perm);
    tlb_invalidate(pgdir, la);

    return 0;
}
```

首先：

```c
get_pte(pgdir, la, 1)
```

查找目标 PTE。

这里 `create = 1`，说明如果中间页表不存在，可以分配新的页表页。

如果创建失败：

```c
return -E_NO_MEM;
```

---

## 4.5 page_insert() 的三种情况

### 情况一：原地址没有映射

直接：

```text
page_ref_inc(page)
        ↓
写入新的 PTE
        ↓
刷新 TLB
```

新页面增加一个引用。

### 情况二：原地址已经映射到同一个页面

首先：

```c
page_ref_inc(page);
```

然后发现：

```c
old_page == page
```

此时执行：

```c
page_ref_dec(page);
```

抵消刚刚增加的引用。

因此引用计数保持不变，只更新新的页表权限。

### 情况三：原地址映射到另一个页面

首先增加新页面引用计数，然后：

```c
page_remove_pte(pgdir, la, ptep);
```

删除旧映射。

旧页面的引用计数随之减少，如果引用数为 0，则释放旧页面。

之后再写入新页面映射。

---

## 4.6 为什么需要刷新 TLB

CPU 访问虚拟地址时，并不一定每次都重新查询页表，而可能直接使用 TLB 中缓存的地址转换结果。

如果已经修改了页表，但 TLB 中仍保存旧映射，则 CPU 可能继续使用旧的物理地址。

因此建立、替换或者删除页表映射后需要执行：

```c
tlb_invalidate(pgdir, la);
```

使旧地址转换失效。

---

## 4.7 Function 1 调用关系

插入：

```text
page_insert()
    ↓
get_pte()
    ↓
page_ref_inc()
    ↓
检查旧映射
    ↓
page_remove_pte()（必要时）
    ↓
写入新 PTE
    ↓
tlb_invalidate()
```

删除：

```text
page_remove()
    ↓
get_pte()
    ↓
page_remove_pte()
    ↓
page_ref_dec()
    ↓
free_page()（引用计数为 0）
    ↓
清除 PTE
    ↓
tlb_invalidate()
```

---

# 五、Challenge 2：分页模式与 get_pte()

**负责人：2412771-谭锦睿**

## 5.1 Sv32、Sv39、Sv48

RISC-V 支持多种分页模式。

### Sv32

主要用于 RV32：

```text
VPN[1] | VPN[0] | page offset
```

使用二级页表。

### Sv39

用于 RV64：

```text
VPN[2] | VPN[1] | VPN[0] | page offset
```

使用三级页表。

每级 VPN 为 9 位，页内偏移为 12 位：

```text
9 + 9 + 9 + 12 = 39 bit
```

当前实验使用的是 Sv39。

代码中可以看到三级索引：

```text
PDX1
PDX0
PTX
```

同时启动页表使用：

```text
boot_page_table_sv39
```

因此可以确定当前工程采用 Sv39。

### Sv48

Sv48 在 Sv39 基础上增加一级页表：

```text
VPN[3] | VPN[2] | VPN[1] | VPN[0] | page offset
```

使用四级页表。

---

## 5.2 为什么 get_pte() 中存在两段相似代码

`get_pte()` 的主要任务是逐级遍历多级页表。

每一级中间页表的处理过程基本相同：

```text
根据 VPN 获得当前页表项
        ↓
检查页表项是否有效
        ↓
如果无效且 create = 1
        ↓
分配新的页表页
        ↓
初始化新的页表页
        ↓
将页表页地址写入当前页表项
        ↓
进入下一级页表
```

因此多级页表中不同层级的代码形式会非常相似。

这种相似并不是偶然的，而是因为不同层级的页表使用了类似的数据结构和遍历方法。

---

## 5.3 get_pte() 是否应该拆分

当前接口：

```c
get_pte(pgdir, la, create)
```

同时支持查询和创建。

例如：

```c
get_pte(pgdir, la, 0);
```

表示只查找。

而：

```c
get_pte(pgdir, la, 1);
```

表示页表不存在时允许创建。

这种设计的优点是：

```text
接口统一
调用简单
复用了页表遍历逻辑
```

但问题是查询函数同时可能执行：

```text
内存分配
页表修改
```

因此存在一定副作用。

另一种设计可以拆成：

```text
find_pte()
create_pte()
```

这样职责更加清晰，但会增加接口数量，而且可能产生重复的页表遍历逻辑。

因此两种设计并没有绝对优劣。

当前 uCore 使用 `create` 参数，是在接口简洁性和职责单一性之间做出的设计权衡。

---

## 5.4 Function 1 编译与验证

执行：

```bash
cd ~/os-lab/nku-os/code
make
```

结果：

```text
pmm.c 编译通过。

page_insert 不再出现 undefined reference。
page_remove 不再出现 undefined reference。
```

此时最终链接仍然存在：

```text
proc_init
cpu_idle
```

未定义。

这两个函数属于其他成员负责的 Function，因此本部分没有继续修改。

同时执行：

```bash
git diff --check
```

结果通过。

工程中已经存在：

```text
check_pgdir()
check_boot_pgdir()
test_lab4_pmm_basic()
```

等页面映射测试代码。

但由于完整内核尚未链接成功，目前无法启动 QEMU 实际运行这些测试。

因此当前 Function 1 的验证结果为：

```text
代码编译通过
符号链接问题已经解决
完整运行测试等待三人代码合并后完成
```

---

# 六、Function 2：进程 / 内核线程创建

**负责人：2413581-黄奕浩**

> 本节由负责人根据本人实际代码实现补充。

## 6.1 任务要求

TODO：根据实验指导书和本人实际负责范围填写。

建议说明：

```text
Function 2 需要实现什么功能
为什么需要这些功能
主要涉及哪些函数
```

---

## 6.2 主要涉及文件

TODO：根据实际修改文件填写。

例如：

```text
code/kern/process/proc.c
code/kern/process/proc.h
```

最终以本人实际修改为准。

---

## 6.3 实现过程

TODO：根据实际代码填写。

建议按照实际调用顺序分析：

```text
入口函数
    ↓
关键步骤 1
    ↓
关键步骤 2
    ↓
关键步骤 3
    ↓
最终结果
```

不要只复制代码，需要说明每一步为什么这样实现。

---

## 6.4 关键代码

TODO：粘贴本人真正修改或补充的核心代码。

```c
// TODO
```

---

## 6.5 关键数据结构分析

TODO：根据实际 Function 2 使用的数据结构填写。

重点分析实际使用到的字段，不需要把整个结构体全部抄一遍。

---

## 6.6 Function 2 调用关系

TODO：结合实际代码整理。

```text
TODO
```

---

## 6.7 实现中遇到的问题

TODO：根据本人真实开发过程填写。

如果没有明显问题，可以删除本节。

---

## 6.8 测试与验证

执行命令：

```bash
make
make qemu
```

实际结果：

```text
TODO
```

需要记录：

```text
是否编译通过
是否链接通过
是否能够进入 QEMU
对应 Function 是否得到正确运行结果
```

不要填写没有实际执行的测试结果。

---

# 七、Function 3：进程调度与上下文切换

**负责人：2411302-王乐之**

> 本节由负责人根据本人实际代码实现补充。

## 7.1 任务要求

TODO：根据实验指导书和本人实际负责范围填写。

---

## 7.2 主要涉及文件

TODO：根据实际修改文件填写。

例如可能涉及：

```text
code/kern/process/
code/kern/schedule/
```

具体以本人实际实现为准。

---

## 7.3 实现过程

TODO：根据实际代码填写。

建议重点说明：

```text
调度入口
如何选择下一个执行实体
什么时候发生上下文切换
切换前后哪些状态发生变化
```

---

## 7.4 关键代码

TODO：粘贴本人实际修改代码。

```c
// TODO
```

---

## 7.5 调用关系

TODO：根据实际调用关系填写。

```text
TODO
```

---

## 7.6 Challenge 1：中断状态保存与恢复

需要说明：

```c
local_intr_save(intr_flag);

/* 临界区 */

local_intr_restore(intr_flag);
```

如何实现中断状态的保存、关闭和恢复。

TODO：由负责人结合实际宏和底层函数完成分析。

建议结合：

```text
sstatus
SSTATUS_SIE
__intr_save()
__intr_restore()
intr_disable()
intr_enable()
```

分析。

---

## 7.7 实现中遇到的问题

TODO：根据本人实际开发过程填写。

---

## 7.8 测试与验证

```bash
make
make qemu
```

实际结果：

```text
TODO
```

不要填写未实际执行的结果。

---

# 八、三个 Function 的关系

三个 Function 最终共同组成 Lab4 的进程管理框架。

目前可以简单表示为：

```text
Function 1
页面映射管理
    ↓
为后续进程管理提供内存基础

Function 2
进程 / 内核线程创建
    ↓
TODO：负责人完成后补充

Function 3
调度与上下文切换
    ↓
TODO：负责人完成后补充
```

完整关系等待三个 Function 全部实现并运行验证后再补充。

---

# 九、实验测试

## 9.1 编译测试

最终合并后执行：

```bash
cd ~/os-lab/nku-os/code
make clean
make
```

结果：

```text
TODO：三人代码合并后填写
```

---

## 9.2 QEMU 测试

执行：

```bash
make qemu
```

结果：

```text
TODO：三人代码合并后填写
```

---

## 9.3 官方测试

按照实验指导书执行对应测试。

```text
TODO
```

结果：

```text
TODO
```

---

## 9.4 Git 修改检查

```bash
git status
git diff --check
git diff --stat
git diff
```

用于检查：

```text
修改文件范围
格式问题
意外修改
三人代码整合结果
```

---

# 十、实验结果

## 10.1 Function 1

已在 `code/kern/mm/pmm.c` 完成：

```text
page_remove_pte()
page_remove()
page_insert()
```

实现能够处理：

```text
无原映射
同一页面重映射
不同页面替换
```

并正确维护：

```text
物理页引用计数
旧映射删除
物理页释放
PTE 更新
TLB 刷新
```

Function 1 当前已经完成源码编译验证。

`page_insert()` 和 `page_remove()` 已不再产生链接错误。

完整 QEMU 测试等待 Function 2 / Function 3 完成后统一进行。

---

## 10.2 Function 2

**负责人：2413581-黄奕浩**

TODO：负责人完成后填写本人实际结果。

---

## 10.3 Function 3

**负责人：2411302-王乐之**

TODO：负责人完成后填写本人实际结果。

---

## 10.4 整体运行结果

TODO：三人代码整合并完成 QEMU 测试后统一填写。

---

# 十一、实验中遇到的问题及解决方法

## 问题一：Lab4 依赖 Lab2 / Lab3 代码

现象：

```text
Lab4 初始代码中存在多个 lab2 / lab3 占位位置，
部分前序实验函数没有实现，导致当前实验无法正常编译。
```

解决：

首先扫描所有：

```text
lab2
lab3
```

注释位置。

然后从对应 Lab2 / Lab3 分支读取已经完成的实现，并迁移到 Lab4。

最终共恢复：

```text
page_init()
物理页分配器
interrupt_handler()
exception_handler()
RESTORE_ALL
__alltraps
```

恢复后这些前序依赖代码均可以正常编译。

---

## 问题二：Function 1 完成后仍无法完整链接

现象：

完成：

```text
page_insert()
page_remove()
```

之后执行 `make`，Function 1 已经不再出现 undefined reference，但仍然存在：

```text
proc_init
cpu_idle
```

未定义。

原因：

这两个函数属于其他成员负责的 Lab4 Function，当前尚未实现。

因此没有为了强行通过链接而修改其他成员代码。

解决：

等待 Function 2 / Function 3 完成后再进行完整整合和测试。

---

## 其他问题

TODO：由各成员根据实际开发过程继续补充。

---

# 十二、重要知识点总结

## 12.1 页面映射不仅是修改一个 PTE

`page_insert()` 和 `page_remove()` 除了修改页表项，还需要同时处理：

```text
页面引用计数
旧映射
页面释放
TLB
```

因此页面映射实际上是一套完整的资源管理过程。

---

## 12.2 get_pte() 的 create 参数

```c
get_pte(pgdir, la, 0);
```

表示：

```text
只查询，不创建页表
```

适合删除映射。

而：

```c
get_pte(pgdir, la, 1);
```

表示：

```text
必要时分配页表
```

适合建立新的页面映射。

---

## 12.3 页面引用计数

一个物理页可能被多个虚拟地址引用。

因此不能在删除一个映射时直接释放物理页。

必须：

```text
page_ref_dec()
    ↓
判断引用计数
    ↓
ref == 0
    ↓
free_page()
```

这样才能避免仍被其他映射使用的页面被提前释放。

---

## 12.4 TLB 与页表一致性

修改页表后，如果 TLB 中还缓存旧的地址转换结果，则 CPU 仍可能继续使用旧映射。

因此修改映射后需要：

```c
tlb_invalidate(pgdir, la);
```

保持 TLB 与页表状态一致。

---

## 12.5 Function 2 / Function 3

TODO：对应成员根据本人实现补充。

---

# 十三、Lab2、Lab3 与 Lab4 的联系

Lab4 并不是完全独立的实验。

Function 1 可以直接看到 Lab2 中：

```text
Page
PTE
页表
物理页分配
```

等机制继续被使用。

同时 Lab4 后续进程管理还会继续使用 Lab3 中已经建立的：

```text
TrapFrame
Trap 入口
中断控制
```

等机制。

因此 Lab4 是前面实验功能进一步组合的阶段。

其他具体联系等待 Function 2 / Function 3 完成后由对应成员继续补充。

---

# 十四、实验总结

当前已经完成 Lab4 前置 Lab2 / Lab3 依赖的恢复，并完成本人负责的 Function 1 页面映射功能。

Function 1 中主要实现：

```text
page_insert()
page_remove()
```

完成了页面映射建立、替换、删除、引用计数维护以及 TLB 刷新。

同时通过分析 Sv32、Sv39 和 Sv48，进一步理解了 RISC-V 多级页表结构，以及 `get_pte()` 中多级页表遍历代码相似的原因。

Function 2 和 Function 3 将由对应成员根据实际代码实现继续补充。

待三个部分全部完成后，再统一进行完整编译、QEMU 运行、官方测试以及最终实验总结。