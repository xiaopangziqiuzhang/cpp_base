# 05-05 编译期计算与 type_traits

> 标准：`static_assert`、`<type_traits>` `[C++11]`；`_v` 变量模板 `[C++17]`；
> `if constexpr`、`void_t` `[C++17]`
> demo：[`05_05_type_traits.cpp`](../../demos/part5_template/05_05_type_traits.cpp)

## 一、解决了什么问题

模板对类型参数有**隐式要求**：

```cpp
template <class T>
T average(const std::vector<T>& v) {
    T sum{};
    for (const auto& x : v) sum += x;      // 要求 T 支持 +=
    return sum / v.size();                  // 要求 T 支持 / size_t
}
```

如果传了不满足要求的类型，错误信息会出现在**模板内部**，
而且往往是几十行的实例化栈：

```
error: no match for 'operator+=' (operand types are 'Widget' and 'const Widget')
  in instantiation of 'T average(const std::vector<T>&) [with T = Widget]'
  required from here ...
```

调用方看到的错误指向库的内部，而真正的问题是"你传的类型不对"。

这一章讲的工具都在解决同一类问题：**让模板的要求变成显式的、可检查的、
报错清晰的约束**，并且能根据类型**选择不同的实现**。

## 二、static_assert `[C++11]`

编译期断言，条件不满足就终止编译并输出指定消息：

```cpp
template <class T>
class RequiresInteger {
    static_assert(std::is_integral<T>::value, "T 必须是整型");
};

RequiresInteger<int> ri;        // ok
// RequiresInteger<double> rd;  // error: static assertion failed: T 必须是整型
```

对比不加断言的情况：错误会出现在模板内部某处，
而加了断言之后错误直接指向"T 必须是整型"这一行。

两个细节：

- **C++11 要求消息参数，C++17 起可以省略**（`static_assert(cond);`）
- 它也可以用在函数体内、命名空间作用域、类定义内

```cpp
template <class T>
T safe_divide(T a, T b) {
    static_assert(std::is_arithmetic_v<T>, "只能用于算术类型");
    return b == 0 ? T{} : a / b;
}
```

### 与 assert 的区别

| | `static_assert` | `assert` |
|---|---|---|
| 检查时机 | **编译期** | 运行期 |
| 失败后果 | 编译错误 | `abort()` |
| 能检查什么 | 常量表达式、类型属性 | 任何运行期条件 |
| Release 版本 | 仍然生效 | 被 `NDEBUG` 移除 |

C++98 时代要模拟编译期断言，只能用一些 trick（比如给数组一个负长度），
错误信息极其难懂。`static_assert` 把它变成了语言特性。

## 三、type_traits 全景

`<type_traits>` 提供了一批**编译期查询类型属性**的工具。
分三大类。

### 类别一：类型判断（返回 bool）

```cpp
std::is_integral_v<T>               // 是否整型
std::is_floating_point_v<T>         // 是否浮点
std::is_arithmetic_v<T>             // 是否算术类型
std::is_pointer_v<T>                // 是否指针
std::is_class_v<T>                  // 是否类类型
std::is_enum_v<T>                   // 是否枚举
std::is_same_v<T, U>                // 两个类型是否相同
std::is_base_of_v<Base, Derived>    // 继承关系
std::is_convertible_v<From, To>     // 是否可隐式转换
```

### 类别二：操作能力判断

```cpp
std::is_default_constructible_v<T>
std::is_copy_constructible_v<T>
std::is_move_constructible_v<T>
std::is_nothrow_move_constructible_v<T>     // 见 02-03，判断真移动的关键
std::is_destructible_v<T>
std::is_trivially_copyable_v<T>             // 可以用 memcpy 吗
std::is_polymorphic_v<T>                    // 有虚函数吗
```

实测几个类型的属性：

```
        int:
          整型=1 浮点=0 指针=0 类=0
          可默认构造=1 可拷贝=1 nothrow可移动=1
          多态=0 平凡=1 大小=4
        std::string:
          整型=0 浮点=0 指针=0 类=1
          可默认构造=1 可拷贝=1 nothrow可移动=1
          多态=0 平凡=0 大小=32
        WithVirtual:
          多态=1 平凡=0 大小=8
```

注意 `is_polymorphic` 对有虚函数的类是 1，`is_trivial` 对 `int` 是 1 ——
这些信息让库能针对不同类型选择最优实现
（比如对 trivially copyable 的类型用 `memcpy` 而不是逐个拷贝构造）。

### 类别三：类型变换

```cpp
std::remove_reference_t<int&>           // int
std::remove_const_t<const int>          // int
std::remove_pointer_t<int*>             // int
std::add_lvalue_reference_t<int>        // int&
std::decay_t<const int&>                // int
std::conditional_t<cond, A, B>          // 编译期三目运算
std::common_type_t<int, double>         // double
std::underlying_type_t<SomeEnum>        // 枚举的底层类型
```

实测全部正确。

**`decay_t` 是最常用的一个** —— 它去掉引用和 cv 限定、
把数组转成指针、把函数转成函数指针，结果相当于"按值传参时的类型"：

```cpp
template <class T>
void f(T&& arg) {
    using ValueType = std::decay_t<T>;     // 拿到"干净"的类型
    ValueType copy = arg;
}
```

这在写泛型代码时几乎必不可少 —— 万能引用推导出的 `T`
可能带引用和 const（见 [02-04](../part2_memory/02-04_perfect_forwarding.md)），
需要 `decay_t` 还原。

### `::value` / `::type` 与 `_v` / `_t` 后缀

```cpp
// C++11 原始写法
std::is_same<int, int>::value
typename std::remove_reference<T>::type

// C++14 的 _t 别名模板
std::remove_reference_t<T>

// C++17 的 _v 变量模板
std::is_same_v<int, int>
```

后两者就是变量模板和别名模板（见 [05-02](05-02_class_template.md)
和 [06-03](../part6_modern_core/06-03_using.md)）。
它们消掉了大量 `typename` 和 `::value` 噪声 ——
**新代码一律用 `_t` / `_v` 版本**。

## 四、SFINAE：替换失败不是错误

**SFINAE（Substitution Failure Is Not An Error）**：
模板实参替换失败时，编译器不报错，只是把该候选**从重载集里剔除**。

这是 C++ 模板实现"条件编译"的基础机制。

```cpp
// 只有 T 有 size() 成员时，这个重载才可用
template <class T>
auto get_size(const T& t) -> decltype(t.size()) { return t.size(); }
```

如果 `T` 没有 `size()`，`decltype(t.size())` 替换失败 ——
这个候选被静默剔除，而不是编译错误。

### 优先级标签分发

写兜底版本时有个坑。我最初写的是：

```cpp
template <class T> auto get_size(const T& t) -> decltype(t.size());
template <class T> std::size_t get_size(const T&, ...);     // 想用省略号降低优先级
```

结果是**二义性错误**：

```
error: call of overloaded 'get_size(std::vector<int>&)' is ambiguous
```

原因：省略号只在"前面的参数需要转换"时才降低优先级。
这里两个候选的第一个参数都精确匹配，所以优先级相同。

正确的做法是**优先级标签（priority tag）**，这也是标准库的常用手法：

```cpp
template <int N> struct Priority : Priority<N - 1> {};
template <> struct Priority<0> {};

// 高优先级：T 有 size() 时才可用
template <class T>
auto get_size_impl(const T& t, Priority<1>) -> decltype(t.size()) { return t.size(); }

// 低优先级：兜底
template <class T>
std::size_t get_size_impl(const T&, Priority<0>) { return 1; }

template <class T>
std::size_t get_size(const T& t) { return get_size_impl(t, Priority<1>{}); }
```

`Priority<1>` 继承自 `Priority<0>`，所以传 `Priority<1>{}` 时，
匹配 `Priority<1>` 是精确匹配（优先），匹配 `Priority<0>` 需要派生类转基类（次优）。

实测：

```
      get_size(vector) = 用 t.size(): 3
      get_size(int)    = 没有 size()，返回 1: 1
```

### 检测成员是否存在：void_t 惯用法

```cpp
template <class T, class = void>
struct has_push_back : std::false_type {};

template <class T>
struct has_push_back<T, std::void_t<decltype(std::declval<T&>().push_back(
                                        std::declval<typename T::value_type>()))>>
    : std::true_type {};
```

实测：

```
      has_push_back<vector<int>>  = 1
      has_push_back<std::string>  = 1（string 确实有 push_back）
      has_push_back<std::set<int>>= 0（set 没有，这才是反例）
```

（写 demo 时我最初用 `std::string` 做反例，结果是 1 ——
因为 `std::string` 确实有 `push_back`。换成 `std::set` 才对。
选对照组时要先确认它真的不满足条件。）

原理：

- `std::void_t<...>` 把任意类型序列映射成 `void`
- 如果 `decltype(...)` 里的表达式合法，偏特化的第二个参数就是 `void`，
  匹配 `has_push_back<T, void>`，选中偏特化（继承 `true_type`）
- 如果表达式非法，偏特化不可用（SFINAE），退回通用版本（继承 `false_type`）

`std::declval<T>()` 用于在不构造对象的情况下得到一个 `T` 类型的"假值"，
专门用于 `decltype` 这种不求值的上下文。

## 五、enable_if：条件启用模板

`std::enable_if` 是 SFINAE 的标准封装：

```cpp
template <bool B, class T = void> struct enable_if {};           // B 为 false 时没有 type
template <class T> struct enable_if<true, T> { using type = T; };
```

条件为 false 时 `enable_if<...>::type` 不存在，触发 SFINAE 剔除该候选。

### 三种使用位置

```cpp
// 位置一：返回类型（最常见）
template <class T>
std::enable_if_t<std::is_integral_v<T>, T> half(T value) { return value >> 1; }

template <class T>
std::enable_if_t<std::is_floating_point_v<T>, T> half(T value) { return value / 2; }

// 位置二：模板参数默认值
template <class T, class = std::enable_if_t<std::is_pointer_v<T>>>
void only_pointers(T p);

// 位置三：函数参数默认值
template <class T>
void f(T value, std::enable_if_t<std::is_integral_v<T>, int> = 0);
```

实测：

```
      half(10)   = 整型版本（位移）: 5
      half(10.0) = 浮点版本（除法）: 5
```

两个函数同名同参数个数，靠 `enable_if` 让它们**互斥地可用**。

### enable_if 的问题

- **难读** —— 签名被 `enable_if_t<...>` 淹没
- **错误信息糟糕** —— 条件不满足时只说"没有匹配的重载"，不说为什么
- **写错容易静默失效** —— 两个条件没有互斥的话，还是二义性
- **样板代码多**

这些问题在 C++17 的 `if constexpr` 和 C++20 的 concepts 之后大部分得到解决。

## 六、if constexpr：取代大部分 SFINAE `[C++17]`

`if constexpr` 的关键性质是：**条件不成立的分支不被实例化**。

```cpp
template <class T>
T half(T value) {
    if constexpr (std::is_integral_v<T>) {
        return value >> 1;              // 只有整型时这行才被编译
    } else if constexpr (std::is_floating_point_v<T>) {
        return value / 2;
    } else {
        static_assert(std::is_arithmetic_v<T>, "只支持算术类型");
        return T{};
    }
}
```

实测和 `enable_if` 版本行为完全一致，但代码从两个函数变成一个，
而且可读性好得多。

注意 `value >> 1` 对 `double` 是非法的 ——
普通 `if` 下这行代码即使不执行也要编译通过，所以会报错。
`if constexpr` 让它在 `T` 不是整型时根本不存在。

### 更多用法

```cpp
template <class T>
void smart_print(const T& value) {
    if constexpr (std::is_pointer_v<T>) {
        std::cout << "指针，指向: " << (value ? "有效" : "空");
    } else if constexpr (std::is_same_v<T, std::string>) {
        std::cout << "字符串，长度 " << value.size();
    } else if constexpr (std::is_arithmetic_v<T>) {
        std::cout << "数值: " << value;
    }
}
```

实测三种类型都走对了分支。

配合可变参数模板处理空包：

```cpp
template <class... Args>
void print_all(const Args&... args) {
    if constexpr (sizeof...(args) > 0) {
        ((std::cout << args << "\n"), ...);
    }
}
```

### 对比表

| | SFINAE / enable_if | `if constexpr` |
|---|---|---|
| 代码量 | 多个函数 + 条件 | 一个函数 |
| 可读性 | 差 | 好 |
| 错误信息 | "没有匹配的重载" | 指向具体的行 |
| 能做什么 | 控制**重载集** | 控制**函数体内的分支** |
| 适用 | 需要根据条件选择不同的函数签名 | 同一签名下的不同实现 |

**`if constexpr` 不能完全取代 SFINAE** ——
如果需要根据条件决定"这个函数是否存在"（影响重载决议、
影响 `is_invocable` 之类的检测），还是要用 enable_if 或 C++20 的 `requires`。

但对于"同一个函数的不同实现"这个最常见的场景，`if constexpr` 是更好的工具。

## 七、实战：根据类型选择最优实现

一个真实的例子 —— 计算迭代器距离，对随机访问迭代器用 O(1)，否则 O(n)：

```cpp
template <class Iter>
std::size_t my_distance(Iter first, Iter last) {
    using Category = typename std::iterator_traits<Iter>::iterator_category;
    if constexpr (std::is_base_of_v<std::random_access_iterator_tag, Category>) {
        return static_cast<std::size_t>(last - first);     // O(1)
    } else {
        std::size_t n = 0;
        while (first != last) { ++first; ++n; }             // O(n)
        return n;
    }
}
```

实测：

```
      vector: 随机访问迭代器，O(1): 5
      list:   前向迭代器，O(n) 遍历: 3
```

这正是 `std::distance` 的实现思路（标准库用的是标签分发，
因为它要兼容 C++98）。

这个模式是 STL 高效的秘密之一：**接口统一，实现按类型特化**。
`std::copy` 对 trivially copyable 类型用 `memmove`、
`std::sort` 对小区间用插入排序、`std::advance` 对随机访问迭代器用加法 ——
全都是同一个思路。

## 八、小结

| 工具 | 用途 |
|---|---|
| `static_assert` `[C++11]` | 编译期断言，把隐式要求变成清晰的错误消息 |
| type_traits：判断 | `is_integral_v`、`is_same_v`、`is_base_of_v` 等 |
| type_traits：能力 | `is_nothrow_move_constructible_v`、`is_trivially_copyable_v` |
| type_traits：变换 | `remove_reference_t`、`decay_t`、`conditional_t`、`common_type_t` |
| `_t` / `_v` 后缀 | C++14/17 的别名，消掉 `typename` 和 `::value` 噪声 |
| **SFINAE** | 模板替换失败不报错，只剔除候选 —— 条件编译的基础机制 |
| 优先级标签 | 组织多个 SFINAE 候选的标准手法；省略号兜底会二义 |
| `void_t` 惯用法 `[C++17]` | 检测成员/操作是否存在 |
| `enable_if` | SFINAE 的封装；三种位置；可读性差、错误信息糟 |
| **`if constexpr` `[C++17]`** | 不成立的分支不实例化；取代大部分 enable_if 用法 |
| 两者的分工 | enable_if 控制**重载集**，`if constexpr` 控制**函数体分支** |

> 下一章：[05-06 模板的工程问题](05-06_template_engineering.md)
