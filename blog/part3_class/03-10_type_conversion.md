# 03-10 类型转换

> 标准：C++98；`explicit` 转换运算符 `[C++11]`；`explicit(bool)` `[C++20]`
> demo：[`03_10_type_conversion.cpp`](../../demos/part3_class/03_10_type_conversion.cpp)

内置类型之间有一套隐式转换规则（`int` → `double`、
派生类指针 → 基类指针）。自定义类型想参与这套规则，
要靠本章的两个机制。

## 一、由其他类型转成自定义类型：转换构造函数

**单参数构造函数就是转换构造函数** ——
它让编译器能把一个其他类型的值"变成"你的类型。

```cpp
class Meters {
public:
    Meters(double value) : _value(value) {}      // 转换构造函数
private:
    double _value;
};

Meters m1(5.0);                   // 直接构造
Meters m2 = 3.0;                  // 隐式转换（拷贝初始化）

void takes_meters(const Meters& m);
takes_meters(7.0);                 // 实参的隐式转换
```

实测三处都调用了 `Meters(double)`。

**多参数构造函数在 C++11 之后也能参与隐式转换**（通过 `{}`）：

```cpp
class Point { public: Point(int x, int y); };
void f(Point p);
f({1, 2});                         // 能编译
```

见 [01-11](../part1_basics/01-11_uniform_init.md)。

## 二、explicit 禁止隐式转换

```cpp
class Seconds {
public:
    explicit Seconds(double v) : _value(v) {}
};

Seconds s1(30.0);                  // 直接构造，ok
// Seconds s2 = 30.0;              // 编译错误
// takes_seconds(30.0);            // 编译错误
takes_seconds(Seconds(30.0));      // 必须显式
```

实测确认隐式转换被禁止。

### 为什么默认应该加 explicit

隐式转换的问题是**它在你没要求的时候发生**：

```cpp
void set_timeout(Seconds s);

set_timeout(30);          // 30 什么？秒？毫秒？
                          // 如果 Seconds 的构造函数不是 explicit，这能编译
```

实测这个例子里，`explicit` 挡住了"单位不明"的调用 ——
调用方必须写 `set_timeout(Seconds(30))`，意图就清楚了。

### 经典反例

`std::vector` 的 `explicit vector(size_type n)`：

```cpp
std::vector<int> v = 5;        // 如果没有 explicit，这会创建 5 个元素的 vector
```

而 `std::string` 的 `string(const char*)` **故意不是 explicit** ——
因为"字面量当字符串用"是设计意图：

```cpp
void f(const std::string& s);
f("literal");                   // 依赖隐式转换
```

**所以规则是"默认加 `explicit`，除非明确想要隐式转换"**，
而不是"一律加"。

详见 [03-02](03-02_constructor.md)。

## 三、由自定义类型转成其他类型：类型转换运算符

```cpp
class Fraction {
public:
    Fraction(int num, int den) : _num(num), _den(den) {}

    operator double() const {                      // 转换运算符
        return static_cast<double>(_num) / _den;
    }
    operator std::string() const {
        return std::to_string(_num) + "/" + std::to_string(_den);
    }
private:
    int _num, _den;
};

Fraction f(3, 4);
double d = f;                  // 隐式调用 operator double()，得 0.75
double sum = f + 1.0;          // 也会隐式转换，得 1.75
std::string s = f;             // 调用 operator string()，得 "3/4"
```

实测全部正确。

### 语法的三个特点

```cpp
operator 目标类型() const
```

1. **没有返回类型声明** —— 运算符名就是返回类型
2. **没有参数**
3. **通常应该是 `const` 的**（转换不该修改对象）

### 为什么这个语法这么怪

因为它要表达的是"这个类型可以当成另一个类型用"，
而不是"调用一个函数得到另一个类型的值"。
后者应该用具名函数（见第七节）。

## 四、隐式转换运算符的危险

这是本章最重要的一节。看一个有隐式 `operator bool` 的类：

```cpp
class BadBool {
public:
    explicit BadBool(int v) : _value(v) {}
    operator bool() const { return _value != 0; }     // 隐式！
private:
    int _value;
};

BadBool a(5), b(3);
a + b;        // ?
a < b;        // ?
a == b;       // ?
```

实测结果：

```
        a + b  = 2  <- 两个 bool 相加！不是 5+3
        a < b  = 0  <- 比较的是 true < true
        a == b = 1  <- 都转成 true，所以相等
```

**全部是无意义的操作，但全都能编译通过。**

原因：`bool` 可以继续**提升**为 `int`（见
[01-06](../part1_basics/01-06_bool_nullptr_enum.md)），
所以 `a` 和 `b` 变成 `true`（即 1），然后参与任何算术。

这叫**隐式转换的传染性** —— 提供一个到内置类型的隐式转换，
就等于允许了该内置类型的全部运算。

## 五、explicit operator bool：C++11 的精巧设计

```cpp
class GoodBool {
public:
    explicit operator bool() const { return _value != 0; }
};
```

加了 `explicit` 后，`a + b` 变成编译错误 —— 正是我们想要的。

**但在"需要 bool 的上下文"中仍然自动转换**：

```cpp
GoodBool a(5), b(3);

if (a) { }              // ok！自动转换
if (a && b) { }          // ok
while (a) { }            // ok
!a;                       // ok
a ? x : y;                // ok

// std::cout << a + b;   // 编译错误，正确地被挡住
int n = a;                // 编译错误
```

实测确认 `if`、`&&`、`!` 都正常工作。

这叫 **contextual conversion to bool**（上下文 bool 转换），
是 C++11 对 `explicit` 转换运算符的特殊规定：

> `explicit operator bool()` 在以下位置会被**自动调用**：
> `if` / `while` / `for` 的条件、`!` 的操作数、
> `&&` `||` 的操作数、`?:` 的条件、`static_assert` 的条件。

**这个设计让"能当条件用"和"不能参与算术"同时成立。**

### 标准库全都用这个模式

```cpp
std::unique_ptr<T> p = ...;
if (p) { }                      // explicit operator bool

std::ifstream ifs(path);
if (ifs) { }                     // 同样

std::optional<T> opt = ...;
if (opt) { }                     // 同样
```

见 [02-05](../part2_memory/02-05_smart_pointers.md)、
[07-04](../part7_stdlib/07-04_tuple_variant.md)。

C++11 之前只能用 **safe bool 惯用法**绕过去
（返回一个无法参与算术的成员指针类型），非常笨重。
`explicit operator bool` 是一个干净的解决。

## 六、二义性：两个方向的转换不能同时有

```cpp
class A {
public:
    A(const B&);               // B -> A（转换构造函数）
};

class B {
public:
    operator A() const;        // B -> A（转换运算符）
};

void takes_a(const A&);
B b;
takes_a(b);        // 编译错误！
```

实际错误：

```
error: conversion from 'B' to 'const A&' is ambiguous
  候选 1: A::A(const B&)
  候选 2: B::operator A()
```

两条路径都能完成转换，编译器无法选择。

**规则：同一对类型之间只提供一个方向的一种转换。**
要么由目标类提供构造函数，要么由源类提供转换运算符，**不要都提供**。

一般倾向于**由目标类提供构造函数**，因为：

- 目标类通常是你自己的类，源类可能是第三方的
- 构造函数更容易发现（在类定义里）
- 可以有多个（针对不同源类型），而转换运算符要写在每个源类里

## 七、隐式转换最多只走一步

```cpp
class Celsius {
public:
    explicit Celsius(double v) : _v(v) {}
};

class Fahrenheit {
public:
    Fahrenheit(const Celsius& c) : _v(c.value() * 9 / 5 + 32) {}
};

void takes_fahrenheit(const Fahrenheit& f);

Celsius c(100.0);
takes_fahrenheit(c);            // ok：一步转换 Celsius -> Fahrenheit
// takes_fahrenheit(100.0);     // 编译错误：需要两步
```

实测确认一步可以，两步不行。

**用户定义的隐式转换最多只走一步。**

- 内置转换可以和它组合：`int` → `double` → `MyClass`（内置 + 用户定义）
- 但不能连续两次用户定义的转换：`double` → `Celsius` → `Fahrenheit`

这个限制是有意的 —— 否则转换路径会呈指数级增长，
编译器的选择难以预测，而且错误信息会无法理解。

## 八、实践建议

### 清单

1. **单参数构造函数默认加 `explicit`**
2. **转换运算符默认加 `explicit`**（`operator bool` 尤其）
3. **优先提供具名函数而不是转换运算符**
4. **同一对类型只提供一个方向的一种转换**
5. 例外：确实想要"像内置类型一样用"时才允许隐式转换

### 第 3 条最值得强调

```cpp
// 不好：隐式转换运算符
class MyString {
public:
    operator const char*() const { return _data.c_str(); }
};

// 好：具名函数
class MyString {
public:
    const std::string& str() const { return _data; }
    const char* c_str() const { return _data.c_str(); }
};
```

为什么具名函数更好：

- **调用点可见** —— `s.c_str()` 一眼看出发生了转换
- **可以有多个** —— 不会二义性
- **避免意外** —— 隐式转出到指针会导致悬垂难以察觉

`std::string` 就是这么设计的：它有 `c_str()` 和 `data()`，
**没有 `operator const char*`**。理由正是第三点：

```cpp
const char* p = get_string();      // 如果有隐式转换，临时 string 销毁后 p 悬垂
```

### 正当的隐式转换场合

**场合一：强类型包装（不提供隐式转换）**

```cpp
class Bytes {
public:
    explicit constexpr Bytes(std::size_t n) : _n(n) {}
    constexpr std::size_t count() const { return _n; }
    // 故意不提供到 size_t 的隐式转换 —— 否则就失去了强类型的意义
};

constexpr Bytes operator""_KB(unsigned long long n) { return Bytes(n * 1024); }
constexpr auto buf = 4_KB;        // 4096 字节
```

实测正确。这和 [01-09](../part1_basics/01-09_literals.md)
讲的自定义字面量是同一个思路：**让单位进入类型系统**。

**场合二：和标准类型互操作（有意允许）**

```cpp
class MyString {
public:
    MyString(const char* s);          // 故意不加 explicit，像 std::string
    MyString(std::string s);
    explicit operator std::string() const;    // 但转出要显式
};
```

**转入宽松、转出严格**是个好的默认策略。

## 九、标准库的做法（值得模仿）

实测验证标准库的选择：

```
        is_convertible<const char*, string> = 1       <- 允许隐式转入
        is_convertible<string, const char*> = 0       <- 不允许隐式转出
        is_convertible<string, string_view> = 1       <- 允许
        is_convertible<string_view, string> = 0       <- explicit，所以是 0
```

四个设计决定：

| 类型 | 转入 | 转出 | 理由 |
|---|---|---|---|
| `std::string` | `const char*` 隐式 | 只有 `c_str()`/`data()` | 隐式转出会导致悬垂 |
| `std::string_view` | `string` 隐式 | `string` 是 **explicit** | 避免意外的内存分配 |
| 智能指针 | 构造函数 **explicit** | `explicit operator bool` | 防止裸指针隐式接管 |
| `std::optional` | `T` 隐式 | `explicit operator bool` | 方便构造，安全判空 |

`string_view` → `string` 是 explicit 这个决定特别值得注意：
它防止了"不小心做了一次堆分配"。见
[07-01](../part7_stdlib/07-01_string.md)。

## 十、C++20 的 explicit(bool)

顺带提一下后续演进。C++20 允许**条件 explicit**：

```cpp
template <class T>
class Wrapper {
public:
    // T 可隐式转换时，Wrapper 也可隐式转换；否则 explicit
    template <class U>
    explicit(!std::is_convertible_v<U, T>) Wrapper(U&& u);
};
```

这解决了泛型包装类的一个老问题：
`std::pair`、`std::tuple`、`std::optional` 的构造函数
应该是 explicit 还是不是，取决于元素类型。
C++17 下标准库要靠 SFINAE 写两套重载，C++20 一行搞定。

## 十一、小结

| 要点 | 说明 |
|---|---|
| 转入自定义类型 | **单参数构造函数**（多参数在 C++11 后也能通过 `{}`）|
| 转出自定义类型 | **类型转换运算符** `operator T() const` |
| 转换运算符的语法 | 无返回类型声明、无参数、通常 const |
| **隐式转换的危险** | 到内置类型的隐式转换会传染该类型的全部运算（实测 `a + b` 变成 bool 相加）|
| `explicit` 构造函数 | 禁止隐式转换；**单参数构造函数默认应加** |
| `explicit` 转换运算符 `[C++11]` | 禁止隐式转换 |
| **contextual conversion** | `explicit operator bool` 在 `if`/`while`/`!`/`&&`/`?:` 中**仍然自动生效** |
| 标准库的模式 | 所有智能指针、流、`optional` 都用 `explicit operator bool` |
| 二义性 | 同一对类型**只提供一个方向的一种转换** |
| 只走一步 | 用户定义的隐式转换不能连续两次 |
| **优先具名函数** | `c_str()` 比 `operator const char*` 好：调用点可见、无二义、避免悬垂 |
| 默认策略 | **转入宽松、转出严格** |
| C++20 | `explicit(bool)` 支持条件 explicit |

Part 3 到此结束。

> 下一章：[04-01 继承基础](../part4_oop/04-01_inheritance.md)
