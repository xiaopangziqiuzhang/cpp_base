# 01-11 统一初始化

> 标准：列表初始化、`initializer_list`、类内成员初始值 `[C++11]`
> demo：[`01_11_uniform_init.cpp`](../../demos/part1_basics/01_11_uniform_init.cpp)

## 一、解决了什么问题

C++98 初始化一个对象有多少种写法？数一下：

```cpp
int x1 = 1;                 // 拷贝初始化
int x2(2);                  // 直接初始化
int arr[] = {1, 2, 3};      // 聚合初始化（只能用于数组和聚合类型）
Aggregate ag = {1, 2.5};    // 聚合初始化
std::vector<int> v(3, 7);   // 构造函数
int* p = new int(5);        // new 表达式
std::string s();            // 陷阱！这是函数声明，不是对象
```

问题不只是"写法多"，而是**哪种写法能用取决于类型**：

- `= {...}` 只能用于数组和聚合类型，不能用于有构造函数的类
- `(...)` 不能用于数组
- 容器没有办法用一组字面值初始化 —— 这是最痛的一点：

```cpp
// C++98：想要一个装着 1,2,3 的 vector
std::vector<int> v;
v.push_back(1);
v.push_back(2);
v.push_back(3);
// 或者先建数组再拷贝
int tmp[] = {1, 2, 3};
std::vector<int> v2(tmp, tmp + 3);
```

而内置数组可以 `int arr[] = {1,2,3};`。
标准库容器的用法居然不如内置数组方便，这是个明显的设计缺口。

C++11 的答案是**列表初始化**：用 `{}` 作为唯一的初始化语法，覆盖所有场合。

## 二、一种语法，所有场合

```cpp
int x{1};                                           // 基本类型
int arr[]{1, 2, 3};                                 // 数组
Aggregate ag{1, 2.5};                               // 聚合类型
Point pt{3, 4};                                     // 有构造函数的类
std::vector<int> v{1, 2, 3};                        // 标准容器
std::map<std::string, int> m{{"a", 1}, {"b", 2}};   // 嵌套
int* p = new int{5};                                // new 表达式
```

实测输出：

```
  x=1 arr[2]=3 ag.b=2.5 pt=(3,4) v.size()=3 m["a"]=1 *p=5
```

这就是"统一初始化（uniform initialization）"这个名字的来由。
对容器的改善尤其明显 —— 终于和内置数组一样方便了。

## 三、`{}` 的第一个附加价值：拒绝窄化转换

`=` 和 `()` 初始化允许丢精度的隐式转换，静默发生：

```cpp
double d = 3.99;

int a = d;        // 允许，静默截断为 3
int b(d);         // 允许，静默截断为 3
// int c{d};      // 编译错误：narrowing conversion of '3.99' from 'double' to 'int'
```

实测输出：

```
  int a = 3.99  -> 3（静默截断）
  int b(3.99)   -> 3（静默截断）
  int c{3.99}   -> 编译错误，{} 拒绝窄化转换
```

`{}` 禁止**窄化转换（narrowing conversion）**：浮点转整型、
大整型转小整型、以及任何可能丢失值的转换。

但如果是**常量表达式且值能被精确表示**，`{}` 会放行：

```cpp
int ok{3};          // ok
char ch{65};        // ok，65 在 char 范围内
// char bad{300};   // 编译错误：300 超出 char 范围
```

这个检查很有价值。`int x = some_double;` 这种静默截断在数值代码里是常见 bug 来源，
用 `{}` 就能让编译器帮你发现。

## 四、`{}` 的第二个附加价值：避开"最烦人的解析"

C++ 有条继承自 C 的规则：**任何能被解析成声明的东西，就是声明**。
于是这行代码不是你想的那样：

```cpp
Widget w();     // 这不是定义对象！
                // 被解析成"声明一个无参、返回 Widget 的函数 w"
w.use();        // 编译错误: request for member 'use' in 'w',
                // which is of non-class type 'Widget()'
```

这叫 **most vexing parse（最烦人的解析）**。更经典的版本是：

```cpp
// 本意：用迭代器区间构造 vector
std::vector<int> v(std::istream_iterator<int>(std::cin),
                   std::istream_iterator<int>());
// 实际：声明了一个函数 v，返回 vector<int>，接受两个参数
```

这个错误的报错信息通常在**使用处**而不是声明处，极难定位。

`{}` 没有这个歧义，因为函数声明的参数列表不能用花括号：

```cpp
Widget w1;      // 正确（无参构造写空就行）
Widget w2{};    // 正确，且绝不会被解析成函数声明
```

## 五、initializer_list：`{}` 背后的类型

`{1, 2, 3}` 的类型是 `std::initializer_list<int>`。
类可以通过接受这个类型的构造函数支持列表初始化：

```cpp
class Collection {
public:
    Collection(std::initializer_list<int> list) : _data(list) { /* ... */ }
    Collection(int count, int value) : _data(count, value) { /* ... */ }
private:
    std::vector<int> _data;
};

Collection c1{1, 2, 3};    // 调 initializer_list 版本
Collection c2(3, 7);       // 调 (int,int) 版本
```

普通函数也可以接受它，这是实现"任意个同类型参数"的轻量办法：

```cpp
int sum(std::initializer_list<int> list) {
    int total = 0;
    for (int v : list) total += v;     // 支持范围 for
    return total;
}
sum({1, 2, 3, 4, 5});      // 15
```

`initializer_list` 的性质：

- 元素是 **const** 的，不能修改
- 它只是底层数组的**视图**（两个指针或指针+长度），拷贝它不拷贝元素
- 底层数组的寿命和这个 `initializer_list` 对象一样长 ——
  所以**不要把它存起来**，`initializer_list` 成员会悬垂
- 所有元素必须是同一类型（或可统一转换），要异构参数请用可变模板参数
  （见 [05-04](../part5_template/05-04_variadic.md)）

## 六、陷阱：initializer_list 构造函数的优先级极高

这是 `{}` 唯一的真正坑点，也是必须知道的一条。

**只要有 `initializer_list` 构造函数可行，它就会被优先选中**，
即使其他构造函数"看起来"更匹配：

```cpp
std::vector<int> v1(3, 7);   // 3 个 7     -> [7, 7, 7]
std::vector<int> v2{3, 7};   // 两个元素   -> [3, 7]
```

实测输出：

```
  vector<int> v1(3,7): size=3 内容: 7 7 7
  vector<int> v2{3,7}: size=2 内容: 3 7
```

同样的两个数字，`()` 和 `{}` 得到完全不同的结果。
这意味着 **`()` 和 `{}` 对容器不可互换**。

优先级有多高？只有在 `initializer_list` **完全无法匹配**时，
才会考虑其他构造函数：

```cpp
std::vector<std::string> s2{3, "x"};   // 3 不能转成 string，
                                       // initializer_list<string> 不可行，
                                       // 于是退回 (count, value) -> 3 个 "x"
```

实测确认 `s2.size()` 是 3。所以行为取决于元素类型 ——
这种"看元素类型才知道调了哪个构造函数"的特性，是 `{}` 最容易出错的地方。

实践规则：

- **容器指定大小时一定用 `()`**：`std::vector<int> v(n);`
- 列举元素时用 `{}`：`std::vector<int> v{1, 2, 3};`
- 自己设计类时，如果同时提供 `initializer_list` 和普通构造函数，
  要意识到调用方用 `{}` 永远选不到普通构造函数

## 七、类内成员初始值 `[C++11]`

`{}` 还解决了另一个老问题：多个构造函数要重复写相同的成员初始化。

```cpp
// C++98：每个构造函数都要重复一遍
class Config {
public:
    Config() : timeout(30), retries(3), name("default") {}
    Config(int t) : timeout(t), retries(3), name("default") {}       // 重复
    Config(int t, int r) : timeout(t), retries(r), name("default") {} // 又重复
private:
    int timeout;
    int retries;
    std::string name;
};
```

C++11 允许在声明成员时直接给初始值：

```cpp
class Config {
public:
    Config() = default;                  // 直接用下面的初始值
    Config(int t) : timeout(t) {}         // 只覆盖 timeout，其余用初始值
private:
    int timeout{30};
    int retries{3};
    std::string name{"default"};
    std::vector<int> tags{1, 2, 3};
};
```

实测输出：

```
  timeout=30 retries=3 name=default tags.size()=3     <- Config c1;
  timeout=60 retries=3 name=default tags.size()=3     <- Config c2(60);
```

规则：**构造函数初始化列表中显式初始化的成员，用列表里的值；
没有提到的成员，用类内初始值。**

这个特性的好处超出"少打字"：

- 新增成员时不会忘记在某个构造函数里初始化它 ——
  忘记初始化成员是未定义行为的常见来源
- 默认值集中在一处，一眼能看全
- 配合 `= default` 可以完全省掉默认构造函数

详见 [03-02 构造函数](../part3_class/03-02_constructor.md)。

## 八、表达式中的 `{}`

`{}` 可以在不写类型名的情况下构造对象：

```cpp
Point make_point() { return {7, 8}; }     // 返回值，类型从函数签名推出

void takes_point(const Point& p);
takes_point({9, 10});                     // 实参，类型从参数声明推出
```

容器操作里很常用：

```cpp
std::map<std::string, std::vector<int>> m;
m.insert({"key", {1, 2, 3}});             // 嵌套的 {}
```

## 九、`()` 和 `{}` 怎么选

| 场合 | 用法 | 原因 |
|---|---|---|
| 容器指定大小 | `vector<int> v(n);` | `{n}` 会变成"一个值为 n 的元素" |
| 容器列举元素 | `vector<int> v{1,2,3};` | 这是 `{}` 的本职 |
| 基本类型，想要窄化检查 | `int x{expr};` | 拒绝精度丢失 |
| 无参构造对象 | `Widget w;` | 最简洁，无歧义 |
| 需要避开 most vexing parse | `Widget w{};` | `()` 会被当函数声明 |
| 类成员默认值 | `int x{30};` | 类内初始值 |
| 模板里的泛型初始化 | `T x{};` | 对任何类型都是零/默认初始化 |

有一个流派主张"除了容器大小，一律用 `{}`"（Herb Sutter 的 "almost always auto"
与 `{}` 组合）。本书的建议更保守：**优先 `{}`，但清楚它的两个例外**
（容器大小、`initializer_list` 优先级）。

最后一点：`T x{};` 这种写法在模板里特别有用。
它对基本类型做零初始化，对类类型调默认构造，是泛型代码里"给个确定初值"的标准写法：

```cpp
template <class T>
T sum(const std::vector<T>& v) {
    T total{};        // int -> 0, double -> 0.0, string -> ""
    for (const auto& x : v) total += x;
    return total;
}
```

## 十、小结

| 要点 | 说明 |
|---|---|
| 解决什么 | C++98 的初始化语法多且与类型耦合；容器无法用字面值列表初始化 |
| 统一性 | `{}` 适用于基本类型、数组、聚合、类、容器、new 表达式 |
| 窄化检查 | `{}` 拒绝丢精度的隐式转换，`=` 和 `()` 不拒绝 |
| most vexing parse | `Widget w();` 是函数声明，`Widget w{};` 不是 |
| `initializer_list` | `{}` 的类型；元素是 const；只是视图，别存起来 |
| **最大陷阱** | `initializer_list` 构造函数优先级最高，`vector<int>{3,7}` ≠ `vector<int>(3,7)` |
| 类内初始值 | 成员默认值写在声明处，构造函数只写需要覆盖的 |
| 表达式中使用 | `return {a, b};`、`f({a, b})` 可省略类型名 |

> 下一章：[01-12 范围 for 与结构化绑定](01-12_range_for.md)
