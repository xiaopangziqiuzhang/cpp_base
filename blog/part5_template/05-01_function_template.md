# 05-01 函数模板

> 标准：C++98；`auto` 非类型参数 `[C++17]`
> demo：[`05_01_function_template.cpp`](../../demos/part5_template/05_01_function_template.cpp)

## 一、解决了什么问题

强类型语言里，参与运算的对象类型在编译时就确定，编译器要做严格的类型检查。
这带来一个矛盾：**类型安全**和**代码复用**冲突。

写一个"把两个数相加"的函数，有四种做法：

**做法一：为每种类型写一个函数**

```cpp
int    add_int(int a, int b)          { return a + b; }
double add_double(double a, double b) { return a + b; }
```

调用方要自己选对函数名。这是 C 的做法（`abs`/`fabs`/`labs`）。

**做法二：重载**

```cpp
int    add(int a, int b)       { return a + b; }
double add(double a, double b) { return a + b; }
```

调用方不用选了，但**实现仍然重复**。而且每新增一种类型就要加一个重载。

**做法三：带参数的宏**

```cpp
#define ADD(a, b) ((a) + (b))
```

能用于任何类型，但没有类型检查，而且有参数多次求值等问题
（见 [01-07](../part1_basics/01-07_inline.md)）。

**做法四：模板 —— 把类型作为参数**

```cpp
template <class T>
T add(T a, T b) { return a + b; }
```

参考文档里有个很好的比喻：把函数比作一场比赛，函数流程就是比赛规则。
以往定义函数时总要指明参数是 `int` 还是 `double`，
这就像为张三（`int`）和李四（`double`）单独制定一套规则；
王五（`char*`）和赵六（`bool`）要比赛，还得再制定一次。

模板的做法是：不管谁和谁比赛，都定义成"A 与 B 比赛"，
制定好 A 与 B 的规则后，比赛时把 A 替换成张三、B 替换成李四就行。
这个过程叫**类型参数化**。

实测模板能用于当初没想到的类型：

```
      模板: add_template(1,2)=3 add_template(1.5,2.5)=4
      模板还能用于当初没想到的类型:
        string: ab
```

（`add_template(1.5, 2.5)` 的结果是 `4.0`，默认格式下显示为 `4`。）

注意最后一行 —— 写模板时根本没考虑 `std::string`，但它照样能用，
因为 `std::string` 支持 `+`。这是模板相对重载的质的差别：
**重载支持的是"你列举出的类型"，模板支持的是"所有满足要求的类型"**。

这种编程方式叫**泛型编程**（generic programming）或通用编程。

## 二、定义形式

```cpp
template <模板参数表>
返回类型 函数名(参数列表) { 函数体 }
```

`template` 放在定义与声明的最前面，其后是用逗号分隔的模板参数表，
用尖括号括起来。模板参数表**不能为空**。

模板参数有两种：

- **类型参数**：用 `class` 或 `typename` 修饰，代表一种类型
- **非类型参数**：代表一个编译期常量（第七节）

### class 和 typename 等价

```cpp
template <class T>    T max_a(T a, T b);
template <typename T> T max_b(T a, T b);      // 完全等价
```

历史原因：C++ 标准还未统一时，很多旧编译器只支持 `class`，
因为那时还没有 `typename` 关键字。

两者在模板参数列表中完全等价。相比 `class`，`typename` 更能体现"类型"的观点
（毕竟模板参数可以是 `int` 这种非类类型，叫 `class` 有点怪）。
现代代码倾向用 `typename`，但 `class` 更短，两者都常见。
本书统一用 `class`。

注意 `typename` 在**另一个场合**有不可替代的含义 ——
用于消除嵌套依赖类型的歧义（见
[05-06](05-06_template_engineering.md)）：

```cpp
template <class T>
void f() {
    typename T::value_type x;     // 这里的 typename 不能换成 class（C++20 前）
}
```

## 三、实例化

函数模板本身不是完整的函数定义 —— 其中的类型参数还不确定，
它只定义了"某些类型的角色在函数中的操作形式"。
必须把模板参数**实例化**才能使用，实例化后的函数称为**模板函数**。

### 隐式实例化：从实参推导

```cpp
add(1, 2);          // T 推导为 int
add(1.5, 2.5);      // T 推导为 double
```

### 显式实例化：指定模板实参

```cpp
show_type<double>(i);      // T = double，实参 i 被转换成 double
show_type<int>(d);         // T = int，实参 d 被截断
```

实测验证：

```
      隐式实例化（从实参推导）:
        T 是 int 吗: 1, 是 double 吗: 0
        T 是 int 吗: 0, 是 double 吗: 1
      显式实例化（指定模板实参）:
        T 是 int 吗: 0, 是 double 吗: 1      <- show_type<double>(i)
        T 是 int 吗: 1, 是 double 吗: 0      <- show_type<int>(d)
```

显式指定的两个用途：

1. **绕过推导**，强制用某个类型
2. **处理无法推导的情况** —— 比如模板参数只出现在返回类型里：

   ```cpp
   template <class To, class From>
   To convert(From f) { return static_cast<To>(f); }

   convert<int>(3.14);     // To 必须显式指定，From 可以推导
   ```

   注意这里的参数顺序：**无法推导的参数要放在前面**，
   这样调用方只需指定它，后面的可以靠推导。

## 四、推导的两条限制

### 限制一：同一个模板参数必须推导出一致的类型

```cpp
template <class T>
T strict_add(T a, T b) { return a + b; }

// strict_add(1, 2.5);    // 编译错误：
                          // deduced conflicting types for parameter 'T' ('int' and 'double')
```

`T` 不能既是 `int` 又是 `double`。三种解法：

```cpp
strict_add<double>(1, 2.5);    // 1. 显式指定
flexible_add(1, 2.5);           // 2. 用两个模板参数
strict_add(1.0, 2.5);           // 3. 手工转换实参
```

第二种做法是现代写法：

```cpp
template <class T1, class T2>
auto flexible_add(T1 a, T2 b) { return a + b; }     // [C++14] 返回类型推导
```

C++11 下要写成 `-> decltype(a + b)`（见
[01-10](../part1_basics/01-10_auto_decltype.md)）。

### 限制二：推导不做隐式转换

```cpp
template <class T>
void takes_vector(const std::vector<T>& v);

std::vector<int> v{1, 2, 3};
takes_vector(v);                // ok

// takes_vector({1, 2, 3});     // 编译错误：无法从 initializer_list 推导
takes_vector<int>({1, 2, 3});   // 显式指定后可以
```

**模板实参推导只做精确匹配**（外加少量允许的调整：
数组转指针、函数转指针、去掉顶层 const、派生类转基类）。
普通函数调用里理所当然的隐式转换，在推导阶段不会发生。

这是模板错误信息常常让人困惑的原因之一 ——
你以为能隐式转换，但推导在转换之前就失败了。

## 五、与普通函数的重载关系

有了函数模板后，如果定义的模板与普通函数同名，同样会重载。

```cpp
void which_one(int);                 // 普通函数

template <class T> void which_one(T);     // 模板
template <class T> void which_one(T*);    // 更特化的模板
```

实测：

```
      传 int:    普通函数 which_one(int)
      传 double: 模板 which_one(T)
      传 int*:   模板 which_one(T*)（更特化）
```

决议规则（按优先级）：

1. **精确匹配的普通函数优先**于模板
   —— 这就是"普通函数会优先于函数模板执行"的准确表述
2. 多个模板都能匹配时，**更特化的模板**优先（`T*` 比 `T` 更特化）
3. 如果普通函数需要转换才能匹配，而模板能精确匹配，**模板赢**
   —— 所以 `which_one(1.5)` 选了模板，而不是把 `1.5` 转成 `int`

强制调用模板版本的写法：

```cpp
which_one<>(i);        // 空的 <> 表示"只考虑模板"
```

实测确认调到了模板版本。

## 六、非类型模板参数

模板参数除了类型，还可以是**编译期常量**：

```cpp
template <class T, int kMultiplier>
T multiply(T x) { return x * kMultiplier; }

multiply<int, 10>(5);       // 50
multiply<double, 3>(1.5);   // 4.5
```

非类型参数也可以有默认值：

```cpp
template <class T, std::size_t kSize = 8>
class FixedArray {
    T _data[kSize]{};
public:
    static constexpr std::size_t size() { return kSize; }
};

FixedArray<int> a;          // 大小 8
FixedArray<int, 16> b;      // 大小 16
```

实测输出 8 和 16。

### 必须是编译期常量

```cpp
int n = 10;
// multiply<int, n>(5);        // 编译错误：n 不是常量表达式

constexpr int kN = 10;
multiply<int, kN>(5);          // ok
```

这是非类型参数的本质约束 —— 它要参与模板实例化，
而实例化发生在编译期。

### 允许的类型

C++17 之前，非类型参数必须是：整型、枚举、指针、引用、
`std::nullptr_t`。**不能是浮点数、字符串字面量、类类型**。

```cpp
// template <double kValue> void f();     // C++20 之前不行
// template <const char* kName> void f(); // 字符串字面量不行（指针可以，但要有链接）
```

C++20 放宽到允许"结构化类型"（满足条件的类类型），
这让 `template <std::array<int,3> kArr>` 这类写法成为可能。

### 非类型参数的实际价值

它让"**大小/配置成为类型的一部分**"：

```cpp
std::array<int, 10>        // 大小是类型的一部分，栈上分配，无堆开销
std::bitset<64>
std::ratio<1, 1000>        // chrono 的基础
```

对比 `std::vector<int>`（大小在运行期）：`std::array<int, 10>`
的大小在编译期已知，可以完全放在栈上，而且 `size()` 是编译期常量。

## 七、auto 非类型参数 `[C++17]`

C++17 之前，写一个"接受任意类型常量"的模板很笨：

```cpp
// C++14
template <class T, T kValue>
void print_value();

print_value<int, 42>();        // 要写两个实参，而且类型是冗余的
print_value<char, 'x'>();
```

C++17 用 `auto` 自动推导类型：

```cpp
template <auto kValue>
void print_value() {
    std::cout << kValue << ", 类型: " << typeid(decltype(kValue)).name();
}

print_value<42>();       // kValue 是 int
print_value<'x'>();      // kValue 是 char
print_value<true>();     // kValue 是 bool
```

实测：

```
        值 = 42, 类型是 int 吗: 1, 是 char 吗: 0
        值 = x, 类型是 int 吗: 0, 是 char 吗: 1
        值 = 1, 类型是 int 吗: 0, 是 char 吗: 0      <- bool
```

这个特性在写编译期工具时很有用，比如实现编译期的类型-值映射、
或者把枚举值作为模板参数而不必重复写枚举类型。

## 八、默认模板参数

```cpp
template <class T, class Compare = std::less<T>>
bool is_ordered(const T& a, const T& b, Compare cmp = Compare{}) {
    return cmp(a, b);
}

is_ordered(1, 2);                                  // 用默认的 less -> true
is_ordered<int, std::greater<int>>(1, 2);          // 指定 greater -> false
```

实测输出 1 和 0。

标准库大量使用这个模式：

```cpp
template <class T, class Allocator = std::allocator<T>>  class vector;
template <class Key, class Compare = std::less<Key>, ...> class set;
template <class CharT, class Traits = char_traits<CharT>, ...> class basic_string;
```

**注意函数模板的默认模板参数是 C++11 引入的**，
C++98 只允许类模板有默认模板参数。

## 九、小结

| 要点 | 说明 |
|---|---|
| 解决什么 | 类型安全与代码复用的冲突；支持"所有满足要求的类型"而非"列举的类型" |
| 定义 | `template <class T> 返回类型 函数名(参数)` |
| `class` vs `typename` | 模板参数表中等价；`typename` 另有消歧用途 |
| 隐式实例化 | 从实参推导模板参数 |
| 显式实例化 | `f<T>(args)`；用于绕过推导或处理无法推导的参数 |
| 推导限制一 | 同一个 `T` 必须推导出一致的类型 |
| 推导限制二 | **不做隐式转换**，只做精确匹配 + 少量调整 |
| 与重载的关系 | 精确匹配的普通函数 > 更特化的模板 > 普通模板；`f<>()` 强制用模板 |
| 非类型参数 | 必须是编译期常量；C++17 前限整型/枚举/指针/引用 |
| 非类型参数的价值 | 让大小/配置成为类型的一部分（`std::array`、`bitset`）|
| `auto` 非类型参数 `[C++17]` | 类型自动推导，省掉冗余的类型实参 |
| 默认模板参数 | 类模板 C++98 就有；函数模板是 C++11 加入的 |

> 下一章：[05-02 类模板](05-02_class_template.md)
