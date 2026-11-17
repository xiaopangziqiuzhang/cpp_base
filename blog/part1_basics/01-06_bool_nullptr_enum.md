# 01-06 bool / nullptr / enum class

> 标准：`bool` C++98、`nullptr` `[C++11]`、`enum class` 与枚举底层类型 `[C++11]`
> demo：[`01_06_nullptr_enum.cpp`](../../demos/part1_basics/01_06_nullptr_enum.cpp)

本章讲三个小特性，它们的共同点是：**都在修补 C 语言留下的类型漏洞**。

## 一、bool

C 没有布尔类型（C99 才加了 `_Bool`），真假靠整数表示，于是 `if (x = 1)`
这种笔误能顺利编译。C++ 把 `bool` 作为基本类型加入，字面值是 `true` 和 `false`。

```cpp
bool b1 = -100;     // true  —— 任何非零值都转 true
bool b2 = 100;      // true
bool b3 = 0;        // false
int  x  = true;     // 1
int  y  = false;    // 0
```

实测输出：

```
bool(-100)=1 bool(100)=1 bool(0)=0 bool(1)=1
int(true)=1 int(false)=0
sizeof(bool) = 1
```

要点：

- 任何数字或指针值都可以**隐式转换**为 bool：非零为 `true`，零为 `false`
- `bool` 转整型时，`true` 是 1，`false` 是 0
- `sizeof(bool)` 是 1。注意不是 0 —— 即使只需要 1 位，也必须可寻址
- 默认输出是 `1`/`0`；想要 `true`/`false` 用 `std::boolalpha`：

```cpp
std::cout << std::boolalpha << b2;   // 输出 true
```

指针到 bool 的隐式转换是惯用的空检查写法：

```cpp
int* p = nullptr;
if (!p) { /* 空指针 */ }
```

### `std::vector<bool>` 不是容器

顺带提醒：`std::vector<bool>` 为了省内存按位存储，`operator[]`
返回代理对象而不是 `bool&`。它不满足容器要求，用 `auto` 接会出意外。
细节见 [01-10 的陷阱二](01-10_auto_decltype.md)。需要真正的 bool 容器时，
用 `std::vector<char>` 或 `std::deque<bool>`。

## 二、NULL 的问题 `[C++11 引入 nullptr]`

### NULL 到底是什么

C 里 `NULL` 是宏，通常定义为 `0` 或 `((void*)0)`。C++ 不允许 `void*`
隐式转成其他指针类型，所以 C++ 的 `NULL` 只能是整数 `0` 或某种编译器扩展。

实测 GCC 的情况：

```
NULL 是整型吗: 1
NULL 就是 long 吗: 1
NULL 是指针吗: 0
```

`NULL` 在 GCC 下展开为 `__null`，类型是 `long`。MSVC 下直接是 `0`，类型是 `int`。
**无论哪种，它都是整型，不是指针类型** —— 这是全部问题的根源。

### 问题一：重载决议走错分支

```cpp
void overloaded(int x);
void overloaded(char* p);

overloaded(NULL);     // 你想调哪个？
```

两种编译器，两种错法：

- **GCC**：`NULL` 是 `long`，`long→int` 和 `long→char*` 都算转换、同级，
  于是**编译错误**：`call of overloaded 'overloaded(NULL)' is ambiguous`
- **MSVC**：`NULL` 是 `0`，精确匹配 `int` 版本，**静默选中 `overloaded(int)`**

一个编译不过，一个编译过了但调错函数。而 `nullptr` 的行为是确定的：

```cpp
overloaded(0);        // 选 int 版本（符合预期）
overloaded(nullptr);  // 选 char* 版本（符合预期）
```

实测输出：

```
传 NULL:    编译错误（GCC: ambiguous）/ 静默选 int（MSVC）
传 0:         overloaded(int): 0
传 nullptr:   overloaded(char*): null
```

### 问题二：模板里彻底失效

这是更严重的一个，因为现代 C++ 到处是模板：

```cpp
template <class F, class P>
void call_with_ptr(F f, P ptr) { f(ptr); }

void takes_pointer(int* p);

// call_with_ptr(takes_pointer, NULL);   // 编译错误
call_with_ptr(takes_pointer, nullptr);   // ok
```

`NULL` 作为模板实参时，`P` 被推导为 `long`（或 `int`），
而 `long` 不能隐式转成 `int*` —— "空指针"的语义在模板推导中完全丢失了。

`nullptr` 的类型是 `std::nullptr_t`，推导后仍然保留"我是空指针"这个信息，
可以转成任意指针类型。

### 问题三：可读性

```cpp
if (p == 0)         // p 是指针还是整数？要回头看声明
if (p == NULL)      // 稍好，但 NULL 是宏，有被重定义的风险
if (p == nullptr)   // 一眼就知道 p 是指针
```

### nullptr 的性质

```cpp
int* p1 = nullptr;
double* p2 = nullptr;
void (*pf)() = nullptr;      // 函数指针也行
// int n = nullptr;          // 编译错误：不能转整型
```

`nullptr` 的类型是 `std::nullptr_t`（定义在 `<cstddef>`）。
它能转换成任意指针类型和成员指针类型，但**不能转换成整型** ——
正是这一条约束消除了上面所有歧义。

规则很简单：**写新代码一律用 `nullptr`，不要用 `NULL` 或 `0`。**

## 三、传统 enum 的三个问题 `[C++11 引入 enum class]`

```cpp
enum Color { kRed, kGreen, kBlue };
enum Fruit { kApple, kBanana };
```

### 问题一：隐式转换成整型

```cpp
Color c = kRed;
int n = c;                       // 不需要任何转换，直接就能赋值
bool weird = (kRed == kApple);   // 两个毫不相关的枚举能比较，编译通过！
```

实测输出：

```
  传统 enum 隐式转 int: 0
  kRed == kApple 竟然能编译，结果=1
```

`kRed == kApple` 得到 `true`，因为两者的底层值都是 0。
这是彻底无意义的比较，但语言允许。GCC 至少会给个警告：

```
warning: comparison between 'enum traditional::Color' and 'enum traditional::Fruit' [-Wenum-compare]
```

警告有帮助，但枚举值被当整数传给函数、参与算术运算这类情况，连警告都没有。

### 问题二：枚举值泄漏到外层作用域

```cpp
enum Color { kRed, kGreen, kBlue };
enum Fruit { kApple, kBanana, kRed };   // 编译错误：kRed 重定义
```

枚举值并不在 `Color` 这个作用域里，而是直接在外层作用域可见：

```cpp
std::cout << kGreen;    // 不需要写 Color::kGreen
```

所以一个项目里所有枚举的所有枚举值都共享一个命名空间，
撞名概率随项目规模上升。这就是为什么 C 代码里到处是 `COLOR_RED`、
`FRUIT_APPLE` 这种手工加前缀的写法 —— 和 [01-01](01-01_namespace.md)
里讲的函数名加前缀是同一个困境。

### 问题三：底层类型不确定

```cpp
sizeof(Color);    // 实测 4，但标准只要求"能容纳所有枚举值"
```

实现可以选任何够用的整型。这带来两个实际麻烦：
结构体布局不可控（影响 ABI 和序列化）、无法前向声明（因为大小未知）。

## 四、enum class 解决了这三个问题 `[C++11]`

```cpp
enum class Color { Red, Green, Blue };
enum class Fruit { Apple, Banana, Red };   // Red 可以重名，互不干扰

enum class Status : std::uint8_t { Ok = 0, Error = 1, Timeout = 2 };  // 指定底层类型
```

逐条对应：

**强作用域** —— 枚举值不泄漏，必须限定访问：

```cpp
Color c = Color::Red;     // 必须写 Color::
// Color c = Red;         // 编译错误
```

于是 `Fruit::Red` 和 `Color::Red` 可以共存，不用再手工加前缀。

**禁止隐式转换** —— 要转就显式转：

```cpp
// int n = c;                     // 编译错误
int n = static_cast<int>(c);      // 要转就写清楚

// bool bad = (Color::Red == Fruit::Red);   // 编译错误：类型不同不可比较
bool ok = (Color::Red == Color::Red);       // 同类型比较没问题
```

**可指定底层类型** —— 布局可控，还支持前向声明：

```cpp
enum class Status : std::uint8_t { /* ... */ };
sizeof(Status);    // 实测 1

enum class Code : int;    // 前向声明，因为大小已知
```

实测输出：

```
  enum class 需显式转换: 0
  Color::Red 与 Fruit::Red 不可比较（类型安全）
  sizeof(Status) = 1（显式指定为 uint8_t）
  Color::Red == Color::Red: 1
```

### 传统 enum 也能指定底层类型 `[C++11]`

如果你有大量老代码依赖隐式转换，但想控制大小：

```cpp
enum LegacyWithType : short { kSmall = 1, kBig = 2 };
```

实测 `sizeof` 是 2，`std::underlying_type_t` 确认是 `short`。
这是个折中方案 —— 保留隐式转换，只修布局问题。

### enum class 的代价：输出和转换要自己写

强类型不是免费的。`enum class` 不能直接 `cout`：

```cpp
// std::cout << Status::Ok;     // 编译错误
std::cout << static_cast<int>(Status::Ok);    // 要显式转
```

实践中通常配一个 `to_string`：

```cpp
const char* to_string(Status s) {
    switch (s) {
        case Status::Ok:      return "Ok";
        case Status::Error:   return "Error";
        case Status::Timeout: return "Timeout";
    }
    return "Unknown";
}
```

这里有个实用技巧：`switch` 覆盖所有枚举值且**不写 `default`** 时，
如果以后新增了枚举值，GCC/Clang 会用 `-Wswitch` 警告你漏了分支。
写了 `default` 就失去这个检查。所以上面把兜底的 `return` 放在 `switch` 外面。

要做位标志（flags）时，`enum class` 需要手工重载运算符：

```cpp
enum class Perm : unsigned { Read = 1, Write = 2, Exec = 4 };

constexpr Perm operator|(Perm a, Perm b) {
    return static_cast<Perm>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}
constexpr bool has(Perm set, Perm bit) {
    return (static_cast<unsigned>(set) & static_cast<unsigned>(bit)) != 0;
}
```

样板代码比传统 enum 多，换来的是不会把 `Perm` 和别的 flags 混用。

## 五、小结

| 特性 | 解决的问题 |
|---|---|
| `bool` | C 用整数表示真假，类型信息缺失 |
| `nullptr` `[C++11]` | `NULL` 是整型，导致重载决议出错、模板推导失效 |
| `enum class` `[C++11]` | 传统 enum 隐式转整型、枚举值泄漏作用域、底层类型不确定 |
| `enum : T` `[C++11]` | 传统 enum 的底层类型不可控 |

实践建议：

- 空指针一律写 `nullptr`
- 新代码的枚举默认用 `enum class`；只在需要隐式转整型
  （如与 C 接口、位运算密集的场合）时用传统 enum
- 需要控制布局或序列化时，显式指定底层类型
- `switch` 枚举时不写 `default`，让编译器帮你检查遗漏分支
- 避免 `std::vector<bool>`

> 下一章：[01-07 inline 函数](01-07_inline.md)
