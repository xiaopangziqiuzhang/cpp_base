# 07-04 tuple 与和类型

> 标准：`pair` C++98、`tuple` `[C++11]`；
> `optional`/`variant`/`any`/`apply` `[C++17]`
> demo：[`07_04_tuple_variant.cpp`](../../demos/part7_stdlib/07_04_tuple_variant.cpp)

本章讲四个"组合类型"。用类型论的术语说：

- **`tuple<A,B,C>`** 是**积类型**："同时有 A 和 B 和 C"
- **`variant<A,B,C>`** 是**和类型**："是 A 或 B 或 C 之一"
- **`optional<T>`** 是 `variant<nothing, T>` 的特例："可能有一个 T"
- **`any`** 是无约束的类型擦除："是任意类型"

C++17 一次补齐了后三个，这让很多以前需要绕弯的设计变得直接。

## 一、pair 与 tuple

```cpp
std::pair<int, std::string> p{1, "one"};
p.first; p.second;

std::tuple<int, std::string, double> t{1, "alice", 95.5};
std::get<0>(t);                  // 按位置取
std::get<double>(t);             // 按类型取（要求该类型在 tuple 中唯一）
std::tuple_size_v<decltype(t)>;  // 3
```

实测全部正常。

C++17 的 CTAD 让类型可以省略（见 [05-02](../part5_template/05-02_class_template.md)）：

```cpp
std::tuple t2{1, 2.5, "three"};      // 不用写 <int, double, const char*>
std::pair p2{1, 2.5};
```

### 接住多返回值的三种写法

```cpp
std::tuple<int, std::string, double> get_record();

// 1. get<N>：啰嗦，位置没有语义
auto r = get_record();
std::get<1>(r);

// 2. std::tie（C++11）：变量必须先声明，不能用 auto
int id; std::string name; double score;
std::tie(id, name, score) = get_record();

// 3. 结构化绑定（C++17）：最清楚
auto [id, name, score] = get_record();
```

实测三种都能拿到 `alice`，但第三种明显最好。
见 [01-12](../part1_basics/01-12_range_for.md)。

`std::tie` 还有个用途 —— 用 `std::ignore` 跳过不关心的部分：

```cpp
std::string only_name;
std::tie(std::ignore, only_name, std::ignore) = get_record();
```

以及实现比较运算符（见 [04-07](../part4_oop/04-07_crtp.md) 的 CRTP 例子）：

```cpp
bool operator<(const Record& rhs) const {
    return std::tie(id, name, score) < std::tie(rhs.id, rhs.name, rhs.score);
}
```

这是最简洁的"按字典序比较多个成员"的写法。

### tuple 该用在哪

**适合**：

- 泛型代码里存储参数包（`std::tuple<Args...>`）
- 临时的多返回值（但更推荐返回具名 struct）

**不适合**：

- **公开接口的返回值** —— `std::get<1>(r)` 没有语义，
  用一个具名 `struct` 更好：

```cpp
// 不好
std::tuple<int, std::string, double> get_record();

// 更好
struct Record { int id; std::string name; double score; };
Record get_record();

auto [id, name, score] = get_record();     // 结构化绑定一样好用
```

C++17 的结构化绑定支持聚合类型，所以用 `struct`
不损失任何便利，还多了字段名这个文档。

## 二、std::apply `[C++17]`

把 tuple 展开成函数实参：

```cpp
int sum3(int a, int b, int c);
auto args = std::make_tuple(1, 2, 3);
std::apply(sum3, args);              // 6
```

实测正确。C++14 下要做这件事得手写 `index_sequence` 展开
（见 [06-04](../part6_modern_core/06-04_cpp14.md)）。

常用于遍历 tuple：

```cpp
auto print_all = [](const auto&... args) {
    ((std::cout << args << " "), ...);     // 折叠表达式
};
std::apply(print_all, std::make_tuple(1, "two", 3.0));
```

实测输出 `1 two 3`。这是"遍历异构容器"的标准写法。

## 三、std::optional `[C++17]`

### 解决什么问题

"函数可能没有返回值"这件事，C++17 之前有三种表达方式，都有缺陷：

```cpp
// 做法一：出参 + bool 返回值
bool find(const std::vector<int>& v, int target, int& out);
// 问题：接口丑；out 在失败时状态不明；不能用于不可默认构造的类型

// 做法二：魔术值
int find(const std::vector<int>& v, int target);
// return -1;  表示没找到？如果 -1 是合法值怎么办

// 做法三：返回指针
int* find(std::vector<int>& v, int target);
// 问题：调用方可能忘记判空；所有权不清；不能返回局部对象
```

`std::optional<T>` 把"可能没有值"编码进**类型**：

```cpp
std::optional<int> find(const std::vector<int>& v, int target) {
    for (int x : v) if (x == target) return x;
    return std::nullopt;
}

if (auto r = find(v, 2)) {        // 转 bool
    use(*r);
}
```

### 常用接口

```cpp
opt.has_value();         // 或直接 if (opt)
*opt;                     // 解引用，空时是【未定义行为】
opt.value();              // 空时抛 std::bad_optional_access
opt.value_or(default);    // 空时返回默认值
opt.reset();              // 置空
opt.emplace(args...);     // 原地构造
```

实测：

```
        has_value() = 1
        *opt = 3, opt.value() = 3
        value_or(默认值): -1
        空 optional 调 value() 抛: bad optional access
        （*empty 是未定义行为，不抛异常）
```

**`*opt` 和 `opt.value()` 的区别要记住**：
前者快但空时是 UB，后者安全但有检查。
这和 `vector` 的 `[]` vs `at()` 是同一个设计模式。

### 典型用途

```cpp
// 1. 可能失败的解析
std::optional<int> parse_int(const std::string& s) {
    try { return std::stoi(s); } catch (...) { return std::nullopt; }
}
parse_int("42").value_or(-1);      // 42
parse_int("abc").value_or(-1);     // -1

// 2. 可选的配置项
struct Config {
    std::string host;
    std::optional<int> port;        // 可以不设置，区别于"设置为 0"
};

// 3. 延迟初始化（替代指针）
std::optional<Connection> conn;     // 还没连接
conn.emplace(host, port);            // 这时才构造

// 4. 返回可能不存在的查找结果
std::optional<User> find_user(int id);
```

第 2 条值得强调：`optional<int>` 能区分"没设置"和"设置为 0"，
而用魔术值或默认值就区分不了。

### 开销

```
      sizeof(int) = 4, sizeof(optional<int>) = 8（多一个 bool + 对齐）
```

`optional<T>` 是 `sizeof(T)` + 一个 bool + 对齐填充。
**没有堆分配** —— 对象内联存储在 optional 里面。

## 四、std::variant `[C++17]`

### 解决什么问题

C 的 `union` 不安全：

```cpp
union Unsafe {
    int i;
    double d;
    // std::string s;      // 不能放非平凡类型！
};
```

两个问题：**不知道当前存的是哪个成员**（要自己维护一个标签），
**不能放有构造/析构函数的类型**。

手工维护标签的写法叫"tagged union"，极易出错：

```cpp
struct TaggedUnion {
    enum { INT, DOUBLE } tag;
    union { int i; double d; };
    // 忘了同步 tag 就是未定义行为
};
```

`std::variant` 是类型安全的 union：

```cpp
std::variant<int, double, std::string> v = 42;
v.index();                       // 0
std::get<int>(v);                 // 42

v = 3.14;                         // index() 变成 1
v = std::string("hello");         // index() 变成 2，自动析构前一个值
```

实测索引变化正确，而且能存 `std::string`。

### 访问方式

```cpp
std::get<int>(v);                     // 类型错误时抛 std::bad_variant_access
std::get<0>(v);                        // 按索引
std::get_if<int>(&v);                  // 失败返回 nullptr，不抛异常
std::holds_alternative<int>(v);        // 检查当前是否是该类型
```

实测：

```
      取错类型抛: std::get: wrong index for variant
      get_if<string> 成功: hello
      get_if<int> 返回 nullptr（安全）
      holds_alternative<string> = 1
```

### std::visit：处理所有可能的类型

```cpp
auto printer = [](const auto& value) { std::cout << value; };
std::visit(printer, v);
```

或者用访问者类做类型分发：

```cpp
struct Visitor {
    void operator()(int x) const { /* 整型处理 */ }
    void operator()(double x) const { /* 浮点处理 */ }
    void operator()(const std::string& x) const { /* 字符串处理 */ }
};
std::visit(Visitor{}, v);
```

实测两种方式都正确分发。

**`visit` 的关键价值：它要求处理所有可能的类型** ——
漏了一个就是编译错误。这比虚函数（漏实现一个会在运行期才发现）
和 switch（漏一个 case 只有警告）都更安全。

常见的 `overloaded` 惯用法（用 lambda 代替访问者类）：

```cpp
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;    // C++17 推导指引

std::visit(overloaded{
    [](int x) { /* ... */ },
    [](double x) { /* ... */ },
    [](const std::string& x) { /* ... */ }
}, v);
```

这里同时用了可变参数继承、`using` 引入基类 `operator()`
（见 [04-01](../part4_oop/04-01_inheritance.md)）、和推导指引 ——
是 C++17 特性组合的一个精彩例子。

### 开销

```
      sizeof(variant<int,double,string>) = 40（最大成员 + 类型标记）
```

`sizeof(std::string)` 是 32，加上类型标记和对齐就是 40。
**没有堆分配**，所有可能的类型都内联存储。

代价：**大小是最大成员的大小**。所以别把大小差异悬殊的类型放一起
（`variant<char, std::array<int,1000>>` 会浪费大量内存）。

## 五、variant 作为虚函数的替代

```cpp
struct Circle { double r; double area() const; };
struct Square { double s; double area() const; };
struct Triangle { double b, h; double area() const; };

using Shape = std::variant<Circle, Square, Triangle>;

std::vector<Shape> shapes{Circle{1.0}, Square{2.0}, Triangle{3.0, 4.0}};

double total = 0;
for (const auto& s : shapes) {
    total += std::visit([](const auto& shape) { return shape.area(); }, s);
}
```

实测总面积 13.1416。

**相比虚函数的优点：**

1. **值语义** —— 可以直接放 `vector`，不需要 `unique_ptr`
   （对比 [04-02](../part4_oop/04-02_derived_lifecycle.md) 的对象切片问题）
2. **无虚表指针、无间接调用** —— 可以内联
   （对比 [04-07](../part4_oop/04-07_crtp.md) 的虚调用开销）
3. **`visit` 要求处理所有类型** —— 新增类型时漏处理是编译错误
4. **不需要公共基类** —— 这三个类之间没有任何关系

**缺点：**

1. **类型集合必须封闭** —— 编译期就要知道全部类型，
   所以不能做插件、不能让用户扩展
2. **大小是最大成员的大小**
3. **不能跨 ABI 边界扩展**

### 选择原则

| 情况 | 选择 |
|---|---|
| 类型集合**封闭**且已知 | **`variant`** |
| 类型集合**开放**（插件、用户扩展）| **虚函数** |
| 需要值语义、放进容器 | `variant` |
| 跨动态库边界 | 虚函数 |
| 性能关键（要内联）| `variant` |

典型的 `variant` 适用场景：解析器的 token 类型、
状态机的状态、JSON 的值类型、错误处理的 `Result<T, E>`。

## 六、std::any `[C++17]`

存储任意类型：

```cpp
std::any a = 42;
std::any_cast<int>(a);              // 42

a = std::string("hello");
std::any_cast<std::string>(a);       // "hello"

a.has_value();
a.type().name();                      // typeid 信息
std::any_cast<int>(a);                // 类型错误时抛 std::bad_any_cast
std::any_cast<double>(&a);            // 指针版本，失败返回 nullptr
a.reset();
```

实测全部正常：

```
      has_value() = 1, type().name() = d
      取错类型抛: bad any_cast
      any_cast<double>(&a) 成功: 3.14
      sizeof(any) = 16
```

### any vs variant

| | `variant<A,B,C>` | `any` |
|---|---|---|
| 类型集合 | **封闭**，编译期已知 | **任意** |
| 类型检查 | 编译期（`visit` 保证完备）| 运行期 |
| 堆分配 | 无 | 大对象要分配 |
| 大小 | 最大成员的大小 | 固定（实测 16），大对象存堆上 |
| 遍历所有类型 | `std::visit` | 做不到（不知道有哪些类型）|

**实践：优先 `variant`。** `any` 只在真的需要"任意类型"时用 ——
比如实现一个通用的属性字典、插件系统的参数传递。

`any` 的根本问题是：**你必须记住里面存的是什么类型**。
这把编译期的类型检查退化成了运行期的约定，容易出错。

## 七、四者的选择

```
需要组合多个值？
├── "同时有 A 和 B 和 C" -> tuple（或更好：具名 struct）
├── "可能有一个 T"       -> optional<T>
├── "是 A 或 B 或 C 之一"
│   ├── 类型集合封闭      -> variant<A,B,C>
│   └── 类型集合开放      -> 虚函数 / unique_ptr<Base>
└── "是任意类型"          -> any（慎用）
```

实测总结输出：

```
      optional<T>  —— "可能有一个 T"（0 或 1 个值）
        用途: 可能失败的查找/解析、可选的配置项、延迟初始化
      variant<A,B,C> —— "是 A 或 B 或 C 之一"（和类型）
        用途: 状态机、表达式树、解析结果、替代虚函数
      any —— "是任意类型"
        用途: 类型擦除的容器、插件系统的参数传递
      tuple<A,B,C> —— "同时有 A 和 B 和 C"（积类型）
        用途: 多返回值、泛型代码里的参数包存储
```

## 八、小结

| 类型 | 语义 | 开销 | 要点 |
|---|---|---|---|
| `pair` / `tuple` | 积类型 | 成员之和 | 公开接口优先用具名 struct |
| `std::tie` | | | 跳过用 `std::ignore`；实现比较运算符很方便 |
| `std::apply` `[C++17]` | | | 把 tuple 展开成函数实参 |
| `optional<T>` `[C++17]` | 0 或 1 个 T | `sizeof(T)` + bool | `*opt` 是 UB，`value()` 抛异常；能区分"没有"和"默认值" |
| `variant<...>` `[C++17]` | 和类型 | 最大成员 | `visit` 要求处理所有类型；可替代封闭集合的虚函数 |
| `any` `[C++17]` | 任意类型 | 固定 + 可能堆分配 | 运行期检查；慎用，优先 `variant` |
| 结构化绑定 | | | 配合以上全部类型，是 C++17 的最佳搭配 |

> 下一章：[07-05 chrono 与 random](07-05_chrono_random.md)
