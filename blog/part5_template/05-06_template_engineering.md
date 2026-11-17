# 05-06 模板的工程问题

> 标准：`extern template` `[C++11]`
> demo：[`05_06_template_engineering.cpp`](../../demos/part5_template/05_06_template_engineering.cpp)

前五章讲模板的语言特性。这一章讲真实项目里用模板会遇到的问题：
为什么要写 `typename`、为什么继承模板基类时要写 `this->`、
编译为什么变慢、代码为什么膨胀。

## 一、两阶段查找

模板的编译分两个阶段：

| 阶段 | 时机 | 做什么 |
|---|---|---|
| **阶段一** | 模板**定义**时 | 检查语法；查找**非依赖**名字 |
| **阶段二** | 模板**实例化**时 | 查找**依赖**名字（通过 ADL）|

"依赖名字"指的是依赖于模板参数的名字。这个两阶段设计是本章所有古怪规则的根源。

实测：

```cpp
void helper(int) { /* 定义在模板之前 */ }

template <class T>
void caller(T t) { helper(t); }

void helper(double) { /* 定义在模板之后 */ }

namespace other {
    struct Widget {};
    void helper(Widget) { /* ... */ }
}

caller(1);                    // helper(int)
caller(2.5);                  // 也是 helper(int)！
caller(other::Widget{});      // other::helper(Widget)
```

输出：

```
        全局 helper(int)
        全局 helper(int)                              <- caller(2.5)
        other::helper(Widget) —— 靠 ADL 找到
```

`caller(2.5)` 调的是 `helper(int)` 而不是 `helper(double)`，
因为 `helper(double)` 声明在模板定义**之后** ——
普通的名字查找在阶段一（定义点）就完成了，看不到它。

而 `other::helper(Widget)` 能被找到，是因为它通过 **ADL**
（见 [01-01](../part1_basics/01-01_namespace.md)）在阶段二查找，
那时实参类型已知，`other` 命名空间被纳入查找范围。

**实践含义**：模板里调用的自由函数，如果希望用户能定制，
要让它通过 ADL 查找（即定义在用户类型所在的命名空间）。
这就是 `std::swap` 惯用法的原理：

```cpp
template <class T>
void algorithm(T& a, T& b) {
    using std::swap;      // 先引入兜底版本
    swap(a, b);           // 非限定调用 -> ADL 能找到 T 所在命名空间的 swap
}
```

如果写成 `std::swap(a, b)`（限定调用），就只会用标准库版本，
用户为自己类型写的高效 `swap` 永远不会被用到。

## 二、typename 消歧

```cpp
template <class Container>
void process(const Container& c) {
    Container::value_type sum{};      // 编译错误！
}
```

实际错误信息：

```
error: need 'typename' before 'Container::value_type' because
       'Container' is a dependent scope
```

原因：`Container::value_type` 是"依赖于模板参数的嵌套名字"。
编译器在阶段一不知道 `Container` 是什么，所以无法判断
`value_type` 是**类型**还是**静态数据成员**。

标准规定：**默认当成值（非类型）**。所以要显式说明：

```cpp
typename Container::value_type sum{};     // 加 typename
```

返回类型里也一样：

```cpp
template <class Container>
typename Container::value_type first_element(const Container& c) {
    return *c.begin();
}
```

实测两者都正常工作。

### 规则

**依赖于模板参数的嵌套类型名，前面必须加 `typename`。**

```cpp
typename T::value_type         // 要加：T 是模板参数
typename std::vector<T>::iterator   // 要加：依赖 T
std::vector<int>::iterator     // 不加：完全确定的类型
typename remove_reference<T>::type  // 要加（所以有了 _t 别名）
```

这正是 C++14/17 提供 `_t` 别名的原因：

```cpp
typename std::remove_reference<T>::type    // C++11，啰嗦
std::remove_reference_t<T>                  // C++14，别名模板不需要 typename
```

C++20 在很多位置放宽了这个要求（能唯一确定是类型的地方可以省略），
但在 C++17 下仍然必须写。

## 三、template 消歧

类似的问题出现在**成员模板**上：

```cpp
template <class H>
void use(const H& h) {
    auto v = h.get<int>();      // 编译错误！
}
```

错误是莫名其妙的：

```
error: expected primary-expression before 'int'
```

原因：编译器不知道 `h.get` 是模板，于是把 `<` 解析成**小于号**：

```cpp
(h.get < int) > ()      // 这是编译器看到的
```

解法是加 `template` 关键字：

```cpp
auto v = h.template get<int>();
```

嵌套类型模板也一样：

```cpp
using T = typename H::template Nested<double>::type;
//        ^^^^^^^^     ^^^^^^^^ 两个关键字都要
```

实测可行：

```
        h.template get<int>() = 42
        Nested<double>::type 是 double: 1
```

### 规则

**依赖于模板参数的成员模板，使用时前面要加 `template`。**

这个语法很丑，但没有办法 —— C++ 的语法在不知道名字含义时就是有歧义的。

## 四、依赖基类：必须用 this-> 或限定名

```cpp
template <class T>
class Base {
public:
    void base_method() const { /* ... */ }
    int _base_value = 42;
};

template <class T>
class Derived : public Base<T> {
public:
    void f() const {
        base_method();            // 编译错误！
    }
};
```

错误信息：

```
error: there are no arguments to 'base_method' that depend on a template
       parameter, so a declaration of 'base_method' must be available
```

原因：`Base<T>` 是**依赖基类** —— 它的具体内容取决于 `T`。
编译器在阶段一**不去依赖基类里查找名字**，
因为 `Base<T>` 可能被特化成完全不同的东西：

```cpp
template <> class Base<int> { /* 可能根本没有 base_method */ };
```

两种解法：

```cpp
this->base_method();         // 解法一：加 this->
Base<T>::base_method();      // 解法二：用限定名
```

实测两种都可行。

**推荐用 `this->`** —— 更短，而且在多层继承时不需要写出具体哪一层。
（但如果要明确调用某个特定基类的版本，就用限定名。）

这就是写 CRTP（见 [04-07](../part4_oop/04-07_crtp.md)）
和模板继承体系时到处是 `this->` 的原因。

## 五、模板必须定义在头文件里

这是模板最大的工程约束。

```cpp
// widget.h
template <class T>
class Widget {
public:
    void f();        // 只声明
};

// widget.cpp
template <class T>
void Widget<T>::f() { /* 定义在这里 */ }

// main.cpp
Widget<int> w;
w.f();              // 链接错误: undefined reference to 'Widget<int>::f()'
```

原因：编译 `main.cpp` 时，编译器需要 `Widget<int>::f()` 的**定义**
才能实例化它。而定义在 `widget.cpp` 里，编译 `widget.cpp` 时
又不知道有人要 `Widget<int>`，所以不会生成代码。

所以：**模板的定义必须对所有使用它的翻译单元可见**，
实践上就是"全部放头文件"。

这带来几个连带后果：

- **编译时间增加** —— 每个包含头文件的 `.cpp` 都要重新实例化一遍
- **实现细节暴露** —— 没法像 Pimpl（见 [03-08](../part3_class/03-08_pimpl.md)）那样隐藏
- **代码膨胀** —— 多个 TU 产生重复的实例化（链接器会合并，但编译的工作重复了）

C++20 的模块（modules）正是为了解决这个问题。

## 六、显式实例化与 extern template

### 显式实例化定义

强制在某处生成模板代码：

```cpp
// 类模板
template class Processor<int>;
template class Processor<double>;

// 函数模板
template int triple<int>(int);
```

### extern template `[C++11]`

声明"别在这里实例化，别处已经实例化了"：

```cpp
// widget.h
template <class T> class Heavy { /* 完整定义 */ };

extern template class Heavy<int>;            // 告诉编译器：别实例化
extern template class Heavy<std::string>;

// heavy_inst.cpp —— 唯一一处实例化
template class Heavy<int>;
template class Heavy<std::string>;
```

### 实测效果

我用 8 个翻译单元各自使用 `Heavy<int>` 和 `Heavy<std::string>` 做了对比：

```
--- 不用 extern template（8 个 TU 各自实例化）---
耗时 1.16 s
--- 用 extern template（只在一处实例化）---
耗时 1.14 s
--- 目标文件大小对比 ---
不用: 548928 字节
使用: 219136 字节
```

**编译时间几乎没变，但目标文件总大小减少了 60%。**

这个结果值得解释清楚，因为网上常说 `extern template` 能大幅缩短编译时间：

- **编译时间**：每个 TU 仍然要解析整个模板定义（这是主要开销），
  只是不生成代码。所以节省有限，在这个小例子里被噪声掩盖了
- **目标文件大小**：实实在在减少了 —— 这直接影响**链接时间**和
  最终二进制体积，在大项目里是可观的收益

所以 `extern template` 的主要价值是**减少目标文件体积和链接时间**，
而不是编译时间。想真正缩短编译时间，手段是减少头文件依赖
（Pimpl、前向声明）或者用 C++20 模块。

### 显式实例化的另一个用途：限制可用类型

```cpp
// matrix.h
template <class T> class Matrix { /* 只声明成员函数 */ };

// matrix.cpp
template <class T> void Matrix<T>::multiply() { /* 定义 */ }
template class Matrix<float>;      // 只为这两种类型生成代码
template class Matrix<double>;
```

这样 `Matrix<int>` 会链接失败 ——
相当于用链接错误强制"这个模板只支持浮点类型"。
好处是实现可以藏在 `.cpp` 里（恢复了编译防火墙），
代价是失去了泛型的开放性。

## 七、编译期开销：模板元编程 vs constexpr

### 两种编译期计算

```cpp
// 方式一：模板递归（C++98 的唯一选择）
template <int N>
struct Factorial { static constexpr long long value = N * Factorial<N-1>::value; };
template <> struct Factorial<0> { static constexpr long long value = 1; };

// 方式二：constexpr 函数（C++14 之后）
constexpr long long factorial_fn(int n) {
    long long r = 1;
    for (int i = 2; i <= n; ++i) r *= i;
    return r;
}
```

两者结果相同（实测都是 `2432902008176640000`），但编译代价差别很大。

### 实测

第一次测量我得到了**反直觉的结果**：

```
=== 模板递归（400 个实例，链有重叠）===
耗时 0.02 s, 峰值内存 16276 KB
=== constexpr 函数（同样的计算量）===
耗时 0.10 s, 峰值内存 14804 KB
```

模板反而更快。原因是**memoization**：
400 个 `Sum<100>` 到 `Sum<499>` 共享同一条递归链，
`Sum<50>` 只实例化一次就被所有后续计算复用。
而 `constexpr` 函数每次调用都要重新执行循环。

改成**互不重叠的递归链**（memoization 帮不上忙）后：

```
=== 模板递归（60 条独立的 300 层递归链）===
耗时 0.19 s, 峰值内存 67568 KB
=== constexpr 函数 ===
耗时 0.00 s, 峰值内存 14288 KB
```

内存差了 **4.7 倍**，时间差距也很明显。

### 结论

- 模板递归的每一层都是一个**独立的类型**，要进编译器的类型表，占内存
- `constexpr` 函数只有一个函数，编译器直接在内部求值器里执行
- 但如果多个计算共享递归链，模板的 memoization 可能反超

**实践建议：C++14 之后，编译期计算优先用 `constexpr` 函数。**
模板元编程留给"真正需要操作类型"的场合（type_traits、
类型列表、SFINAE），不要用它做数值计算。

## 八、减少代码膨胀

模板的每个实例化都是独立的代码。`vector<int>`、`vector<double>`、
`vector<MyClass>` 各有一份完整的方法实现。

### 技巧：把与类型无关的逻辑提出去

```cpp
// 不好：全部代码都随 T 复制
template <class T>
class BadVector {
    void reserve(std::size_t n) { /* 大量与 T 无关的簿记逻辑 */ }
};

// 更好：与类型无关的部分放非模板基类
class VectorBase {
protected:
    void* _data = nullptr;
    std::size_t _size = 0, _capacity = 0;
    void grow_impl(std::size_t new_cap, std::size_t elem_size);   // 只有一份代码
};

template <class T>
class GoodVector : private VectorBase {
public:
    void reserve(std::size_t n) { grow_impl(n, sizeof(T)); }      // 转调
};
```

这样 `GoodVector<int>` 和 `GoodVector<double>` 共享 `grow_impl` 的代码。
标准库的 `std::vector` 实现用了类似手法。

### 其他减少膨胀的手段

- **类型擦除**：用 `void*` 或 `std::function` 统一处理，只保留一份代码
- **减少实例化的类型数**：`vector<int>` 和 `vector<long>`
  在 64 位 Linux 上布局相同，但仍是两份代码 ——
  能统一的话就统一
- **把大函数移出模板**：模板里只放需要类型信息的那几行
- **显式实例化 + extern template**：减少重复实例化

## 九、小结

| 问题 | 原因 | 解法 |
|---|---|---|
| 两阶段查找 | 定义时查非依赖名字，实例化时查依赖名字 | 需要用户定制的函数靠 ADL 查找（`using std::swap`）|
| 要写 `typename` | 编译器不知道嵌套名字是类型还是值 | `typename T::value_type`；或用 `_t` 别名 |
| 要写 `template` | `<` 被解析成小于号 | `h.template get<int>()` |
| 继承模板基类要写 `this->` | 编译器不进依赖基类查找名字 | `this->member` 或 `Base<T>::member` |
| 模板必须在头文件 | 实例化需要看到定义 | 接受它；或用显式实例化限制类型 |
| `extern template` | 避免每个 TU 重复实例化 | **主要减少目标文件体积和链接时间**（实测 -60%），编译时间节省有限 |
| 编译期计算慢 | 模板递归的每层都是独立类型 | **优先用 `constexpr` 函数**（实测内存差 4.7 倍）|
| 代码膨胀 | 每个实例化一份完整代码 | 把与类型无关的逻辑提到非模板基类/函数 |

Part 5 到此结束。

> 下一章：[06-01 lambda 表达式全解](../part6_modern_core/06-01_lambda.md)
