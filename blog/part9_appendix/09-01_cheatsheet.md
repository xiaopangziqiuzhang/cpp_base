# 09-01 特性速查表

按标准版本列出全部特性和所在章节。标 ★ 的是日常使用频率最高、建议优先掌握的。

---

## C++11：语言特性

### 类型与声明

| 特性 | 章节 | 一句话 |
|---|---|---|
| ★ `auto` | [01-10](../part1_basics/01-10_auto_decltype.md) | 类型推导；会丢弃引用和顶层 const |
| ★ `decltype` | [01-10](../part1_basics/01-10_auto_decltype.md) | 推导表达式类型；保留引用和 const |
| 尾置返回类型 | [01-10](../part1_basics/01-10_auto_decltype.md) | `auto f() -> decltype(...)`，解决返回类型依赖参数 |
| ★ `nullptr` | [01-06](../part1_basics/01-06_bool_nullptr_enum.md) | 类型是 `nullptr_t`，消除 `NULL` 的歧义 |
| ★ `enum class` | [01-06](../part1_basics/01-06_bool_nullptr_enum.md) | 强作用域、禁隐式转换、可指定底层类型 |
| ★ `using` 类型别名 | [06-03](../part6_modern_core/06-03_using.md) | 取代 `typedef`，支持模板化 |
| ★ 别名模板 | [06-03](../part6_modern_core/06-03_using.md) | `template <class T> using X = ...` |
| `constexpr` | [01-02](../part1_basics/01-02_const_constexpr.md) | 要求编译期可求值 |
| `inline namespace` | [01-01](../part1_basics/01-01_namespace.md) | 库的版本演进 |

### 初始化与语法糖

| 特性 | 章节 | 一句话 |
|---|---|---|
| ★ 列表初始化 `{}` | [01-11](../part1_basics/01-11_uniform_init.md) | 统一初始化语法；拒绝窄化转换 |
| `initializer_list` | [01-11](../part1_basics/01-11_uniform_init.md) | `{}` 背后的类型；构造函数优先级最高 |
| ★ 范围 for | [01-12](../part1_basics/01-12_range_for.md) | 默认用 `const auto&` |
| ★ 类内成员初始值 | [03-02](../part3_class/03-02_constructor.md) | 默认值写在声明处 |
| 原始字符串 | [01-09](../part1_basics/01-09_literals.md) | `R"(...)"`，正则和路径必用 |
| 自定义字面量 | [01-09](../part1_basics/01-09_literals.md) | `operator""_km`，单位进类型 |
| 二进制字面量 | [01-09](../part1_basics/01-09_literals.md) | `0b1010`（C++14 标准化）|

### 移动语义（最重要的一组）

| 特性 | 章节 | 一句话 |
|---|---|---|
| ★★ 右值引用 `&&` | [02-03](../part2_memory/02-03_move_semantics.md) | 区分"还要用"和"马上就没了" |
| ★★ 移动构造/赋值 | [02-03](../part2_memory/02-03_move_semantics.md) | 偷资源 + 置空源对象 + **标 `noexcept`** |
| ★★ `std::move` | [02-03](../part2_memory/02-03_move_semantics.md) | 只是 `static_cast`，自己不移动任何东西 |
| ★ 万能引用 | [02-04](../part2_memory/02-04_perfect_forwarding.md) | `T&&` + 推导；引用折叠 |
| ★ `std::forward` | [02-04](../part2_memory/02-04_perfect_forwarding.md) | 有条件转右值；必须写 `<T>` |

### 类相关

| 特性 | 章节 | 一句话 |
|---|---|---|
| ★ `= default` / `= delete` | [03-04](../part3_class/03-04_copy_control.md) | 明确表达意图；`delete` 可用于任何函数 |
| ★ `override` | [04-05](../part4_oop/04-05_override.md) | **零代价**，消除一整类覆盖失败的 bug |
| `final` | [04-05](../part4_oop/04-05_override.md) | 禁止覆盖/继承；帮助去虚化 |
| 委托构造 | [03-02](../part3_class/03-02_constructor.md) | 构造函数委托给另一个 |
| 继承构造 | [03-02](../part3_class/03-02_constructor.md) | `using Base::Base;` |
| 显式转换运算符 | [03-10](../part3_class/03-10_type_conversion.md) | `explicit operator bool()`，contextual conversion |

### C++98 的基础机制（本书也覆盖）

| 特性 | 章节 | 一句话 |
|---|---|---|
| 友元函数 / 友元类 | [03-09](../part3_class/03-09_operator_overload.md) | 不对称、不传递、**不继承**；避免友元类 |
| ★ 运算符重载 | [03-09](../part3_class/03-09_operator_overload.md) | 三种形式；对称运算符用非成员；别重载 `&&` `\|\|` `,` |
| `operator()` | [03-09](../part3_class/03-09_operator_overload.md) | 仿函数；lambda 的底层 |
| `operator[]` | [03-09](../part3_class/03-09_operator_overload.md) | 必须成员；const/非 const 成对 |
| `operator->` | [03-09](../part3_class/03-09_operator_overload.md) | 返回指针后编译器**自动继续应用 `->`** |
| `operator<<` / `>>` | [03-09](../part3_class/03-09_operator_overload.md) | 必须非成员 + 返回流引用 |
| 转换构造函数 | [03-10](../part3_class/03-10_type_conversion.md) | 单参数构造函数就是；默认加 `explicit` |
| 类型转换运算符 | [03-10](../part3_class/03-10_type_conversion.md) | `operator T() const`；优先用具名函数 |

### 模板

| 特性 | 章节 | 一句话 |
|---|---|---|
| ★ 可变参数模板 | [05-04](../part5_template/05-04_variadic.md) | 任意个数任意类型；配合 `forward` |
| `sizeof...` | [05-04](../part5_template/05-04_variadic.md) | 参数包长度，编译期常量 |
| ★ `static_assert` | [05-05](../part5_template/05-05_type_traits.md) | 编译期断言 |
| `extern template` | [05-06](../part5_template/05-06_template_engineering.md) | 减少目标文件体积（实测 -60%）|
| 函数模板默认实参 | [05-01](../part5_template/05-01_function_template.md) | C++98 只有类模板能用 |

### lambda 与可调用对象

| 特性 | 章节 | 一句话 |
|---|---|---|
| ★★ lambda | [06-01](../part6_modern_core/06-01_lambda.md) | 匿名类对象；捕获陷阱是重点 |
| ★ `std::function` | [06-02](../part6_modern_core/06-02_callable.md) | 类型擦除；代价是阻止内联 |
| `std::bind` | [06-02](../part6_modern_core/06-02_callable.md) | **优先用 lambda 替代** |
| `std::ref` / `cref` | [06-02](../part6_modern_core/06-02_callable.md) | 在 bind/thread 中传引用 |

### 其他

| 特性 | 章节 | 一句话 |
|---|---|---|
| ★ `noexcept` | [01-08](../part1_basics/01-08_exception.md) | 影响容器是否用移动；移动函数必标 |
| `[[noreturn]]` | [01-09](../part1_basics/01-09_literals.md) | 函数不返回 |
| 对齐控制 | [02-02](../part2_memory/02-02_new_delete.md) | `alignas` / `alignof` |
| 内存模型 | [08-04](../part8_concurrency/08-04_atomic.md) | 第一次定义多线程程序的行为 |

---

## C++11：库特性

| 特性 | 章节 | 一句话 |
|---|---|---|
| ★★ `unique_ptr` | [02-05](../part2_memory/02-05_smart_pointers.md) | 独占所有权，零开销，**默认选择** |
| ★ `shared_ptr` | [02-05](../part2_memory/02-05_smart_pointers.md) | 引用计数；注意循环引用 |
| ★ `weak_ptr` | [02-05](../part2_memory/02-05_smart_pointers.md) | 打破循环；必须 `lock()` 后用 |
| `make_shared` | [02-05](../part2_memory/02-05_smart_pointers.md) | 一次分配 + 异常安全 |
| ★ `array` | [07-02](../part7_stdlib/07-02_containers.md) | 零开销替代内置数组 |
| ★ `unordered_*` | [07-02](../part7_stdlib/07-02_containers.md) | 哈希容器，实测查找快 5.5 倍 |
| `forward_list` | [07-02](../part7_stdlib/07-02_containers.md) | 单向链表，省一个指针 |
| ★ emplace 系列 | [07-02](../part7_stdlib/07-02_containers.md) | 完美转发，省掉临时对象 |
| ★ `tuple` | [07-04](../part7_stdlib/07-04_tuple_variant.md) | 多返回值（但更推荐具名 struct）|
| ★ `chrono` | [07-05](../part7_stdlib/07-05_chrono_random.md) | 单位进类型；计时用 `steady_clock` |
| ★ `random` | [07-05](../part7_stdlib/07-05_chrono_random.md) | 引擎 + 分布；取代 `rand()` |
| `regex` | [07-06](../part7_stdlib/07-06_regex_filesystem.md) | 功能完备但慢；regex 对象必复用 |
| ★ `thread` | [08-01](../part8_concurrency/08-01_thread.md) | 析构前必须 join/detach |
| ★ `mutex` / `lock_guard` | [08-02](../part8_concurrency/08-02_mutex.md) | RAII 加锁 |
| `unique_lock` | [08-02](../part8_concurrency/08-02_mutex.md) | 五种额外能力；配合条件变量 |
| ★ `condition_variable` | [08-03](../part8_concurrency/08-03_condition_variable.md) | **必须用带谓词的 wait** |
| ★ `atomic` | [08-04](../part8_concurrency/08-04_atomic.md) | 不等于无锁；大对象退化为加锁 |
| ★ `future` / `async` | [08-05](../part8_concurrency/08-05_async.md) | 异常自动传递；必写 `launch::async` |
| `promise` | [08-05](../part8_concurrency/08-05_async.md) | 写端/读端分离 |
| `packaged_task` | [08-05](../part8_concurrency/08-05_async.md) | 线程池的基础 |
| `type_traits` | [05-05](../part5_template/05-05_type_traits.md) | 编译期类型查询与变换 |
| `thread_local` | [08-01](../part8_concurrency/08-01_thread.md) | 每线程一份，无需同步 |

---

## C++14

全部特性见 [06-04 C++14 特性汇总](../part6_modern_core/06-04_cpp14.md)。

| 特性 | 章节 | 一句话 |
|---|---|---|
| ★ 返回类型推导 | [01-10](../part1_basics/01-10_auto_decltype.md) | `auto f() { return x; }` |
| ★ `decltype(auto)` | [01-10](../part1_basics/01-10_auto_decltype.md) | 保留引用的返回类型推导 |
| ★★ 泛型 lambda | [06-01](../part6_modern_core/06-01_lambda.md) | `[](auto a, auto b) {}` |
| ★★ 初始化捕获 | [06-01](../part6_modern_core/06-01_lambda.md) | `[p = std::move(ptr)]`，支持移动捕获 |
| ★★ `make_unique` | [02-05](../part2_memory/02-05_smart_pointers.md) | 补上 C++11 的疏漏 |
| ★ 变量模板 | [05-02](../part5_template/05-02_class_template.md) | `_v` 后缀的基础 |
| ★★ 放宽的 `constexpr` | [01-02](../part1_basics/01-02_const_constexpr.md) | 允许循环和分支，编译期计算变实用 |
| `[[deprecated]]` | [01-09](../part1_basics/01-09_literals.md) | 废弃警告 |
| ★ 数字分隔符 | [01-09](../part1_basics/01-09_literals.md) | `1'000'000` |
| ★ `s` / `ms` / `h` 后缀 | [01-09](../part1_basics/01-09_literals.md) | 字符串和时间字面量 |
| `std::exchange` | [06-04](../part6_modern_core/06-04_cpp14.md) | 实现移动语义的好帮手 |
| `integer_sequence` | [06-04](../part6_modern_core/06-04_cpp14.md) | `std::apply` 的基础 |
| ★ `_t` 别名 | [06-03](../part6_modern_core/06-03_using.md) | 省掉 `typename` 和 `::type` |
| `shared_timed_mutex` | [08-02](../part8_concurrency/08-02_mutex.md) | 读写锁 |

---

## C++17：语言特性

全部特性见 [06-05 C++17 语言特性汇总](../part6_modern_core/06-05_cpp17.md)。

| 特性 | 章节 | 一句话 |
|---|---|---|
| ★★ 结构化绑定 | [01-12](../part1_basics/01-12_range_for.md) | `auto [k, v] = pair;` |
| ★★ if/switch 初始化语句 | [01-12](../part1_basics/01-12_range_for.md) | `if (auto it = find(); it != end)` |
| ★★ `if constexpr` | [05-05](../part5_template/05-05_type_traits.md) | 不成立的分支不实例化 |
| ★★ 折叠表达式 | [05-04](../part5_template/05-04_variadic.md) | `(args + ...)` |
| ★★ CTAD | [05-02](../part5_template/05-02_class_template.md) | 类模板实参推导 |
| ★ 内联变量 | [01-02](../part1_basics/01-02_const_constexpr.md) | 头文件里定义唯一实体 |
| ★ `static inline` 成员 | [03-05](../part3_class/03-05_special_members.md) | 不需要类外定义 |
| 嵌套命名空间 | [01-01](../part1_basics/01-01_namespace.md) | `namespace a::b::c {}` |
| 保证的拷贝消除 | [02-03](../part2_memory/02-03_move_semantics.md) | 能返回不可移动的类型 |
| `auto` 非类型模板参数 | [05-01](../part5_template/05-01_function_template.md) | `template <auto V>` |
| `constexpr` lambda | [06-01](../part6_modern_core/06-01_lambda.md) | 满足条件时隐式 constexpr |
| ★ `[*this]` 捕获 | [06-01](../part6_modern_core/06-01_lambda.md) | 按值捕获整个对象 |
| `noexcept` 进类型系统 | [06-05](../part6_modern_core/06-05_cpp17.md) | 函数指针类型区分 noexcept |
| ★ `[[nodiscard]]` | [01-09](../part1_basics/01-09_literals.md) | 返回值被忽略时警告 |
| `[[maybe_unused]]` | [01-09](../part1_basics/01-09_literals.md) | 抑制未使用警告 |
| `[[fallthrough]]` | [01-09](../part1_basics/01-09_literals.md) | 声明故意落穿 |
| `__has_include` | [06-05](../part6_modern_core/06-05_cpp17.md) | 条件包含头文件 |
| 单参数 `static_assert` | [06-05](../part6_modern_core/06-05_cpp17.md) | 可省略消息 |

---

## C++17：库特性

| 特性 | 章节 | 一句话 |
|---|---|---|
| ★★ `optional` | [07-04](../part7_stdlib/07-04_tuple_variant.md) | "可能没有值"进类型 |
| ★★ `variant` | [07-04](../part7_stdlib/07-04_tuple_variant.md) | 类型安全的 union；可替代封闭集合的虚函数 |
| `any` | [07-04](../part7_stdlib/07-04_tuple_variant.md) | 任意类型；慎用 |
| ★★ `string_view` | [07-01](../part7_stdlib/07-01_string.md) | 零拷贝字符串视图；**只做参数** |
| ★ `filesystem` | [07-06](../part7_stdlib/07-06_regex_filesystem.md) | 路径拼接用 `/`；双接口错误处理 |
| 并行算法 | [07-03](../part7_stdlib/07-03_algorithms.md) | **libstdc++ 需要 TBB，否则静默串行** |
| ★ `std::apply` | [07-04](../part7_stdlib/07-04_tuple_variant.md) | 把 tuple 展开成实参 |
| ★ `std::invoke` | [06-02](../part6_modern_core/06-02_callable.md) | 统一五种可调用对象的调用语法 |
| `is_invocable` | [06-02](../part6_modern_core/06-02_callable.md) | 检测可调用性 |
| ★ `scoped_lock` | [08-02](../part8_concurrency/08-02_mutex.md) | 多锁免死锁；应取代 `lock_guard` |
| `shared_mutex` | [08-02](../part8_concurrency/08-02_mutex.md) | 读写锁；**实测高并发下可能更慢** |
| ★ `_v` 后缀 | [06-03](../part6_modern_core/06-03_using.md) | `is_same_v` 等 |
| `void_t` | [05-05](../part5_template/05-05_type_traits.md) | 检测成员是否存在 |
| ★ `from_chars` / `to_chars` | [07-01](../part7_stdlib/07-01_string.md) | 最快的数值转换，无异常无分配 |
| `try_emplace` | [07-02](../part7_stdlib/07-02_containers.md) | 键已存在时不构造值 |
| `insert_or_assign` | [07-02](../part7_stdlib/07-02_containers.md) | 不要求值可默认构造 |
| `clamp` / `gcd` / `lcm` | [07-03](../part7_stdlib/07-03_algorithms.md) | 小工具 |
| `reduce` / `transform_reduce` | [07-03](../part7_stdlib/07-03_algorithms.md) | 并行版 accumulate；浮点结果可能不同 |
| `sample` | [07-03](../part7_stdlib/07-03_algorithms.md) | 随机抽样 |
| `std::byte` | — | 表达"原始字节"而非小整数 |

---

## 按优先级的学习路径

### 第一优先（不懂就读不懂现代 C++ 代码）

1. [02-03 右值引用与移动语义](../part2_memory/02-03_move_semantics.md)
2. [02-05 智能指针](../part2_memory/02-05_smart_pointers.md)
3. [06-01 lambda 表达式](../part6_modern_core/06-01_lambda.md)
4. [01-10 auto 与 decltype](../part1_basics/01-10_auto_decltype.md)
5. [01-11 统一初始化](../part1_basics/01-11_uniform_init.md)
6. [01-12 范围 for 与结构化绑定](../part1_basics/01-12_range_for.md)

### 第二优先（写类和模板时需要）

7. [03-04 拷贝控制与 Rule of Five](../part3_class/03-04_copy_control.md)
8. [02-04 完美转发](../part2_memory/02-04_perfect_forwarding.md)
9. [04-05 override/final](../part4_oop/04-05_override.md)
10. [05-04 可变模板参数](../part5_template/05-04_variadic.md)
11. [05-05 type_traits 与 if constexpr](../part5_template/05-05_type_traits.md)

### 第三优先（按需查阅）

12. [07-04 optional/variant](../part7_stdlib/07-04_tuple_variant.md)
13. [07-01 string_view](../part7_stdlib/07-01_string.md)
14. [Part 8 并发](../part8_concurrency/)（写多线程代码时）
15. [07-05 chrono](../part7_stdlib/07-05_chrono_random.md)

---

## 实测数据汇总

本书所有性能数字都在 g++ 11.4 / x86-64 / 128 核上实测。
汇总如下，供快速查阅：

| 对比 | 结果 | 出处 |
|---|---|---|
| 堆分配 vs 栈分配 | 堆慢 **26 倍** | [02-01](../part2_memory/02-01_memory_layout.md) |
| 移动 vs 拷贝（1000 字节 string）| 移动快 **8.9 倍** | [02-03](../part2_memory/02-03_move_semantics.md) |
| 移动函数漏写 `noexcept` | 容器扩容慢 **1.6 倍** | [02-03](../part2_memory/02-03_move_semantics.md) |
| 虚函数 vs CRTP（`-O2`）| 虚函数慢约 **4 倍**（含缓存因素）| [04-07](../part4_oop/04-07_crtp.md) |
| 模板递归 vs `constexpr` | 模板多用 **4.7 倍**内存 | [05-06](../part5_template/05-06_template_engineering.md) |
| `extern template` | 目标文件小 **60%** | [05-06](../part5_template/05-06_template_engineering.md) |
| SSO 短字符串 vs 堆字符串 | 短的快一倍多 | [07-01](../part7_stdlib/07-01_string.md) |
| `string_view::substr` vs `string::substr` | 快 **3.2 倍** | [07-01](../part7_stdlib/07-01_string.md) |
| `unordered_map` vs `map`（查找）| 快 **5.5 倍** | [07-02](../part7_stdlib/07-02_containers.md) |
| 循环内构造 `regex` | 慢 **102 倍** | [07-06](../part7_stdlib/07-06_regex_filesystem.md) |
| 手写解析 vs 复用 regex | 手写快 **390 倍** | [07-06](../part7_stdlib/07-06_regex_filesystem.md) |
| `std::endl` vs `"\n"` | `endl` 慢 **15 倍** | [07-07](../part7_stdlib/07-07_iostream.md) |
| `to_string` vs `ostringstream` | 快 **5 倍** | [07-07](../part7_stdlib/07-07_iostream.md) |
| `atomic` vs `mutex` | atomic 快 **13 倍** | [08-04](../part8_concurrency/08-04_atomic.md) |
| 不共享 vs atomic | 不共享快到测不出 | [08-04](../part8_concurrency/08-04_atomic.md) |
| 手工并行归约 | 8 线程快 **7.7 倍** | [08-05](../part8_concurrency/08-05_async.md) |

### 几个反直觉的实测结果

| 发现 | 说明 | 出处 |
|---|---|---|
| **并行算法可能完全无效** | libstdc++ 缺 TBB 时静默串行 | [07-03](../part7_stdlib/07-03_algorithms.md) |
| **`shared_mutex` 可能更慢** | 64 线程 + 短临界区时慢 19% | [08-02](../part8_concurrency/08-02_mutex.md) |
| **内存序在 x86 上无差别** | `fetch_add` 三种内存序生成相同汇编 | [08-04](../part8_concurrency/08-04_atomic.md) |
| **ASan 检测不到循环引用** | 栈上残留指针让 LSan 误判为可达 | [02-05](../part2_memory/02-05_smart_pointers.md) |

> 下一章：[09-02 高频面试题解答](09-02_interview.md)
