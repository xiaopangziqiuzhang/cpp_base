# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 仓库定位

中文 C++ 知识库，主线是"**新特性解决了什么问题**"：覆盖 C++98 基础语法到 C++11/14/17 特性，64 篇章节（`blog/`）配 62 个可编译运行的 demo（`demos/`）。

目录分工：

- `blog/` — 正文，按 Part 0~9 分子目录；`blog/README.md` 是全书总索引
- `demos/` — 与章节一一对应的可运行代码，由构建脚本全量验证
- `notes/` — 过程产物：知识树、大纲、进度与实测发现
- `scripts/build_all.sh` — 唯一的验证入口
- 根目录 `readme.md` 是最初的任务简述，不是索引，别和 `blog/README.md` 混淆。它引用的两个 `/juicefs-algorithm/...` 参考文档在本机不存在，不必去找

## 常用命令

```bash
# 全量验证（增量：跳过已成功且源码未变的）
./scripts/build_all.sh

# 只验证某个 Part 或单个 demo（参数是对 demo 路径做 grep 的过滤串）
./scripts/build_all.sh part1_basics
./scripts/build_all.sh 02_05_smart_pointers

# 全量重编 / 指定并行度
./scripts/build_all.sh --force
./scripts/build_all.sh --jobs 8

# 手工编译单个 demo（标准和链接参数见该文件首行）
g++ -std=c++17 -Wall -Wextra -O0 -g demos/part2_memory/02_05_smart_pointers.cpp -o /tmp/d && /tmp/d
```

产物位置（`build/` 已 gitignore，可随时重建）：

| 路径 | 内容 |
|---|---|
| `build/bin/<flat>` | 可执行文件 |
| `build/out/<flat>.out` | 运行输出，章节中引用的"实测输出"来自这里 |
| `build/log/<flat>.compile.log` | 编译日志，编译失败先看它 |
| `build/state/<flat>.<md5>.ok` | 断点续跑标记 |
| `build/summary.txt` | 本次汇总（OK / SKIP / FAIL 清单） |

`<flat>` = demo 相对路径去掉 `.cpp` 后把 `/` 换成 `__`，例如 `part2_memory__02_05_smart_pointers`。

## demo 的硬约定

构建脚本解析 demo 的**首行注释**来决定编译参数，写错会用错标准或漏链接库：

```cpp
// std=c++17 flags=-pthread
// 对应章节: blog/part2_memory/02-05_smart_pointers.md
// 主题: unique_ptr / shared_ptr / weak_ptr
```

- `std=` 省略时默认 `c++17`；`flags=` 之后的内容整体透传给 g++。当前分布：50 个 `c++17`、9 个 `c++17 flags=-pthread`、3 个 `c++14`
- 脚本另认两个首行标记：`nodemorun` 只编译不运行，`allowfail` 允许非 0 退出（演示异常/assert 时用）。目前没有 demo 用到，但机制可用
- 第二行 `对应章节:` 是 demo 反向指回章节的链接，62 个 demo 全部具备，新增时必须写
- 续跑签名 = 源码内容 + std + flags 的 md5。改过 demo 会自动失效重编，不需要 `--force`

命名在两处刻意不同，容易写错：demo 用下划线（`demos/part1_basics/01_10_auto_basic.cpp`），章节用连字符（`blog/part1_basics/01-10_auto_decltype.md`）。且 demo 与章节不是严格 1:1，一章可以配多个 demo。

demo 内部统一结构：按 `// ---------- N. 主题 ----------` 分节，每节一个 `void` 函数，`main()` 里逐个调用并打印分隔标题，便于把输出对回章节。所有 demo 必须在 `-Wall -Wextra` 下零警告通过。

## 章节的写作约定

`blog/part0_overview/00-02_environment.md` 定下的固定结构，新增章节照此执行：

1. **问题** — 没有这个特性时代码怎么写、痛在哪
2. **机制** — 特性如何工作，必要时给编译器视角的解释
3. **陷阱** — 可实测复现的坑，附真实输出
4. **取舍** — 何时该用、何时不该用
5. **小结** — 表格速查

约 250 行/章。特性标注 `[C++11]` `[C++14]` `[C++17]`，无标注即 C++98 内容；同一特性跨标准增强时分别标注（如 lambda 是 `[C++11]`、泛型 lambda 是 `[C++14]`）。凡现代写法替代旧写法处，并排给出新旧对比。

## 新增或修改章节时要同步的四处

索引最容易漏，这些文件需要跟着更新：

- `blog/README.md` — 总索引表格（章节链接 + 关键特性列）
- `notes/03_progress.md` — 完成状态勾选
- `blog/part9_appendix/09-04_demo_index.md` — demo 到章节的映射
- `blog/part9_appendix/09-01_cheatsheet.md` — 涉及新特性时补进速查表

`notes/03_progress.md` 末尾记录了已验证的实测发现和写作中踩过的坑（例如 GCC 的 `NULL` 是 `__null` 导致重载二义性、libstdc++ 缺 TBB 时 `std::execution::par` 静默退化为串行、`shared_mutex` 在高并发短临界区下比 `mutex` 慢）。改相关章节前先读它，别和已验证的结论冲突。该文件也记了一条教训：大纲完成后应用脚本对着知识树做关键词覆盖度核查，而不是凭记忆确认写全了。

## 环境

原始验证环境 g++ 11.4.0 (Ubuntu 22.04)，支持 `-std=c++11/14/17/20`。`<filesystem>` 在 GCC 8 及更早需要额外链接 `-lstdc++fs`，GCC 9 起并入标准库，filesystem demo 已做兼容处理。
