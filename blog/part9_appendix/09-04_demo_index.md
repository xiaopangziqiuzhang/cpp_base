# 09-04 demo 索引

全部 62 个 demo 文件，14392 行代码，全部经
[`scripts/build_all.sh`](../../scripts/build_all.sh) 编译并运行验证通过。

---

## 怎么编译

每个 demo 的首行注明了所需标准和链接选项：

```cpp
// std=c++17 flags=-pthread
```

照此编译：

```bash
g++ -std=c++17 -Wall -Wextra -pthread demos/part8_concurrency/08_01_thread.cpp -o /tmp/a && /tmp/a
```

批量编译（增量、全核并行、支持断点续跑）：

```bash
./scripts/build_all.sh                   # 全部
./scripts/build_all.sh part2_memory      # 只编某个目录
./scripts/build_all.sh 02_03             # 只编匹配的文件
./scripts/build_all.sh --force           # 强制重编
./scripts/build_all.sh --jobs 16         # 限制并行度
```

产物位置：

```
build/bin/    可执行文件
build/out/    运行输出（正文引用的"实测输出"都来自这里）
build/log/    编译日志
build/state/  断点续跑的状态标记
```

---

## Part 0 导读

| 文件 | 标准 | 行数 | 主题 |
|---|---|---|---|
| [`00_01_standard_probe.cpp`](../../demos/part0_overview/00_01_standard_probe.cpp) | c++17 | 92 | 探测当前编译器与标准版本 |

对应章节：[00-01](../part0_overview/00-01_history.md)、[00-02](../part0_overview/00-02_environment.md)

---

## Part 1 基础语法

| 文件 | 标准 | 行数 | 主题 |
|---|---|---|---|
| [`01_01_namespace.cpp`](../../demos/part1_basics/01_01_namespace.cpp) | c++17 | 150 | 命名空间三种用法、匿名空间、嵌套简化、inline namespace、ADL |
| [`01_01_using_pitfall.cpp`](../../demos/part1_basics/01_01_using_pitfall.cpp) | c++17 | 81 | `using namespace std` 的三个具体问题 |
| [`01_02_const.cpp`](../../demos/part1_basics/01_02_const.cpp) | c++17 | 140 | const 四种修饰、const vs 宏 |
| [`01_02_constexpr.cpp`](../../demos/part1_basics/01_02_constexpr.cpp) | c++17 | 141 | constexpr 从 C++11 到 C++17 的演进 |
| [`01_03_reference.cpp`](../../demos/part1_basics/01_03_reference.cpp) | c++17 | 161 | 引用的本质、做参数、做返回值、与指针对比 |
| [`01_04_cast.cpp`](../../demos/part1_basics/01_04_cast.cpp) | c++17 | 171 | 四种 cast + C 风格转换的危险 |
| [`01_05_overload.cpp`](../../demos/part1_basics/01_05_overload.cpp) | c++17 | 136 | 函数重载、name mangling、重载决议、默认参数 |
| [`01_06_nullptr_enum.cpp`](../../demos/part1_basics/01_06_nullptr_enum.cpp) | c++17 | 158 | bool、NULL 的问题与 nullptr、enum class |
| [`01_07_inline.cpp`](../../demos/part1_basics/01_07_inline.cpp) | c++17 | 127 | inline vs 宏、inline 的现代语义 |
| [`01_08_exception.cpp`](../../demos/part1_basics/01_08_exception.cpp) | c++17 | 222 | try/catch/throw、标准异常体系、noexcept、RAII |
| [`01_09_literals.cpp`](../../demos/part1_basics/01_09_literals.cpp) | c++17 | 195 | 原始字符串、自定义字面量、数字分隔符、属性 |
| [`01_10_auto_basic.cpp`](../../demos/part1_basics/01_10_auto_basic.cpp) | **c++14** | 126 | auto 推导规则与三个陷阱 |
| [`01_10_decltype.cpp`](../../demos/part1_basics/01_10_decltype.cpp) | **c++14** | 100 | decltype、尾置返回、decltype(auto) |
| [`01_11_uniform_init.cpp`](../../demos/part1_basics/01_11_uniform_init.cpp) | c++17 | 188 | 统一初始化、initializer_list、most vexing parse |
| [`01_12_range_for.cpp`](../../demos/part1_basics/01_12_range_for.cpp) | c++17 | 213 | 范围 for、自定义类型适配、结构化绑定 |

---

## Part 2 内存模型与资源管理

| 文件 | 标准 | flags | 行数 | 主题 |
|---|---|---|---|---|
| [`02_01_memory_layout.cpp`](../../demos/part2_memory/02_01_memory_layout.cpp) | c++17 | | 129 | **实测各内存区地址**、堆栈速度对比 |
| [`02_02_new_delete.cpp`](../../demos/part2_memory/02_02_new_delete.cpp) | c++17 | | 217 | new/delete 工作步骤、operator new 重载、placement new |
| [`02_03_move_semantics.cpp`](../../demos/part2_memory/02_03_move_semantics.cpp) | c++17 | | 264 | 值类别、移动构造/赋值、**noexcept 的性能影响** |
| [`02_04_perfect_forwarding.cpp`](../../demos/part2_memory/02_04_perfect_forwarding.cpp) | c++17 | | 200 | 万能引用、引用折叠、forward、emplace 原理 |
| [`02_05_smart_pointers.cpp`](../../demos/part2_memory/02_05_smart_pointers.cpp) | c++17 | | 246 | 三种智能指针、**循环引用的实测泄漏**、自定义删除器 |
| [`02_06_raii.cpp`](../../demos/part2_memory/02_06_raii.cpp) | c++17 | `-pthread` | 211 | RAII 原理、scope guard、copy-and-swap |

---

## Part 3 类与对象

| 文件 | 标准 | flags | 行数 | 主题 |
|---|---|---|---|---|
| [`03_01_class_basics.cpp`](../../demos/part3_class/03_01_class_basics.cpp) | c++17 | | 209 | class vs struct、**this 指针实测**、空指针调用成员函数 |
| [`03_02_constructor.cpp`](../../demos/part3_class/03_02_constructor.cpp) | c++17 | | 271 | **初始化顺序陷阱**（故意保留警告）、委托构造、继承构造 |
| [`03_03_destructor.cpp`](../../demos/part3_class/03_03_destructor.cpp) | c++17 | | 175 | 各种存储期对象的生灭时机与顺序 |
| [`03_04_copy_control.cpp`](../../demos/part3_class/03_04_copy_control.cpp) | c++17 | | 266 | 深浅拷贝、copy-and-swap、**五大函数的生成规则** |
| [`03_05_special_members.cpp`](../../demos/part3_class/03_05_special_members.cpp) | c++17 | | 239 | const/引用/类对象/static 成员、C++17 inline 静态成员 |
| [`03_06_const_member.cpp`](../../demos/part3_class/03_06_const_member.cpp) | c++17 | `-pthread` | 236 | **const 成员函数的本质**、const 重载、mutable |
| [`03_07_singleton.cpp`](../../demos/part3_class/03_07_singleton.cpp) | c++17 | `-pthread` | 218 | **单例模式的四次进化**（含多线程验证）|
| [`03_08_pimpl.cpp`](../../demos/part3_class/03_08_pimpl.cpp) | c++17 | | 195 | Pimpl 惯用法、不完整类型问题 |
| [`03_09_operator_overload.cpp`](../../demos/part3_class/03_09_operator_overload.cpp) | c++17 | | 446 | 友元、**三种重载形式**、特殊运算符的重载 |
| [`03_10_type_conversion.cpp`](../../demos/part3_class/03_10_type_conversion.cpp) | c++17 | | 325 | 转换构造函数、**explicit operator bool**、二义性 |

---

## Part 4 面向对象

| 文件 | 标准 | 行数 | 主题 |
|---|---|---|---|
| [`04_01_inheritance.cpp`](../../demos/part4_oop/04_01_inheritance.cpp) | c++17 | 251 | 三种派生方式、名字隐藏、**空基类优化** |
| [`04_02_derived_lifecycle.cpp`](../../demos/part4_oop/04_02_derived_lifecycle.cpp) | c++17 | 238 | 构造/析构顺序、**对象切片实测**、派生类复制控制 |
| [`04_03_diamond.cpp`](../../demos/part4_oop/04_03_diamond.cpp) | c++17 | 218 | 菱形继承、虚继承、**对象布局地址对比** |
| [`04_04_virtual.cpp`](../../demos/part4_oop/04_04_virtual.cpp) | c++17 | 247 | **虚表实测**（直接通过槽位调用）、构造析构中的虚调用 |
| [`04_05_override.cpp`](../../demos/part4_oop/04_05_override.cpp) | c++17 | 238 | 重载/隐藏/覆盖、**静默的覆盖失败**、final |
| [`04_06_abstract.cpp`](../../demos/part4_oop/04_06_abstract.cpp) | c++17 | 278 | 纯虚函数、抽象类、**虚析构函数的必要性实测** |
| [`04_07_crtp.cpp`](../../demos/part4_oop/04_07_crtp.cpp) | c++17 | 217 | **虚函数开销实测**、CRTP 静态多态 |

---

## Part 5 模板与泛型编程

| 文件 | 标准 | 行数 | 主题 |
|---|---|---|---|
| [`05_01_function_template.cpp`](../../demos/part5_template/05_01_function_template.cpp) | c++17 | 200 | 函数模板、推导限制、非类型参数、auto 非类型参数 |
| [`05_02_class_template.cpp`](../../demos/part5_template/05_02_class_template.cpp) | c++17 | 266 | 类模板、成员函数模板、**CTAD**、变量模板 |
| [`05_03_specialization.cpp`](../../demos/part5_template/05_03_specialization.cpp) | c++17 | 267 | 全特化、偏特化、**三个坑**（含"特化永不被调用"）|
| [`05_04_variadic.cpp`](../../demos/part5_template/05_04_variadic.cpp) | c++17 | 256 | 参数包展开的四种方式、**折叠表达式**、tuple 原理 |
| [`05_05_type_traits.cpp`](../../demos/part5_template/05_05_type_traits.cpp) | c++17 | 266 | static_assert、SFINAE、**优先级标签**、if constexpr |
| [`05_06_template_engineering.cpp`](../../demos/part5_template/05_06_template_engineering.cpp) | c++17 | 253 | 两阶段查找、typename/template 消歧、**编译期开销** |

---

## Part 6 C++11/14/17 核心特性

| 文件 | 标准 | 行数 | 主题 |
|---|---|---|---|
| [`06_01_lambda.cpp`](../../demos/part6_modern_core/06_01_lambda.cpp) | c++17 | 290 | **闭包本质**、捕获陷阱、泛型 lambda、初始化捕获 |
| [`06_02_callable.cpp`](../../demos/part6_modern_core/06_02_callable.cpp) | c++17 | 288 | 五种可调用对象、**std::function 的代价**、invoke |
| [`06_03_using.cpp`](../../demos/part6_modern_core/06_03_using.cpp) | c++17 | 238 | typedef vs using、别名模板、`_t`/`_v` 惯例 |
| [`06_04_cpp14.cpp`](../../demos/part6_modern_core/06_04_cpp14.cpp) | **c++14** | 223 | C++14 全部特性（**用 -std=c++14 编译以证明**）|
| [`06_05_cpp17.cpp`](../../demos/part6_modern_core/06_05_cpp17.cpp) | c++17 | 268 | C++17 语言特性汇总 |

---

## Part 7 标准库

| 文件 | 标准 | flags | 行数 | 主题 |
|---|---|---|---|---|
| [`07_01_string.cpp`](../../demos/part7_stdlib/07_01_string.cpp) | c++17 | | 289 | **SSO 阈值实测**、string_view 的三个陷阱、from_chars |
| [`07_02_containers.cpp`](../../demos/part7_stdlib/07_02_containers.cpp) | c++17 | | 286 | 容器全景、**迭代器失效规则**、vector 增长实测 |
| [`07_03_algorithms.cpp`](../../demos/part7_stdlib/07_03_algorithms.cpp) | c++17 | `-pthread` | 292 | 迭代器分类、erase-remove、**并行算法的 TBB 陷阱** |
| [`07_04_tuple_variant.cpp`](../../demos/part7_stdlib/07_04_tuple_variant.cpp) | c++17 | | 298 | tuple/optional/variant/any、**variant 替代虚函数** |
| [`07_05_chrono_random.cpp`](../../demos/part7_stdlib/07_05_chrono_random.cpp) | c++17 | | 289 | chrono 三概念、**取模偏差**、引擎 + 分布 |
| [`07_06_regex_filesystem.cpp`](../../demos/part7_stdlib/07_06_regex_filesystem.cpp) | c++17 | | 296 | regex 三操作、**性能实测（102 倍）**、filesystem |
| [`07_07_iostream.cpp`](../../demos/part7_stdlib/07_07_iostream.cpp) | c++17 | | 327 | 流状态、**endl 的代价**、文件 IO、stringstream |

---

## Part 8 并发

| 文件 | 标准 | flags | 行数 | 主题 |
|---|---|---|---|---|
| [`08_01_thread.cpp`](../../demos/part8_concurrency/08_01_thread.cpp) | c++17 | `-pthread` | 259 | 创建/join/detach、传参陷阱、**数据竞争实测**、thread_local |
| [`08_02_mutex.cpp`](../../demos/part8_concurrency/08_02_mutex.cpp) | c++17 | `-pthread` | 308 | mutex 家族、scoped_lock、**shared_mutex 的反直觉实测** |
| [`08_03_condition_variable.cpp`](../../demos/part8_concurrency/08_03_condition_variable.cpp) | c++17 | `-pthread` | 271 | 虚假唤醒、**完整的阻塞队列**（TSan 验证）|
| [`08_04_atomic.cpp`](../../demos/part8_concurrency/08_04_atomic.cpp) | c++17 | `-pthread` | 363 | CAS、六种内存序、**无锁栈**、内存序性能真相 |
| [`08_05_async.cpp`](../../demos/part8_concurrency/08_05_async.cpp) | c++17 | `-pthread` | 364 | future/promise/packaged_task、**完整线程池**（TSan 验证）|

---

## 按用途索引

### 想看某个特性怎么用

查 [09-01 特性速查表](09-01_cheatsheet.md)，它按标准版本列出全部特性和章节。

### 想看某个实测数据怎么来的

| 数据 | demo |
|---|---|
| 堆分配比栈慢 26 倍 | `02_01_memory_layout.cpp` |
| 移动比拷贝快 8.9 倍 | `02_03_move_semantics.cpp` |
| 漏写 noexcept 慢 1.6 倍 | `02_03_move_semantics.cpp` |
| 虚表的地址和槽位 | `04_04_virtual.cpp` |
| 虚函数 vs CRTP | `04_07_crtp.cpp` |
| SSO 阈值是 15 字符 | `07_01_string.cpp` |
| unordered_map 快 5.5 倍 | `07_02_containers.cpp` |
| 并行算法无效（缺 TBB）| `07_03_algorithms.cpp` |
| regex 循环内构造慢 102 倍 | `07_06_regex_filesystem.cpp` |
| endl 慢 15 倍 | `07_07_iostream.cpp` |
| 数据竞争丢失 75% 更新 | `08_01_thread.cpp` |
| shared_mutex 高并发更慢 | `08_02_mutex.cpp` |
| atomic 比 mutex 快 13 倍 | `08_04_atomic.cpp` |
| 手工并行归约快 7.7 倍 | `08_05_async.cpp` |

### 想要可直接用的代码

这些 demo 里的实现可以直接拿去用（或作为起点）：

| 代码 | 位置 |
|---|---|
| `ScopeGuard`（scope guard）| `02_06_raii.cpp` |
| `ThreadGuard`（RAII 线程）| `08_01_thread.cpp` |
| `BlockingQueue`（阻塞队列）| `08_03_condition_variable.cpp` |
| `ThreadPool`（线程池）| `08_05_async.cpp` |
| `Timer`（计时器）| `07_05_chrono_random.cpp` |
| Meyers Singleton | `03_07_singleton.cpp` |
| Pimpl 模板 | `03_08_pimpl.cpp` |
| CRTP mixin | `04_07_crtp.cpp` |
| 优先级标签分发 | `05_05_type_traits.cpp` |
| 强类型单位（`Bytes` + `_KB`）| `03_10_type_conversion.cpp` |
| `explicit operator bool` 模式 | `03_10_type_conversion.cpp` |
| `thread_local` 随机数引擎 | `07_05_chrono_random.cpp` |

其中 `BlockingQueue` 和 `ThreadPool` 经过 ThreadSanitizer 压测验证。

**但 `08_04_atomic.cpp` 里的无锁栈不要直接用** ——
它有 ABA 和内存回收问题，只用于说明原理。

---

## 关于 demo 的编写约定

1. **每个文件可独立编译运行**，不依赖其他文件
2. **首行注明标准和链接选项**，构建脚本据此编译
3. **错误写法被注释掉并标明原因**，保证整个文件能编过
   —— 想亲眼看报错就取消注释
4. **用 namespace 分节**，避免名字冲突
5. **成员变量用 `_` 前缀**（与参考文档一致）
6. **一律写 `std::`**，不用 `using namespace std`
   —— 原因见 [01-01](../part1_basics/01-01_namespace.md)
7. **性能测量都用 `steady_clock`**，并说明编译优化级别

### 故意保留警告的文件

| 文件 | 警告 | 原因 |
|---|---|---|
| `01_06_nullptr_enum.cpp` | `-Wenum-compare` | 演示传统 enum 可以跨类型比较 |
| `03_02_constructor.cpp` | `-Wreorder`、`-Wuninitialized` | 演示初始化顺序陷阱 |
| `04_02_derived_lifecycle.cpp` | `-Wextra`（基类未在拷贝构造中初始化）| 演示忘记调用基类拷贝构造 |

这三个警告**都是各章要演示的 bug**，编译器的提示本身就是论证的一部分
（正文里引用了这些警告文本）。其余 59 个文件在 `-Wall -Wextra` 下零警告。

---

## 验证全书代码

```bash
cd cpp_1111
./scripts/build_all.sh --force
```

预期输出：

```
共 62 个 demo，并行度 128
OK  part0_overview/00_01_standard_probe.cpp
...
----------------------------------------
成功: 62  跳过: 0  失败: 0
```

如果某个 demo 失败，查 `build/log/<name>.compile.log`（编译错误）
或 `build/out/<name>.out`（运行输出）。

---

全书完。

> 返回 [总目录](../README.md)
