# 05-03 特化与偏特化

> demo：[`05_03_specialization.cpp`](../../demos/part5_template/05_03_specialization.cpp)

## 一、为什么需要特化

模板的通用实现对某些类型是错的。最经典的例子：

```cpp
template <class T>
T add(T a, T b) { return a + b; }

add(1, 2);                  // 3，正确
add("hello,", "world");     // 指针相加 —— 编译错误（或者更糟，编译通过但毫无意义）
```

`const char*` 的 `+` 不是字符串拼接，而是指针算术。

还有一类情况：通用实现能用，但对某些类型有更优的做法。
比如 `std::vector<bool>` 用位存储（见 [01-10](../part1_basics/01-10_auto_decltype.md)），
`std::swap` 对标准容器只交换内部指针。

**特化**就是为"特定类型"提供专门的实现。

## 二、函数模板的全特化

语法有两个要点：

1. `template` 后直接跟 `<>`，里面不写类型
2. 在函数名后跟 `<>`，其中写要特化的类型

```cpp
template <>
返回类型 函数名<特化类型>(特化类型 参数1, ...);
```

例子：

```cpp
// 基础模板（必须先有它）
template <class T>
void describe(T value) { std::cout << "通用版本, 值 = " << value; }

// 全特化
template <>
void describe<bool>(bool value) {
    std::cout << "bool 特化版本, 值 = " << (value ? "true" : "false");
}

template <>
void describe<const char*>(const char* value) {
    std::cout << "const char* 特化版本, 长度 = " << std::strlen(value);
}
```

实测：

```
        通用版本, 值 = 42
        通用版本, 值 = 3.14
        bool 特化版本, 值 = true
        const char* 特化版本, 长度 = 5, 值 = hello
```

两条注意事项（源自参考文档，仍然准确）：

- **使用模板特化时，必须先有基础的函数模板**
- **特化版本的函数名和参数列表要和原基础模板相同**，避免不必要的错误

### 函数模板不能偏特化

```cpp
template <class T1, class T2> void f(T1, T2);
// template <class T> void f<T, int>(T, int);    // 编译错误：函数模板不能偏特化
```

需要"部分特化"的效果时，用**重载**：

```cpp
template <class T1, class T2> void f(T1, T2);     // 通用
template <class T> void f(T, int);                 // 重载，效果类似偏特化
```

## 三、类模板的全特化

```cpp
// 通用版本
template <class T>
class TypeInfo {
public:
    static std::string name() { return "unknown"; }
    static constexpr bool is_special = false;
};

// 全特化
template <>
class TypeInfo<int> {
public:
    static std::string name() { return "int"; }
    static constexpr bool is_special = true;
    static constexpr int min_value = -2147483648;     // 特化可以加新成员！
};
```

实测：

```
      TypeInfo<double>::name() = unknown
      TypeInfo<int>::name()    = int min_value=-2147483648
      TypeInfo<string>::name() = std::string
      特化版本可以有【完全不同的接口】（这里多了 min_value）
```

注意最后一条：**特化版本可以有完全不同的接口** ——
它是一个独立的类定义，不需要和通用版本保持一致。

这既是能力也是陷阱（见第七节）。`std::numeric_limits<T>`
就利用了这一点：整型特化有 `digits10`，浮点特化有 `epsilon`。

## 四、类模板的偏特化

**类模板可以偏特化**（函数模板不行）。偏特化是"部分固定模板参数"：

```cpp
template <class T1, class T2>
class Pair { /* 通用版本 */ };

template <class T>
class Pair<T, T> { /* 偏特化：两个类型相同 */ };

template <class T>
class Pair<T, int> { /* 偏特化：第二个是 int */ };

template <class T1, class T2>
class Pair<T1*, T2*> { /* 偏特化：都是指针 */ };

template <>
class Pair<double, double> { /* 全特化 */ };
```

实测匹配结果：

```
      Pair<char, long>     -> 通用: Pair<T1, T2>
      Pair<char, char>     -> 偏特化: Pair<T, T>（两类型相同）
      Pair<char, int>      -> 偏特化: Pair<T, int>
      Pair<int*, char*>    -> 偏特化: Pair<T1*, T2*>（都是指针）
      Pair<double, double> -> 全特化: Pair<double, double>
```

### 匹配优先级

**全特化 > 更特化的偏特化 > 通用版本**

"更特化"的判断依据是：如果偏特化 A 能匹配的所有实参集合都是
偏特化 B 能匹配集合的子集，那么 A 更特化。

如果两个偏特化都能匹配且无法比较谁更特化，就是**二义性错误**：

```cpp
template <class T> class X<T, int>;      // 候选 1
template <class T> class X<int, T>;      // 候选 2
// X<int, int> x;      // 编译错误：二义，两个偏特化都匹配且不可比较
```

### 偏特化的形式

偏特化不只是"固定某个参数"，还可以对**类型结构**做模式匹配：

```cpp
template <class T> class X<T*>;                    // 指针
template <class T> class X<T&>;                    // 引用
template <class T> class X<const T>;               // const
template <class T, std::size_t N> class X<T[N]>;   // 数组
template <class R, class... Args> class X<R(Args...)>;  // 函数类型
template <class T> class X<std::vector<T>>;        // 特定模板的实例
```

最后几种形式是**模板元编程的核心工具** ——
它让模板能够"拆解"一个类型的结构。

## 五、偏特化的主要用途：类型萃取

这是偏特化最重要的应用，整个 `<type_traits>` 库都建立在它上面。

### 判断类型属性

```cpp
template <class T>
struct is_pointer_impl { static constexpr bool value = false; };

template <class T>
struct is_pointer_impl<T*> { static constexpr bool value = true; };    // 偏特化
```

实测：

```
        is_pointer_impl<int>::value  = 0
        is_pointer_impl<int*>::value = 1
```

原理：`int*` 能匹配 `T*` 这个偏特化（`T = int`），`int` 不能，只能用通用版本。

### 变换类型

```cpp
template <class T> struct remove_ref_impl      { using type = T; };
template <class T> struct remove_ref_impl<T&>  { using type = T; };
template <class T> struct remove_ref_impl<T&&> { using type = T; };
```

实测 `remove_ref_impl<int&>::type` 和 `remove_ref_impl<int&&>::type` 都是 `int`。

这就是 `std::remove_reference` 的实现方式。`std::move` 和 `std::forward`
都依赖它（见 [02-04](../part2_memory/02-04_perfect_forwarding.md)）。

### 萃取嵌套类型

```cpp
template <class C>
struct element_type { using type = typename C::value_type; };

template <class T, std::size_t N>
struct element_type<T[N]> { using type = T; };      // 数组没有 value_type，要特化
```

实测两种都正确：

```
        vector<double> 的元素类型是 double 吗: 1
        int[5] 的元素类型是 int 吗: 1
```

这个模式解决了"内置类型没有嵌套类型"的问题 ——
容器有 `value_type`，数组没有，用偏特化把它们统一到同一个接口下。
`std::iterator_traits` 正是这么处理裸指针的：

```cpp
template <class Iter> struct iterator_traits {
    using value_type = typename Iter::value_type;
};
template <class T> struct iterator_traits<T*> {     // 裸指针的偏特化
    using value_type = T;
};
```

所以 STL 算法既能用容器迭代器，也能用裸指针。

详见 [05-05 type_traits](05-05_type_traits.md)。

## 六、坑一：函数模板特化不参与常规重载决议

这是最隐蔽的一个坑，而且后果是"代码永远不被执行"。

```cpp
template <class T> void f(T)   { /* 通用模板 */ }
template <class T> void f(T*)  { /* 另一个模板 */ }
template <> void f<int*>(int*) { /* 特化 */ }

int* p = &i;
f(p);        // 调用哪个？
```

实测：

```
      f(p) 实际调用的是:
        f(T*) 另一个模板
      ^ 走了 f(T*)，而不是我们写的 f<int*> 特化
```

**那个特化永远不会被调用。**

原因是重载决议的顺序：

1. 先在**所有基础模板**之间做重载决议 —— 这里选中了 `f(T*)`（更特化）
2. 然后看选中的那个模板**有没有适用的特化** —— `f(T*)` 没有特化
3. 而 `f<int*>` 实际上是 `f(T)` 的特化（`T = int*`），而 `f(T)` 在第 1 步就落选了

结论（Herb Sutter 的著名建议）：

> **不要特化函数模板。需要针对特定类型定制时，用重载。**

```cpp
template <class T> void f(T);        // 通用模板
void f(int*);                         // 重载 —— 行为可预测
```

类模板特化没有这个问题，可以放心用。

## 七、坑二：特化必须在首次实例化之前声明

```cpp
template <class T>
struct Widget { /* 通用版本 */ };

void early_use() { Widget<int> w; }     // 这里用通用版本实例化了 Widget<int>

template <>
struct Widget<int> { /* 特化 */ };       // 编译错误：
// error: specialization of 'Widget<int>' after instantiation
```

编译器已经用通用版本生成了 `Widget<int>`，再声明特化就冲突了。

实践规则：**把所有特化声明和基础模板放在同一个头文件里**，
并且放在基础模板之后、任何使用之前。

这个规则对跨文件的场合尤其重要 ——
如果头文件 A 用了 `Widget<int>`，头文件 B 才声明它的特化，
那么包含顺序会改变程序行为（而且可能不报错，只是行为不对）。

## 八、坑三：特化不继承通用版本的任何成员

```cpp
template <class T>
class Base {
public:
    void common() { /* ... */ }
    void also_common() { /* ... */ }
};

template <>
class Base<int> {
public:
    void common() { /* ... */ }
    // 没有 also_common！
};

Base<int> bi;
bi.common();          // ok
// bi.also_common();  // 编译错误：特化版本没有这个成员
```

实测确认。**特化是一个全新的类定义**，和通用版本没有任何关系 ——
不是继承，不是覆盖，就是两个独立的类。

后果：通用版本加了新成员，所有特化都要手工同步。
特化一多，这就成了维护负担。

### 解法：把公共部分提到基类

```cpp
template <class T>
class Common {
public:
    void shared_logic() { /* 所有版本共用 */ }
};

template <class T>
class Widget : public Common<T> {
public:
    void specific() { /* 通用实现 */ }
};

template <>
class Widget<int> : public Common<int> {      // 显式继承，复用公共部分
public:
    void specific() { /* int 专用实现 */ }
};
```

### 另一种解法：用 `if constexpr` 替代特化 `[C++17]`

很多简单的特化可以用 `if constexpr` 写在一个函数里：

```cpp
// 以前要写两个特化
template <class T>
void process(T value) {
    if constexpr (std::is_pointer_v<T>) {
        // 指针的处理
    } else {
        // 其他类型的处理
    }
}
```

这样就没有"同步成员"的问题了。详见 [05-05](05-05_type_traits.md)。

## 九、小结

| 概念 | 要点 |
|---|---|
| 为什么特化 | 通用实现对某些类型是错的，或有更优做法 |
| 函数模板全特化 | `template <> void f<T>(T)`；必须先有基础模板 |
| 函数模板偏特化 | **不支持**；用重载代替 |
| 类模板全特化 | `template <> class X<int> {}`；可以有完全不同的接口 |
| 类模板偏特化 | 支持；可对类型结构做模式匹配（`T*`、`T[N]`、`R(Args...)`）|
| 匹配优先级 | 全特化 > 更特化的偏特化 > 通用版本；无法比较时二义 |
| 主要用途 | **类型萃取**（整个 `<type_traits>` 的实现基础）|
| **坑一** | 函数模板特化不参与常规重载决议，可能永远不被调用 → **用重载代替** |
| **坑二** | 特化必须在首次实例化之前声明 → 和基础模板放同一头文件 |
| **坑三** | 特化不继承通用版本的成员 → 公共部分提到基类，或用 `if constexpr` |

> 下一章：[05-04 可变模板参数](05-04_variadic.md)
