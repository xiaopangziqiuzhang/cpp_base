# 01-02 const 与 constexpr

> 标准：`const` C++98、`constexpr` `[C++11]`、放宽的 constexpr `[C++14]`、
> `if constexpr` 与 `inline` 变量 `[C++17]`
> demo：[`01_02_const.cpp`](../../demos/part1_basics/01_02_const.cpp)、
> [`01_02_constexpr.cpp`](../../demos/part1_basics/01_02_constexpr.cpp)

## 一、const 解决了什么问题

C 里表达"这个值不该变"只有宏：

```c
#define NUMBER 1024
```

宏在预处理阶段做字符串替换，编译器根本看不到 `NUMBER` 这个名字。
由此带来三个问题：没有类型、不做类型检查、出错信息里只剩替换后的内容。

`const` 把"不可变"变成了类型系统的一部分。

### const 常量与宏定义的区别（常考题）

| | 宏定义 | const 常量 |
|---|---|---|
| 处理阶段 | 预处理阶段展开，做字符串替换 | 编译阶段 |
| 类型 | 没有类型 | 有具体类型 |
| 类型检查 | 不做任何检查 | 编译期执行类型检查 |
| 调试 | 符号表里没有，无法调试 | 可以调试 |
| 作用域 | 从 `#define` 到文件尾或 `#undef`，无视作用域 | 遵守作用域规则 |
| 能否是类成员 | 不能 | 可以 |

宏最危险的是**参数多次求值**。经典例子：

```cpp
#define MACRO_MAX(a, b) ((a) > (b) ? (a) : (b))

int i = 5, j = 3;
int r1 = MACRO_MAX(i++, j);
```

展开成 `((i++) > (j) ? (i++) : (j))`，`i++` 出现了两次。实测输出：

```
MACRO_MAX(i++, j) -> r1=6, i 变成了 7（期望 6，实际被加了两次）
std::max(i++, j)  -> r2=5, i = 6
```

注意外层括号已经加全了 —— 括号只能解决优先级问题，解决不了多次求值。
函数（含 `inline` 函数）的参数只求值一次，所以结论是：
**尽量用 const 替换宏常量，用 inline 函数替换宏函数**（详见 [01-07](01-07_inline.md)）。

## 二、const 修饰变量

```cpp
const int number1 = 10;   // const 在前
int const number2 = 20;   // const 在后，完全等价
// const int val;         // 编译错误：常量必须初始化
```

两种写法等价。有人提倡 `int const` 的"从右往左读"风格，因为它在指针场合更一致；
实践中 `const int` 更常见。本书统一用 `const int`，但在讲指针时会用从右往左的读法。

## 三、const 修饰指针：看 const 在 `*` 的哪一侧

这是 const 最容易混淆的用法。只需记住一条规则：
**`const` 修饰的是它左边最近的那个东西；如果左边没有东西，就修饰右边的。**

```cpp
int number1 = 10, number2 = 20;

const int* p1 = &number1;        // 常量指针：所指内容不可改
// *p1 = 100;                    // 编译错误
p1 = &number2;                   // ok：指向可改

int const* p2 = &number1;        // 与 p1 完全等价

int* const p3 = &number1;        // 指针常量：指向不可改
*p3 = 100;                       // ok：所指内容可改
// p3 = &number2;                // 编译错误

const int* const p4 = &number1;  // 两者都不可改
```

`p1` / `p2` 的 `const` 在 `*` 左边，管的是 `int`（所指内容）；
`p3` 的 `const` 在 `*` 右边，管的是指针本身。

命名上容易乱，用"从右往左读"最可靠：

- `int const *  p` → p 是指针，指向 const int → **所指内容不可改**
- `int * const  p` → p 是 const 指针，指向 int → **指针本身不可改**

## 四、const 修饰成员函数与对象

把 `const` 放在参数表和函数体之间，得到 **const 成员函数**：

```cpp
class Computer {
public:
    void print() const {
        std::cout << _brand << " " << _price << "\n";
        // _price = 0;     // 编译错误：不能修改成员
        // setPrice(0);    // 编译错误：不能调用非 const 成员函数
    }
    void setPrice(double price) { _price = price; }
private:
    std::string _brand;
    double _price;
};
```

const 成员函数的两个约束：

1. 只能读取数据成员，不能修改
2. 只能调用其它 const 成员函数

**它的本质是改变了 `this` 指针的类型**：
普通成员函数的隐含参数是 `Computer* const this`，
const 成员函数的是 `const Computer* const this`。
所以"不能修改成员"并不是什么特殊规则，就是 const 指针的常规行为。
这一点在 [03-06](../part3_class/03-06_const_member.md) 会展开。

**const 对象只能调用 const 成员函数**：

```cpp
const Computer cpc("ThinkPad X1", 9999);
cpc.print();          // ok
// cpc.setPrice(1);   // 编译错误：passing 'const Computer' as 'this'
```

因为 const 对象只允许被创建、只读访问、撤销，改写是不允许的。
这反过来说明：**凡是不修改对象状态的成员函数都应该声明为 const**，
否则你的类对 const 对象就是不可用的。

### const 可以参与重载

```cpp
const std::string& brand() const { return _brand; }  // const 对象调用
std::string&       brand()       { return _brand; }  // 非 const 对象调用
```

实测输出：

```
非 const 对象调用 brand(): [非const版本] Huawei MateBook14
const 对象调用 brand():   [const 版本] ThinkPad X1
```

标准库大量使用这个模式：`begin()`/`end()`、`operator[]`、`at()`、`front()`
都有 const 和非 const 两个版本，前者返回 const 引用，后者返回可写引用。

## 五、mutable：const 成员函数里的例外

有时一个函数逻辑上不改变对象状态，但需要改某个辅助成员（计数、缓存、互斥量）。
`mutable` 成员可以在 const 成员函数里被修改：

```cpp
class Cache {
public:
    int get() const {
        ++_access_count;    // mutable 成员，const 函数内也能改
        return _value;
    }
private:
    int _value = 42;
    mutable int _access_count = 0;
};

const Cache c;
c.get(); c.get(); c.get();   // const 对象被访问了 3 次
```

`mutable` 表达的是"**逻辑上的常量性**"：对外看对象没变，内部的实现细节变了。
最典型的正当用途是在 const 成员函数里加锁：

```cpp
mutable std::mutex _mtx;     // 否则 const 成员函数无法 lock
```

滥用 `mutable` 会破坏线程安全的直觉 —— 别人以为 const 意味着可以并发读，
结果你在里面改了状态。用的时候要么保证线程安全，要么在文档里写明。

## 六、const 引用可以延长临时对象的寿命

```cpp
std::string make_string() { return "temporary"; }

const std::string& ref = make_string();   // 临时对象的寿命延长到 ref 的作用域
std::cout << ref;                         // 仍然有效

// std::string& bad = make_string();      // 编译错误：非 const 左值引用不能绑右值
```

这条规则是"按 const 引用传参"这个惯用法的基础 ——
`void f(const std::string& s)` 既能接收左值、也能接收临时量，都不产生拷贝。
详见 [01-03 引用](01-03_reference.md)。

## 七、constexpr 解决了什么问题 `[C++11]`

`const` 只保证"初始化后不再改变"，**不保证值在编译期已知**：

```cpp
int runtime_value() { return 42; }

const int a = runtime_value();   // ok：运行期才知道 a 的值
// int arr[a]{};                 // 编译错误：a 不是常量表达式
```

但很多场合要求编译期常量：数组大小、模板非类型实参、`static_assert`、
枚举值、位域宽度。C++98 下，这些地方只能用字面量、枚举，
或者碰巧是编译期常量的 `const int` —— 而你**无法表达"我要求这是编译期常量"**，
写错了只能从一长串模板错误里反推。

`constexpr` 补上的就是这个声明能力：

```cpp
constexpr int d = 10;                  // 要求编译期可求值，否则编译报错
// constexpr int b = runtime_value();  // 编译错误，意图不满足时立刻失败
```

两者的分工：

| | 保证 | 用途 |
|---|---|---|
| `const` | 初始化后不可修改 | 接口约束、只读访问 |
| `constexpr` | 编译期可求值（同时隐含只读） | 需要常量表达式的场合 |

实测对比：

```
a=42 c=10 d=10 sizeof(arr1)=40 sizeof(arr2)=40
```

## 八、constexpr 函数：C++11 → C++14 的放宽

C++11 的 constexpr 函数限制极严：函数体基本上只能有**一条 return 语句**，
不能有局部变量、不能有循环、不能有 if 语句。想写递归只能用三目运算符：

```cpp
// C++11：只能这么写
constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}
```

C++14 放宽了限制，允许局部变量、循环、分支，于是可以写得像普通函数：

```cpp
// C++14：正常写法
constexpr int factorial(int n) {
    int result = 1;
    for (int i = 2; i <= n; ++i) {   // C++11 下这是编译错误
        result *= i;
    }
    return result;
}

constexpr int fib(int n) {
    if (n <= 1) return n;            // C++14 起可以用 if 语句
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) { int t = a + b; a = b; b = t; }
    return b;
}
```

这个放宽的实际意义很大：C++11 的限制逼着人把算法改写成递归形式，
既难读又容易栈溢出（编译期递归深度有限）。C++14 之后，
"编译期计算"从模板元编程的黑魔法变成了可以用普通代码写的东西。

### constexpr 函数的双重身份

`constexpr` 函数**既能在编译期求值，也能在运行期调用**：

```cpp
constexpr int f10 = factorial(10);   // 编译期：static_assert 可验证
static_assert(f10 == 3628800, "");

int n = 6;
std::cout << factorial(n);           // 运行期：n 不是常量
```

实测输出：

```
5! = 120, 10! = 3628800, fib(20) = 6765
运行期调用 factorial(6) = 720
```

所以 `constexpr` 不是"强制编译期执行"，而是"**在可能时允许编译期执行**"。
想强制，C++20 提供了 `consteval`。

## 九、constexpr 构造函数与字面类型

类也能参与编译期计算，只要构造函数和成员函数都是 `constexpr`：

```cpp
class Point {
public:
    constexpr Point(int x, int y) : _x(x), _y(y) {}
    constexpr int x() const { return _x; }
    constexpr int y() const { return _y; }
    constexpr void move(int dx, int dy) { _x += dx; _y += dy; }  // C++14 起可修改成员
private:
    int _x, _y;
};

constexpr Point translate(Point p, int dx, int dy) { p.move(dx, dy); return p; }

constexpr Point origin(0, 0);
constexpr Point moved = translate(origin, 3, 4);
static_assert(moved.x() == 3 && moved.y() == 4, "编译期完成了对象构造和计算");
```

整个对象的构造、成员修改、函数调用全部发生在编译期，运行时只剩两个立即数。
这类类型称为**字面类型（literal type）**。

## 十、if constexpr：编译期分支 `[C++17]`

普通 `if` 的两个分支都必须能编译通过，即使其中一个永远不会执行。
这在模板里是真问题：

```cpp
template <class T>
auto describe(T value) {
    if (std::is_integral<T>::value) {
        return value * 2;              // T 是 const char* 时，这行也要能编译
    } else {
        return std::string_view("unknown");
    }
    // 还有一个问题：两个 return 的类型不同，auto 推导冲突
}
```

`if constexpr` 让**不成立的分支根本不被实例化**：

```cpp
template <class T>
auto describe(T value) {
    if constexpr (std::is_integral_v<T>) {
        return value * 2;
    } else if constexpr (std::is_floating_point_v<T>) {
        return value / 2.0;
    } else {
        return std::string_view("unknown type");
    }
}
```

实测输出：

```
describe(21)    = 42
describe(3.0)   = 1.5
describe("hi") = unknown type
```

这取代了 C++11/14 里用 SFINAE、`enable_if`、标签分发写的大量样板代码，
是 C++17 在模板编程上最实用的改进。详见
[05-05 编译期计算与 type_traits](../part5_template/05-05_type_traits.md)。

## 十一、inline 变量：头文件里定义常量 `[C++17]`

C++17 之前，在头文件里定义一个供全局共享的常量很别扭：

```cpp
// C++17 之前
// header.h
extern const double kPi;      // 只能声明
// source.cpp
const double kPi = 3.14159;   // 必须在某个 .cpp 里定义
```

如果直接在头文件里写 `const double kPi = 3.14;`，因为 const 对象默认内部链接，
每个包含它的翻译单元都会有一份独立副本 —— 浪费空间，而且取地址会得到不同结果。

C++17 的 `inline` 变量解决了这个问题：

```cpp
// header.h，仅此一处
inline constexpr double kPi = 3.14159265358979323846;
```

多个翻译单元包含它也只有一个实体。这对 header-only 库意义很大，
顺带也解决了静态成员的类内初始化问题（见
[03-05 特殊成员](../part3_class/03-05_special_members.md)）。

## 十二、小结

| 用法 | 含义 |
|---|---|
| `const int x = 1;` | 只读变量，必须初始化 |
| `const int* p` / `int const* p` | 所指内容不可改 |
| `int* const p` | 指针指向不可改 |
| `const int* const p` | 两者都不可改 |
| `void f() const` | const 成员函数，`this` 变成 `const T* const` |
| `const T obj;` | const 对象，只能调用 const 成员函数 |
| `mutable T m;` | 允许在 const 成员函数中修改 |
| `const T& r = tmp;` | 延长临时对象寿命 |
| `constexpr` `[C++11]` | 要求编译期可求值 |
| `constexpr` 函数 `[C++14]` | 可含局部变量、循环、分支 |
| `if constexpr` `[C++17]` | 不成立的分支不实例化 |
| `inline` 变量 `[C++17]` | 头文件里定义唯一实体 |

实践建议：

- 优先 `constexpr` 而非 `const`，优先 `const` 而非宏
- 所有不修改对象状态的成员函数都加 `const`
- 按 `const T&` 传递大对象参数
- 头文件里的全局常量用 `inline constexpr` `[C++17]`

> 下一章：[01-03 引用](01-03_reference.md)
