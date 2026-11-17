# C++ 知识库：从基础语法到 C++11/14/17

一套以"**新特性解决了什么问题**"为主线的 C++ 知识库。
每个特性都先讲它要解决的痛点，再讲机制，配可编译运行的 demo 和实测输出。

- 全部代码位于 [`../demos/`](../demos/)，由 [`../scripts/build_all.sh`](../scripts/build_all.sh) 全量编译验证
- 实测环境：g++ 11.4.0 (Ubuntu 22.04)，支持 `-std=c++11/14/17/20`
- 标注约定：`[C++11]` `[C++14]` `[C++17]` 表示特性引入的标准，无标注为 C++98 内容

---

## Part 0 导读

| 章节 | 内容 |
|---|---|
| [00-01 C++ 的三个时代与标准演进](part0_overview/00-01_history.md) | 语言起源的两条约束、三个时代、标准一览、C++11 为何是分水岭 |
| [00-02 环境准备与本书读法](part0_overview/00-02_environment.md) | 编译器要求、demo 编译方式、特性测试宏、阅读路径建议 |

## Part 1 基础语法：C++ 在 C 之上加了什么

| 章节 | 关键特性 |
|---|---|
| [01-01 命名空间](part1_basics/01-01_namespace.md) | `namespace`、匿名空间、嵌套命名空间简化 `[C++17]`、`inline namespace` `[C++11]` |
| [01-02 const 与 constexpr](part1_basics/01-02_const_constexpr.md) | const 四种修饰、const vs 宏、`constexpr` `[C++11/14]`、内联变量 `[C++17]` |
| [01-03 引用](part1_basics/01-03_reference.md) | 引用的本质、做参数、做返回值、引用 vs 指针 |
| [01-04 类型转换](part1_basics/01-04_cast.md) | 四种 cast、为什么不用 C 风格、`dynamic_cast` 与 RTTI |
| [01-05 函数重载与默认参数](part1_basics/01-05_overload.md) | name mangling、`extern "C"`、默认参数规则与二义性 |
| [01-06 bool / nullptr / enum class](part1_basics/01-06_bool_nullptr_enum.md) | `NULL` 的问题与 `nullptr` `[C++11]`、`enum class` `[C++11]` |
| [01-07 inline 函数](part1_basics/01-07_inline.md) | inline vs 宏、必须放头文件的原因、现代语义 |
| [01-08 异常与异常安全](part1_basics/01-08_exception.md) | `try/catch/throw`、标准异常体系、`noexcept` `[C++11]`、三级异常安全保证 |
| [01-09 字面量与语法糖](part1_basics/01-09_literals.md) | 原始字符串 `[C++11]`、自定义字面量 `[C++11]`、数字分隔符 `[C++14]`、属性 `[[...]]` |
| [01-10 类型推导：auto 与 decltype](part1_basics/01-10_auto_decltype.md) | `auto` 推导规则与三个陷阱、`decltype`、尾置返回、`decltype(auto)` `[C++14]` |
| [01-11 统一初始化](part1_basics/01-11_uniform_init.md) | `{}` 初始化、`initializer_list`、最烦人的解析、窄化转换 |
| [01-12 范围 for 与结构化绑定](part1_basics/01-12_range_for.md) | 范围 for `[C++11]`、结构化绑定 `[C++17]`、if/switch 初始化语句 `[C++17]` |

## Part 2 内存模型与资源管理

| 章节 | 关键特性 |
|---|---|
| [02-01 程序内存布局](part2_memory/02-01_memory_layout.md) | 五大区、实测各区地址、栈 vs 堆 |
| [02-02 new/delete 表达式](part2_memory/02-02_new_delete.md) | new 三步、delete 两步、`operator new/delete` 重载、placement new |
| [02-03 右值引用与移动语义](part2_memory/02-03_move_semantics.md) | 值类别、移动构造/赋值、`std::move`、Rule of Five、`noexcept` 的影响 |
| [02-04 完美转发](part2_memory/02-04_perfect_forwarding.md) | 万能引用、引用折叠、`std::forward`、`emplace_back` 原理 |
| [02-05 智能指针](part2_memory/02-05_smart_pointers.md) | `unique_ptr`、`shared_ptr`、`weak_ptr`、`make_unique` `[C++14]`、循环引用 |
| [02-06 RAII 与资源管理惯用法](part2_memory/02-06_raii.md) | RAII 原理、scope guard、为什么不再手写 delete |

## Part 3 类与对象

| 章节 | 关键特性 |
|---|---|
| [03-01 类的定义与 this](part3_class/03-01_class_basics.md) | class vs struct、成员函数的本质、`this`、空指针调用成员函数 |
| [03-02 构造函数](part3_class/03-02_constructor.md) | 初始化列表与初始化顺序、委托构造 `[C++11]`、继承构造 `[C++11]`、类内初始值 `[C++11]`、`explicit` |
| [03-03 析构函数与对象生命周期](part3_class/03-03_destructor.md) | 析构时机与顺序、各存储期对象的生灭 |
| [03-04 拷贝控制与 Rule of Five](part3_class/03-04_copy_control.md) | 深浅拷贝与 double free、`=default` / `=delete` `[C++11]`、五大函数生成规则 |
| [03-05 特殊成员](part3_class/03-05_special_members.md) | const/引用/类对象/static 成员、static 成员内联初始化 `[C++17]` |
| [03-06 const 成员函数与 mutable](part3_class/03-06_const_member.md) | const 成员函数的本质、重载决议、`mutable` |
| [03-07 单例模式的四次进化](part3_class/03-07_singleton.md) | 裸指针 → 加锁 → Meyers Singleton `[C++11]` → `call_once` |
| [03-08 Pimpl 惯用法](part3_class/03-08_pimpl.md) | 编译期解耦、与 `unique_ptr` 配合、不完整类型问题 |
| [03-09 友元与运算符重载](part3_class/03-09_operator_overload.md) | 友元函数/类、三种重载形式、`++`/`()`/`[]`/`->`/`<<` 的重载 |
| [03-10 类型转换](part3_class/03-10_type_conversion.md) | 转换构造函数、类型转换运算符、`explicit operator bool`、二义性 |

## Part 4 面向对象：继承与多态

| 章节 | 关键特性 |
|---|---|
| [04-01 继承基础](part4_oop/04-01_inheritance.md) | 三种派生方式的权限、继承的局限、`using` 改变访问权限 |
| [04-02 派生类的构造与销毁](part4_oop/04-02_derived_lifecycle.md) | 构造/析构顺序、多基继承、基类派生类转换 |
| [04-03 名字冲突与菱形继承](part4_oop/04-03_diamond.md) | 二义性、虚继承、虚基类对象布局 |
| [04-04 虚函数与多态](part4_oop/04-04_virtual.md) | 动态绑定、虚表虚指针、实测虚表、构造析构中的虚调用 |
| [04-05 override/final 与三种"同名"](part4_oop/04-05_override.md) | 重载 vs 隐藏 vs 覆盖、`override` `[C++11]`、`final` `[C++11]` |
| [04-06 纯虚函数、抽象类、接口](part4_oop/04-06_abstract.md) | 纯虚函数、抽象类、虚析构函数的必要性 |
| [04-07 多态的代价与 CRTP](part4_oop/04-07_crtp.md) | 虚调用开销实测、内联障碍、CRTP 静态多态 |

## Part 5 模板与泛型编程

| 章节 | 关键特性 |
|---|---|
| [05-01 函数模板](part5_template/05-01_function_template.md) | 实例化、推导、与重载的关系、非类型参数、`auto` 非类型参数 `[C++17]` |
| [05-02 类模板](part5_template/05-02_class_template.md) | 成员函数定义、默认模板参数、CTAD `[C++17]`、变量模板 `[C++14]` |
| [05-03 特化与偏特化](part5_template/05-03_specialization.md) | 全特化、偏特化、常见坑 |
| [05-04 可变模板参数](part5_template/05-04_variadic.md) | 参数包、`sizeof...`、递归展开、逗号表达式展开、折叠表达式 `[C++17]` |
| [05-05 编译期计算与 type_traits](part5_template/05-05_type_traits.md) | `static_assert` `[C++11]`、type_traits、SFINAE、`enable_if`、`if constexpr` `[C++17]` |
| [05-06 模板的工程问题](part5_template/05-06_template_engineering.md) | 两阶段查找、`typename`/`template` 消歧、显式实例化、编译期开销 |

## Part 6 C++11/14/17 核心语言特性专题

| 章节 | 关键特性 |
|---|---|
| [06-01 lambda 表达式全解](part6_modern_core/06-01_lambda.md) | 闭包本质、捕获方式、泛型 lambda `[C++14]`、初始化捕获 `[C++14]`、`constexpr` lambda `[C++17]` |
| [06-02 可调用对象统一视图](part6_modern_core/06-02_callable.md) | 函数对象、`std::function`、`std::bind`、`std::ref`、`std::invoke` `[C++17]` |
| [06-03 类型别名与 using](part6_modern_core/06-03_using.md) | `typedef` vs `using` `[C++11]`、别名模板、`_t`/`_v` 惯例 `[C++14]` |
| [06-04 C++14 特性汇总](part6_modern_core/06-04_cpp14.md) | 泛型 lambda、auto 返回推导、`make_unique`、变量模板、`[[deprecated]]` |
| [06-05 C++17 语言特性汇总](part6_modern_core/06-05_cpp17.md) | 结构化绑定、`if constexpr`、折叠表达式、CTAD、内联变量、保证的拷贝消除 |

## Part 7 标准库

| 章节 | 关键特性 |
|---|---|
| [07-01 string 与 string_view](part7_stdlib/07-01_string.md) | string 接口、底层实现（Eager/COW/SSO/fbstring）、`string_view` `[C++17]` 及悬垂陷阱 |
| [07-02 容器全景](part7_stdlib/07-02_containers.md) | `array` `[C++11]`、`forward_list` `[C++11]`、`unordered_*` `[C++11]`、emplace 系列、选择决策表 |
| [07-03 迭代器与算法](part7_stdlib/07-03_algorithms.md) | 迭代器分类、常用算法、erase-remove 惯用法、并行算法 `[C++17]` |
| [07-04 tuple 与和类型](part7_stdlib/07-04_tuple_variant.md) | `pair`/`tuple`、`tie`、`apply` `[C++17]`、`optional`/`variant`/`any` `[C++17]` |
| [07-05 chrono 与 random](part7_stdlib/07-05_chrono_random.md) | 时间点/时长/时钟、计时惯用法、随机数引擎与分布 |
| [07-06 正则与文件系统](part7_stdlib/07-06_regex_filesystem.md) | `regex` `[C++11]`、`filesystem` `[C++17]` |
| [07-07 IO 流体系](part7_stdlib/07-07_iostream.md) | 流类型关系、流状态、缓冲区、文件 IO、`stringstream` 格式转换 |

## Part 8 并发

| 章节 | 关键特性 |
|---|---|
| [08-01 线程](part8_concurrency/08-01_thread.md) | `thread`、join/detach、传参陷阱、`thread_local` |
| [08-02 互斥与锁](part8_concurrency/08-02_mutex.md) | mutex 家族、`lock_guard`/`unique_lock`/`scoped_lock` `[C++17]`、死锁 |
| [08-03 条件变量与同步](part8_concurrency/08-03_condition_variable.md) | `condition_variable`、虚假唤醒、生产者消费者队列 |
| [08-04 原子操作与内存模型](part8_concurrency/08-04_atomic.md) | `atomic`、CAS、六种内存序、无锁栈 |
| [08-05 异步任务](part8_concurrency/08-05_async.md) | `future`/`promise`/`packaged_task`/`async`、线程池实现 |

## Part 9 附录

| 章节 | 内容 |
|---|---|
| [09-01 特性速查表](part9_appendix/09-01_cheatsheet.md) | C++11/14/17 全特性一览 + 所在章节 |
| [09-02 高频面试题解答](part9_appendix/09-02_interview.md) | 各章考点汇总与答案 |
| [09-03 现代 C++ 编码清单](part9_appendix/09-03_guidelines.md) | 该用什么不该用什么、Core Guidelines 摘要 |
| [09-04 demo 索引](part9_appendix/09-04_demo_index.md) | 全部 demo 文件与对应章节 |

---

## 快速开始

```bash
# 验证环境
g++ -std=c++17 demos/part0_overview/00_01_standard_probe.cpp -o /tmp/probe && /tmp/probe

# 编译全部 demo（增量、全核并行、支持断点续跑）
./scripts/build_all.sh
```
