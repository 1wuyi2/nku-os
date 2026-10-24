# 本次开发记录

日期：2026-10-24（Asia/Shanghai）。本人：2411302-王乐之。

## 初始状态

初始路径 `/home/wlz/nku-os-lab2/nku-os-lab2` 包含 `code/`、`report/`，不含 `.git`。父目录同样没有 Git 元数据，也未发现适用的 AGENTS.md，因此初始阶段无法核实当前分支、旧提交及原工作区状态，报告原有 commit 信息按原文保留。修改前将原目录复制到 `/tmp/ucore-slub-baseline/`；原有受保护源码哈希和初次交付差异见 `evidence/integrity.txt`，初次源代码补丁见 `evidence/slub-changes.patch`。用户随后提供真实仓库及截图并授权上传，正式迁移记录见下文。

确认 `pmm_init()` 完成管理器初始化、DTB 页登记和管理器自检；`kern_init()` 此后调用 SLUB 初始化及条件自测。Best-Fit 的 `property`、`page_link` 以及 Buddy 的 ALLOCATED_HEAD/原始请求字段均有实际用途，因此选择页内独立 Slab 头。

## 任务与调整

实际输入为一份总任务，原文保存在 `evidence/original-task.txt`。两份四段式文档是从总任务整理、与最终代码一致的子规格，未虚构两个模型调用。此次没有用户另行发来的修复 Prompt；下列内容为助手在连续开发中的实际调整决策及命令，供追溯。

| 阶段 | 观察及实际处理 |
|---|---|
| 工程检查 | `git status` 返回非 Git 仓库；保留解压副本及原成员成果，最终提供可迁移补丁，提交保持待办 |
| 指导书 | 网页工具访问失败，随后 WSL `curl` 成功读取首页、练习及 GDB 分支页面；已据此补做 QEMU 源码调试 |
| QEMU 环境 | 系统为 8.2.2；下载官方 4.1.1 源码至 `/tmp` 并本地构建，未修改系统版本 |
| 构建依赖 | 初次尝试 `--disable-pixman` 被 QEMU 4.1.1 拒绝；改为 `apt-get download libpixman-1-dev`，在 `/tmp/qemu-deps` 解包并设置 include/library/pkg-config 路径，无须 sudo |
| SLUB | 首次主体及自测编译通过；十一组内核自测首轮即通过，没有虚构分配器失败或修复 |
| 回归 | `tools/validate_slub.py` 依次清理构建各配置，默认官方 20/20，Buddy 八组及 SLUB 十一组均通过，最后恢复默认构建 |
| 内核 GDB | `tools/sv39.gdb` 在 `slub_init` 断点读取真实 satp，通过 monitor 物理读取完成支持超级页的页表遍历，并比较两种地址读出的字节 |
| 模拟器 GDB | 第一轮打印循环变量 `i` 时出现 `value has been optimized out`；保留 `qemu-mmu-gdb-first-attempt.log`，移除该冗余打印，使用仍可观察的 PPN、ptshift 和实际输出指针；重跑通过 |
| TLB | 在 QEMU 的条件断点观察 M 模式直通、Sv39 miss、真实页表读取、4 KiB 软件 TLB 填充，以及后续命中跳过 walker |

内核测试没有 panic/断言失败。QEMU 的 `terminating on signal 15` 来自验证脚本在成功标记后终止持续运行的内核，不代表内存异常。

## 环境复现

编译过程完整日志为 `evidence/qemu-configure.log`、`qemu-build.log`，本轮构建参数包含 `-O2 -g`；实际二进制保留调试信息。源码下载自 [QEMU 官方 4.1.1 归档](https://download.qemu.org/qemu-4.1.1.tar.xz)。主机 GCC 用于构建 QEMU；交叉 GCC 13.2.0 用于构建内核。实际内核及模拟器调试均使用 GDB 15.1。

本地已安装到用户目录：

```bash
QEMU_LAB2="$HOME/.local/opt/qemu-4.1.1/bin/qemu-system-riscv64 -L $HOME/.local/opt/qemu-4.1.1/share/qemu"
cd /home/wlz/nku-os-lab2/nku-os-lab2/code
python3 tools/validate_slub.py --qemu "$QEMU_LAB2"
python3 tools/debug_sv39.py --qemu "$QEMU_LAB2"
python3 tools/debug_qemu_mmu.py \
  --qemu "$HOME/.local/opt/qemu-4.1.1/bin/qemu-system-riscv64" \
  --bios-dir "$HOME/.local/opt/qemu-4.1.1/share/qemu"
```

`make qemu` 的普通默认命令会使用系统 PATH 中的 QEMU；此主机仍是 8.2.2，复现时必须传上述 `QEMU` 覆盖值。未改写 Makefile。host GDB 的源码断点需要保留 `/tmp/qemu-4.1.1` 源码树；迁移或重启后可重新解压归档到该目录，或在 GDB 使用 `set substitute-path /tmp/qemu-4.1.1 <新的源码目录>`。脚本从当前 ELF 动态获取 `kern_init`，修改代码或编译宏会使真实地址变化，应重跑后读取新记录。

## Git 迁移

初次交付时没有 `.git`，尚无 commit 哈希。`evidence/git-check.log` 保留当时 Git 状态、历史及差异命令无法执行的原始原因；`slub-changes.patch` 是加入本次用户截图之前的初次交付补丁，其检查结果保留在 `patch-check.log`，用于追溯初始成果。

用户随后提供 `https://github.com/1wuyi2/nku-os.git`，授权将成果上传到 `lab2`。已克隆到 `/home/wlz/nku-os-lab2/nku-os-git`，远程起点为 `dc81539ea2af1312732fe07d442c38dd68154212`。排除 Windows/WSL 换行差异后，原源码、报告和成员 Prompt 与开发前快照一致，未发生内容冲突。代码及工具以 `2cba00f` 提交，提交信息为 `完善报告`；报告、Prompt、证据与截图单独提交。只迁入明确归属本任务的文件，没有提交原有 `code/obj` 构建产物，也没有重写原成员历史。

仓库克隆使用 Windows Git，开发检查和提交使用 WSL Git；克隆初期通过仓库级换行设置识别 CRLF，随后将 WSL 工作副本中的文本规范为 LF，并在该仓库设置 `core.autocrlf=input`、`core.filemode=false`，保证 shell 脚本在 WSL 可执行。没有修改全局 Git 配置或原有文件的版本内容。初次 WSL Git 直连远程未及时返回，改用可正常连接的 Windows Git 完成远程操作。

原始工具日志含有工具自身输出的行尾空白，历史统一补丁的空白上下文行也必须保留。新增 `.gitattributes` 仅对证据目录中的原始日志、历史补丁和用户原始任务文本保留换行并关闭空白样式检查，以原样保存证据；源码、报告和其他文本继续执行正常 `git diff --check`。

## 截图与核对事项

初次交付时执行通道只提供终端输出，保留原始日志。用户随后在本地终端执行命令并提供两张真实截图，已原样复制为 `images/slub-test-results.png`、`images/sv39-gdb-results.png`，分别插入主报告 5.5 和 4.4 节。第一张覆盖 SLUB 十一组 PASS、页数恢复及压力测试；第二张覆盖真实 SATP/VPN/PTE、物理地址计算和字节一致结果。两图内容与保存日志一致，未编辑或合成图片。QEMU 源码调试与其他回归验证继续以完整文本日志作为证据。

原报告 2.1 与 Buddy 章节的工具链版本分别写为 10.2.0 与 13.2.0；这是原有记录，未改写其他成员章节。本轮以 `evidence/environment.txt` 中 13.2.0 为准。原报告总完成日期保留，新增部分明确写本轮日期。最终差异检查以真实 `lab2` 仓库为准，历史初次交付的补丁和非仓库错误日志保留用于追溯。

链接核对发现原 Buddy 章节引用 `prompt1.md`，当前远程分支实际仅包含整合后的 `prompt.md`。此引用来自原报告，已保留原文供小组核对；本次新增图片、Prompt 和日志链接均已检查存在。

用户随后要求统一提交说明，本轮代码和报告两个提交均已改为“完善报告”。原始运行日志中的旧提交编号保留用于追溯，改写前后实现代码树一致。

## 提交注释复核

用户补充的课程提交要求明确要求保留 lab2 标记，并将 YOUR CODE 替换为实际学号。核对发现 `pmm.c:69`、`best_fit_pmm.c:6` 两处仍为占位文字，现已按报告分工改为 `2412771`（谭锦睿），保留功能编号。两个文件修改前后的预处理结果完全一致，算法及官方测试代码未改。代码目录已全量检索，无剩余 YOUR CODE 占位标记。SLUB 属于 Challenge，课程截图对其不要求原框架标记。
