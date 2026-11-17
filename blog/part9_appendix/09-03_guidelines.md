# 09-03 现代 C++ 编码清单

一份可执行的检查表。每条都指向本书对应章节的论证。

---

## 一、资源管理

### 不要手工管理资源

| 不要 | 要 | 出处 |
|---|---|---|
| `new` / `delete` | `make_unique` / `make_shared` / `vector` | [02-05](../part2_memory/02-05_smart_pointers.md) |
| `malloc` / `free` | 同上 | [02-02](../part2_memory/02-02_new_delete.md) |
| `char*` + `strcpy` | `std::string` | [07-01](../part7_stdlib/07-01_string.md) |
| 裸数组 + 手工大小 | `std::vector` / `std::array` | [07-02](../part7_stdlib/07-02_containers.md) |
| `mtx.lock()` / `unlock()` | `std::scoped_lock` | [08-02](../part8_concurrency/08-02_mutex.md) |
| `fopen` / `fclose` | `std::fstream` | [07-07](../part7_stdlib/07-07_iostream.md) |
| 手工 `join` | RAII 包装（或 C++20 `jthread`）| [08-01](../part8_concurrency/08-01_thread.md) |

理由：手工管理在**异常面前不可靠** —— 中途抛异常就泄漏。
见 [01-08](../part1_basics/01-08_exception.md)、[02-06](../part2_memory/02-06_raii.md)。

### 用类型表达所有权

```cpp
std::unique_ptr<T>     // 我独占，我负责释放
std::shared_ptr<T>     // 共享，最后一个走的人释放
std::weak_ptr<T>       // 只观察，用前要检查
T& / T*                // 只借用，不负责释放（T* 表示可为空）
```

**默认用 `unique_ptr`** —— 零开销，而且以后改成 `shared_ptr` 容易，反之很难。

见 [02-05](../part2_memory/02-05_smart_pointers.md)。

### 智能指针的注意点

- 用 `make_unique`/`make_shared`，不要 `new`
- **循环引用会泄漏**，反向指针用 `weak_ptr`
- `weak_ptr` 必须 `lock()` 后用，不要先 `expired()` 再 `lock()`
- 函数参数用 `const T&` 或 `T*`，**不要用 `const shared_ptr<T>&`**
  （暴露了不必要的所有权细节）
- `shared_from_this()` 要求对象已被 `shared_ptr` 管理，不能在构造函数里用

---

## 二、类的设计

### Rule of Zero 优先

```cpp
// 优先：成员都是 RAII 类型，五个特殊函数一个都不写
class Good {
    std::string _name;
    std::vector<int> _data;
};
```

**要么一个都不声明，要么全部五个都声明。** 只声明一两个会出意外：
声明了析构函数，移动函数就不再自动生成（退化为拷贝，**无警告**）。

见 [03-04](../part3_class/03-04_copy_control.md)。

### 特殊成员函数清单

```cpp
class Resource {
public:
    ~Resource();                                   // 1
    Resource(const Resource&);                     // 2
    Resource& operator=(const Resource&);          // 3
    Resource(Resource&&) noexcept;                 // 4  <- noexcept 必须有
    Resource& operator=(Resource&&) noexcept;      // 5  <- noexcept 必须有
};
```

- **手写移动函数必须加 `noexcept`**，否则容器扩容退回拷贝（实测慢 1.6 倍）
- 考虑用 copy-and-swap 把 3 和 5 合并成一个 `operator=(Resource rhs)`
- 用 `= default` / `= delete` 明确表达意图

### const 正确性

- **所有不修改对象状态的成员函数都加 `const`**
- const 和非 const 版本重复时，非 const 版本转调 const 版本 + `const_cast`
  （**不能反方向**）
- 注意**浅 const**：`T* data() const` 会泄漏可写访问，
  const 版本要返回 `const T*`
- `mutable` 用于互斥量和缓存；用了就要保证线程安全

见 [03-06](../part3_class/03-06_const_member.md)。

### 构造函数

- **单参数构造函数默认加 `explicit`**（除非明确想要隐式转换）
- **初始化列表的顺序与成员声明顺序一致**，开 `-Wreorder`
- 默认值用类内成员初始值，不要在每个构造函数里重复
- 多个构造函数共享逻辑用**委托构造**，不要用 `init()` 函数
- 不要让一个成员的初始化依赖另一个成员

见 [03-02](../part3_class/03-02_constructor.md)。

---

## 三、继承与多态

### 继承的使用

- **显式写出继承方式**（`class` 默认是 private 继承！）
- `public` 继承表达 **is-a**；其他两种表达"用它实现"
- **能用组合就不要用 private 继承**
- 多继承时**最多一个基类带数据和实现**，其余是纯接口
- 派生类定义同名函数时加 `using Base::f;` 保留基类重载

见 [04-01](../part4_oop/04-01_inheritance.md)。

### 虚函数

- **覆盖虚函数一律写 `override`** —— 零代价，消除一整类 bug
- 写了 `override` 就不要再写 `virtual`
- 开 `-Wsuggest-override` 检查漏写
- **有虚函数的类，析构函数要是虚的**
- 不打算被继承的类标 `final`，不要加 `virtual`
- **不要在构造/析构函数里调用虚函数**
- **虚函数不要带默认参数**（默认参数静态绑定，虚函数动态绑定）
- 多态类型用指针/引用传递，容器存智能指针（防切片）

见 [04-04](../part4_oop/04-04_virtual.md)、[04-05](../part4_oop/04-05_override.md)、
[04-06](../part4_oop/04-06_abstract.md)。

### 什么时候不用虚函数

| 情况 | 替代方案 |
|---|---|
| 类型集合封闭且编译期已知 | `std::variant` + `std::visit` |
| 需要给一批类注入通用实现 | CRTP |
| 只需要存储行为 | `std::function` |
| 纯泛型（鸭子类型） | 模板 |

见 [04-07](../part4_oop/04-07_crtp.md)、
[07-04](../part7_stdlib/07-04_tuple_variant.md)。

---

## 四、函数接口

### 参数传递

| 情况 | 写法 |
|---|---|
| 只读、大对象 | `const T&` |
| 只读、字符串 | `std::string_view`（C++17）|
| 只读、小对象（≤2 个指针大小）| `T`（按值）|
| 要修改实参 | `T&` |
| 可以为空 | `T*` |
| 要存起来 | `T`（按值）+ `std::move` |
| 需要移动语义 | `T&&` |
| 转发给别人 | `T&&` + `std::forward`（万能引用）|

```cpp
// 要存起来的典型写法
class Widget {
public:
    explicit Widget(std::string name) : _name(std::move(name)) {}
private:
    std::string _name;
};
```

见 [01-03](../part1_basics/01-03_reference.md)、
[02-04](../part2_memory/02-04_perfect_forwarding.md)。

### 返回值

- 直接 `return local;`，**不要 `return std::move(local)`**（阻止 NRVO）
- 返回多个值用具名 `struct`，不要用 `tuple`（位置没有语义）
- 可能失败用 `std::optional`，不要用魔术值或出参
- 不要返回局部变量的引用/指针

见 [02-03](../part2_memory/02-03_move_semantics.md)、
[07-04](../part7_stdlib/07-04_tuple_variant.md)。

### 错误处理

| 情况 | 方式 |
|---|---|
| 真正的异常情况 | 抛异常（继承 `std::exception`）|
| 可预期的失败（查找不到）| `std::optional` |
| 可预期的失败 + 错误原因 | `error_code` 出参，或 C++23 的 `expected` |
| 程序 bug（不该发生）| `assert` |
| 编译期可检查的约束 | `static_assert` |

- 异常按 `const T&` 捕获
- **析构函数绝不能抛异常**
- 返回错误码的函数标 `[[nodiscard]]`
- 不抛异常的函数标 `noexcept`（尤其移动函数）

见 [01-08](../part1_basics/01-08_exception.md)。

---

## 五、现代语法

### 该用的

```cpp
auto it = v.begin();                    // 类型冗长时
for (const auto& x : container) { }      // 遍历
auto [k, v] = *map_it;                   // 结构化绑定（C++17）
if (auto it = m.find(k); it != m.end())  // if 初始化语句（C++17）
using Vec = std::vector<int>;            // 不要用 typedef
enum class Color { Red };                 // 不要用裸 enum
nullptr                                   // 不要用 NULL 或 0
constexpr int kMax = 100;                 // 不要用宏常量
inline constexpr double kPi = 3.14;       // 头文件里的常量（C++17）
static inline int _count = 0;             // 静态成员（C++17）
```

### 不该用的

| 不要 | 原因 | 替代 |
|---|---|---|
| `using namespace std;` 在头文件或全局 | 污染 + 升级标准可能冲突 | 函数内局部 `using std::xxx;` |
| C 风格转换 `(T)x` | 一种语法四种语义，无法 grep | 四种 `_cast` |
| `std::bind` | 可读性差、难内联、错误信息糟 | lambda |
| `std::endl` 在循环里 | 每次 flush（实测慢 15 倍）| `"\n"` |
| `rand()` | 质量差、取模偏差、非线程安全 | `<random>` 的引擎 + 分布 |
| `std::auto_ptr` | 已移除 | `unique_ptr` |
| `std::vector<bool>` | 不是真容器，返回代理对象 | `vector<char>` / `deque<bool>` |
| `typedef` | 不能模板化、方向不自然 | `using` |
| 虚函数 + 默认参数 | 静态绑定 vs 动态绑定 | NVI 惯用法 |
| `[=]` 捕获 this | 捕获的是指针，会悬垂 | `[*this]`（C++17）或捕获具体成员 |
| 函数模板特化 | 不参与常规重载决议 | 重载 |
| 手写无锁数据结构 | ABA + 内存回收极难正确 | 用锁或成熟库 |

### auto 的使用边界

**该用**：迭代器、lambda、模板中间结果、范围 for。

**不该用**：

- 类型是接口契约的一部分（公开函数返回类型、结构体成员）
- 可能遇到代理对象（`vector<bool>`、表达式模板）
- 读者需要知道具体类型才能理解代码

见 [01-10](../part1_basics/01-10_auto_decltype.md)。

### 初始化的选择

| 场合 | 写法 | 原因 |
|---|---|---|
| 容器指定大小 | `vector<int> v(n);` | `{n}` 会变成"一个值为 n 的元素" |
| 容器列举元素 | `vector<int> v{1,2,3};` | — |
| 想要窄化检查 | `int x{expr};` | `{}` 拒绝精度丢失 |
| 无参构造对象 | `Widget w;` | — |
| 避开 most vexing parse | `Widget w{};` | `Widget w();` 是函数声明 |
| 泛型代码 | `T x{};` | 对任何类型都是零/默认初始化 |

见 [01-11](../part1_basics/01-11_uniform_init.md)。

---

## 六、模板

- `class` 和 `typename` 在模板参数里等价，选一个统一用
- 依赖模板参数的嵌套类型要写 `typename`；成员模板要写 `template`
- 继承模板基类时访问基类成员要写 `this->`
- **不要特化函数模板，用重载**
- 类模板特化要放在基础模板之后、首次使用之前
- 编译期计算优先用 `constexpr` 函数，不要用模板递归
- 用 `static_assert` 把模板的隐式要求变成显式约束
- 优先 `if constexpr` 而不是 SFINAE / `enable_if`
- 用 `_t` / `_v` 后缀，自定义 trait 也提供它们
- 把与类型无关的逻辑提到非模板基类，减少代码膨胀

见 [Part 5](../part5_template/)。

---

## 七、并发

### 基本规则

- `thread` 析构前必须 `join()` 或 `detach()`，**尽量只用 join**
- 用 RAII 包装线程（C++20 用 `jthread`）
- 传引用参数要 `std::ref`；隐式转换在调用方先做
- 优先用 `std::async` 或线程池，而不是裸线程
- `std::async` **必须显式写 `std::launch::async`**
- 保存 `std::async` 的返回值（否则 future 析构阻塞，变成串行）

### 同步

- 默认用 `std::scoped_lock`（C++17）
- `mutex` 成员要是 `mutable` 的
- 条件变量**必须用带谓词的 `wait`**
- 不确定时用 `notify_all`（正确性优先）
- 多个锁用 `scoped_lock`，或固定加锁顺序
- **持有锁时不要调用用户代码**（回调、虚函数）
- 临界区尽量小；锁和它保护的数据放在一起

### 优化顺序

1. **先考虑不共享** —— `thread_local` 或每线程一份数据，最后汇总
2. 必须共享时，单个变量用 `atomic`
3. 复合操作用 `mutex`
4. 只在实测确认有收益时才用 `shared_mutex`（实测高并发下可能更慢）
5. **不要手写无锁数据结构**

实测依据：不共享 > `atomic`（快 13 倍于 mutex）> `mutex`。
见 [08-04](../part8_concurrency/08-04_atomic.md)。

### 必做的检查

```bash
-fsanitize=thread        # ThreadSanitizer，能可靠检测数据竞争
```

多线程代码应该常规性地用 TSan 跑测试。
实测它准确报出了 `++counter` 的竞争位置。

见 [Part 8](../part8_concurrency/)。

---

## 八、编译选项

### 开发时

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic \
    -Wshadow -Wconversion -Wsign-conversion \
    -Wsuggest-override -Wnon-virtual-dtor \
    -Wold-style-cast -Wcast-align -Wunused \
    -g -O0
```

关键警告的作用：

| 选项 | 作用 | 出处 |
|---|---|---|
| `-Wall -Wextra` | 基础警告集 | 全书 |
| `-Wreorder`（含在 `-Wall`）| 初始化列表顺序 | [03-02](../part3_class/03-02_constructor.md) |
| `-Wsuggest-override` | 漏写 `override` | [04-05](../part4_oop/04-05_override.md) |
| `-Wnon-virtual-dtor` | 多态类的非虚析构函数 | [04-06](../part4_oop/04-06_abstract.md) |
| `-Wdelete-non-virtual-dtor`（含在 `-Wall`）| 通过非虚析构删除 | [04-06](../part4_oop/04-06_abstract.md) |
| `-Wold-style-cast` | C 风格转换 | [01-04](../part1_basics/01-04_cast.md) |
| `-Wshadow` | 变量遮蔽 | — |
| `-Wconversion` | 隐式转换丢精度 | — |
| `-Wunused-result` | 忽略 `[[nodiscard]]` 返回值 | [01-09](../part1_basics/01-09_literals.md) |

### 测试时

```bash
# 内存错误 + 未定义行为
g++ -std=c++17 -g -fsanitize=address,undefined

# 数据竞争（不能和 address 同时用）
g++ -std=c++17 -g -fsanitize=thread

# libstdc++ 的迭代器和容器调试
g++ -std=c++17 -D_GLIBCXX_DEBUG
```

### 发布时

```bash
g++ -std=c++17 -O2 -DNDEBUG -flto
```

- `-O2` 通常比 `-O3` 更合适（`-O3` 的代码膨胀可能降低缓存命中率）
- `-flto` 开启链接时优化，能跨翻译单元内联和去虚化
- `-DNDEBUG` 关闭 `assert`

### 把警告当错误

```bash
-Werror                  # 新项目建议开
-Werror=return-type      # 或者只对关键警告开
```

---

## 九、一些设计原则

这些不是 C++ 特有的，但在 C++ 里尤其重要。

### 优先组合而非继承

继承的耦合度远高于组合 —— 派生类能访问 protected 成员，
基类的任何改动都可能影响所有派生类。

见 [04-01](../part4_oop/04-01_inheritance.md)。

### 优先值语义

C++ 的值语义（配合移动语义）通常比引用语义更简单：
没有生命周期问题、没有别名问题、线程安全更容易推理。

```cpp
std::vector<Widget> widgets;                       // 优先
std::vector<std::unique_ptr<Widget>> widgets;      // 需要多态时才用
```

### 让错误在编译期暴露

| 手段 | 把什么从运行期移到编译期 |
|---|---|
| 强类型（`enum class`、自定义单位类型）| 类型混用 |
| `explicit` | 意外的隐式转换 |
| `override` | 覆盖失败 |
| `static_assert` | 模板的类型要求 |
| `[[nodiscard]]` | 忽略返回值 |
| `constexpr` | 常量计算 |
| `std::visit` | 漏处理某个类型 |

每一条都是用编译期的严格换取运行期的安全。

### 先测量再优化

本书给出了大量实测数据，但也有几个反直觉的结果：

- 并行算法可能完全无效（缺 TBB 时静默串行）
- `shared_mutex` 在高并发短临界区下反而更慢
- 内存序在 x86 上对 `fetch_add` 没有差别
- 模板递归在能 memoize 时比 `constexpr` 更快

**结论：不要相信"应该更快"，要看实测和汇编。**

---

## 十、参考资料

### 书

- 《Effective Modern C++》(Scott Meyers) —— C++11/14 最实用的一本
- 《Effective C++》《Effective STL》(Scott Meyers) —— 经典，但部分条款已过时
- 《C++ Templates: The Complete Guide》(2nd ed.) —— 模板的权威参考
- 《C++ Concurrency in Action》(2nd ed., Anthony Williams) —— 并发唯一推荐
- 《The C++ Programming Language》(4th ed., Bjarne Stroustrup) —— 语言设计者的视角
- 《C++ Core Guidelines》—— 在线，持续更新

### 在线资源

- [cppreference.com](https://en.cppreference.com/) —— 最准确的参考，优先于任何教程
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/) —— Stroustrup 和 Sutter 维护
- [Compiler Explorer](https://godbolt.org/) —— 看汇编，验证优化是否发生
- [Quick C++ Benchmark](https://quick-bench.com/) —— 快速做微基准测试
- [Google C++ Style Guide 中文版](https://zh-google-styleguide.readthedocs.io/en/latest/google-cpp-styleguide/)

### 关于 Google 风格指南

参考文档里提到了它。需要说明：它是为 Google 的特定环境
（庞大的单一代码库、长期的 ABI 兼容需求）设计的，
有些条款（如早期禁用异常、限制模板）**不适合一般项目**。

更通用的选择是 **C++ Core Guidelines**。

> 下一章：[09-04 demo 索引](09-04_demo_index.md)
