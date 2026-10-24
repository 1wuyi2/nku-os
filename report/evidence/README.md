# 本轮证据索引

日期：2026-10-24。所有运行日志来自真实 WSL/QEMU/GDB；没有生成模拟终端截图。主报告补充了 4.2.4、4.4、5.5、6.1、7.3、7.4 及本人任务检查项，其他成员正文保留。

| 文件 | 内容 |
|---|---|
| `environment.txt` | 初始工具版本、缺少 Git 元数据及 pixman 开发包的真实诊断 |
| `manual-fetch.log`、`manual-sources.txt` | 指导书访问记录、核对的页面和取阅方式 |
| `qemu-version.txt`、`qemu-configure.log`、`qemu-build.log` | 本地实际构建的 QEMU 4.1.1 版本、参数和完整构建输出 |
| `slub-build.log`、`slub-qemu.log` | 默认 Best-Fit 上层十一组 SLUB 测试，31924 → 31924 页 |
| `default-build.log`、`default-qemu.log` | 默认构建及真实启动，无 SLUB 自测 |
| `grade.log`、`grade-serial.log` | 原官方脚本 20/20 及注入测试的真实串口 PASS |
| `buddy-slub-build.log`、`buddy-slub-qemu.log` | Buddy 原八组 + SLUB 十一组，31923 → 31923 页 |
| `default-final-build.log`、`default-final-qemu.log` | 最终重新清理并恢复默认配置后的编译、启动 |
| `validation-summary.log` | 完整自动验证流程真实汇总 |
| `sv39-gdb.log`、`sv39-qemu.log` | guest GDB、monitor 物理读取、超级页遍历及字节比较 |
| `qemu-mmu-gdb.log` | host GDB：M 模式直通、Sv39 页表读取、叶子计算、TLB 填充和命中 |
| `qemu-mmu-gdb-first-attempt.log` | 首次调试打印优化变量失败的真实记录，保留以便追溯 |
| `original-task.txt` | 用户实际输入总任务；两份子 Prompt 据此整理 |
| `git-check.log` | 在当前副本执行 Git 状态/历史/差异命令的真实错误原因 |
| `integrity.txt` | 初次交付时受保护源码 SHA-256，以及当时的修改/新增清单 |
| `slub-changes.patch` | 加入用户截图及真实提交记录之前的初次交付补丁，不包含证据日志 |
| `patch-check.log` | 在独立临时基线执行补丁适用性和空白检查的真实结果 |

复现入口为 `code/tools/validate_slub.py`、`debug_sv39.py` 和 `debug_qemu_mmu.py`，参数见 [开发记录](../development.md)。默认 `make qemu` 仍从 PATH 选择系统 QEMU，当前机器必须覆盖为本地 4.1.1 并设置 `-L` 固件目录。

初始解压副本没有 `.git`，当时未创建 commit，相关日志保留用于追溯。用户随后提供真实仓库并授权上传，已克隆 `1wuyi2/nku-os` 的 `lab2`，核对远程基线一致并完成实现提交 `2cba00f`；报告、Prompt、证据与截图单独提交。最终提交情况见主报告 6.1 与开发记录。

用户提供的两张真实终端截图已保存到 `../images/slub-test-results.png` 和 `../images/sv39-gdb-results.png`，分别插入 5.5 和 4.4 节。图片原样复制，所有关键数值与原始日志一致；未生成模拟截图。
