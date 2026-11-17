# 00-02 环境准备与本书读法

## 一、本书的写法约定

### 每章的固定结构

1. **问题**：没有这个特性时，代码要怎么写、痛在哪里
2. **机制**：特性本身怎么工作，必要时给出编译器视角的解释
3. **陷阱**：实测能复现的坑，附真实输出
4. **取舍**：什么时候该用、什么时候不该用
5. **小结**：表格速查

readme 里"新特性解决了什么问题"这一要求，落在第 1 步。
一个特性如果脱离了它要解决的问题，就只剩语法，记不住也用不对。

### 标准标注

正文和表格里会用 `[C++11]` `[C++14]` `[C++17]` 标注特性引入的标准。
没有标注的，是 C++98 就有的内容。

需要留意一类情况：有些特性在标准 A 引入，在标准 B 被显著增强。
例如 lambda 是 `[C++11]`，但泛型 lambda 是 `[C++14]`，
`constexpr` lambda 和 `*this` 捕获是 `[C++17]`。这类情况会分别标注。

### 新旧对比

凡是现代写法替代了旧写法的地方，并排给出：

```cpp
// C++98
std::vector<int>::const_iterator it = v.begin();
for (; it != v.end(); ++it) { use(*it); }

// C++11
for (const auto& x : v) { use(x); }
```

目的不是贬低旧写法 —— 维护老代码时你仍然要读懂它 ——
而是让"为什么要变"这件事变得具体。

## 二、环境要求

本书所有代码在以下环境实测通过：

```
g++ (Ubuntu 11.4.0-1ubuntu1~22.04.2) 11.4.0
支持 -std=c++11 / c++14 / c++17 / c++20
```

最低要求：

| 标准 | GCC | Clang | MSVC |
|---|---|---|---|
| C++11 | 4.8.1+ | 3.3+ | VS2015+ |
| C++14 | 5+ | 3.4+ | VS2017+ |
| C++17 | 7+（建议 9+） | 5+ | VS2017 15.7+ |

C++17 的 `<filesystem>` 在 GCC 8 及更早版本需要单独链接
`-lstdc++fs`，GCC 9 起并入标准库，不再需要。本书的 filesystem demo 做了兼容处理。

## 三、编译单个 demo

每个 demo 文件的首行写明了它需要的标准和额外链接选项：

```cpp
// std=c++17 flags=-pthread
```

照此编译即可：

```bash
g++ -std=c++17 -Wall -Wextra -pthread demos/part8_concurrency/08_01_thread.cpp -o /tmp/a && /tmp/a
```

推荐始终开 `-Wall -Wextra`。C++ 有大量"能编过但不是你想要的"写法，
警告是第一道防线。调试内存问题时再加上：

```bash
# AddressSanitizer：检测越界、use-after-free、泄漏
g++ -std=c++17 -g -fsanitize=address,undefined demo.cpp -o a && ./a

# 看模板实例化错误时，截断过深的展开
g++ -std=c++17 -ftemplate-backtrace-limit=0 demo.cpp
```

## 四、批量编译全部 demo

仓库里带了构建脚本，用来确认全书代码都还能编过：

```bash
./scripts/build_all.sh              # 增量编译（跳过未改动的）
./scripts/build_all.sh --force      # 全量重编
./scripts/build_all.sh --jobs 32    # 限制并行度
./scripts/build_all.sh part2_memory # 只编某个目录
```

脚本的几个设计点：

- **进程级并行**：用 `xargs -P` 按 CPU 核数并行，每个 demo 独立编译运行，互不干扰
- **断点续跑**：成功的 demo 按"源码内容 + 编译选项"算哈希，落盘到
  `build/state/`。再次运行时内容没变就跳过，中途被打断也能接着跑
- **日志分离**：编译错误进 `build/log/`，运行输出进 `build/out/`，
  正文里引用的"实测输出"都来自这里
- **特殊标记**：首行带 `nodemorun` 的只验证编译不运行（例如演示死锁的代码）；
  带 `allowfail` 的允许非零退出（例如演示 `assert` 失败、未捕获异常）

目录结构：

```
cpp_1111/
├── blog/      # 正文，按 part 分目录
├── demos/     # 可编译 demo，目录与 blog 一一对应
├── notes/     # 知识树、大纲、进度
├── scripts/   # 构建脚本
└── build/     # 构建产物（bin/out/log/state），可随时删除重建
```

## 五、确认你的编译器都支持什么

直接跑 [`demos/part0_overview/00_01_standard_probe.cpp`](../../demos/part0_overview/00_01_standard_probe.cpp)：

```bash
g++ -std=c++17 demos/part0_overview/00_01_standard_probe.cpp -o /tmp/probe && /tmp/probe
```

本机实测输出：

```
=== 编译器 ===
GCC 11.4.0

=== 标准版本 ===
__cplusplus = 201703  -> C++17

=== 语言特性测试宏 ===
rvalue_references     = 200610
lambdas               = 200907
constexpr             = 201603
generic_lambdas       = 201304  [C++14]
return_type_deduction = 201304  [C++14]
variable_templates    = 201304  [C++14]
structured_bindings   = 201606  [C++17]
if_constexpr          = 201606  [C++17]
fold_expressions      = 201603  [C++17]
deduction_guides      = 201703  [C++17]
inline_variables      = 201606  [C++17]

=== 库特性测试宏 ===
lib_optional     = 201606  [C++17]
lib_string_view  = 201803  [C++17]
lib_filesystem   = 201703  [C++17]
lib_make_unique  = 201304  [C++14]
```

这些 `__cpp_xxx` 宏比 `__cplusplus` 有用得多。
`__cplusplus` 只告诉你标准版本，而特性宏精确回答"这个特性在我的编译器上能不能用"，
这在需要兼容多个编译器版本的项目里是刚需：

```cpp
#ifdef __cpp_if_constexpr
    if constexpr (std::is_integral_v<T>) { /* ... */ }
#else
    // 退回到 SFINAE 或标签分发
#endif
```

宏的值是形如 `201703` 的年月号，表示该特性最后一次修订的时间，
可以用来区分"支持基础版本"和"支持修订版本"。

## 六、阅读路径建议

**如果你是 C++ 初学者**：按顺序读。Part 0→1→2→3→4→5 是一条完整的学习路径，
Part 6 以后可以按需查阅。

**如果你有 C++98/03 基础，想补现代特性**：

1. [02-03 右值引用与移动语义](../part2_memory/02-03_move_semantics.md) —— 优先级最高，
   它改变了 C++ 的对象传递方式，不懂这个就读不懂现代 C++ 代码
2. [02-05 智能指针](../part2_memory/02-05_smart_pointers.md) —— 现代 C++ 不再手写 `delete`
3. [06-01 lambda 表达式全解](../part6_modern_core/06-01_lambda.md) —— 配合 STL 算法
4. [01-10 auto 与 decltype](../part1_basics/01-10_auto_decltype.md)、
   [01-11 统一初始化](../part1_basics/01-11_uniform_init.md) —— 日常写法的变化
5. [03-04 拷贝控制与 Rule of Five](../part3_class/03-04_copy_control.md) —— 类的写法变了
6. [Part 8 并发](../part8_concurrency/) —— 如果你在写多线程代码

**如果你在准备面试**：[09-02 高频面试题解答](../part9_appendix/09-02_interview.md)
汇总了各章的考点并给出答案出处。

**如果你只想查某个特性**：[09-01 特性速查表](../part9_appendix/09-01_cheatsheet.md)
按标准列出全部特性和所在章节。

## 七、关于代码风格

本书的 demo 代码遵循几条约定，和正文里讨论的实践保持一致：

- 成员变量用 `_` 前缀（与参考文档一致），局部变量用小写下划线
- 一律用 `std::` 限定，不在示例里写 `using namespace std;`
  —— 原因见 [01-01 命名空间](../part1_basics/01-01_namespace.md)
- 演示"错误写法"的代码会注释掉并标明 `// 编译错误：原因`，
  保证整个文件能编过。想亲眼看到报错，取消注释再编译
- 每个 demo 可独立编译运行，不依赖其他文件

> 下一章：[01-01 命名空间](../part1_basics/01-01_namespace.md)
