# 06-03 类型别名与 using

> 标准：`using` 类型别名与别名模板 `[C++11]`；`_t` 后缀 `[C++14]`；`_v` 后缀 `[C++17]`
> demo：[`06_03_using.cpp`](../../demos/part6_modern_core/06_03_using.cpp)

## 一、typedef 与 using 的基本等价

```cpp
typedef unsigned long ulong_old;
using ulong_new = unsigned long;              // 完全等价

typedef std::vector<std::string> StringVec_old;
using StringVec_new = std::vector<std::string>;
```

实测 `is_same_v` 全为 1 —— 两者产生的是同一个类型别名，没有任何区别。

唯一的差别是**写法方向**：

```cpp
typedef 旧类型 新名字;        // 名字在后
using 新名字 = 旧类型;         // 名字在前，和变量声明的方向一致
```

`using` 的方向更自然 —— 和 `int x = 1;` 一样是"名字 = 值"。

## 二、函数指针：可读性差别明显

`typedef` 的语法是"把要定义的名字放到它作为变量时该在的位置"。
对简单类型没问题，对函数指针就很别扭：

```cpp
typedef int (*FuncPtr_old)(double, const std::string&);
//            ^^^^^^^^^^^ 名字埋在中间

using FuncPtr_new = int (*)(double, const std::string&);
//    ^^^^^^^^^^^ 名字在最左边
```

更复杂的例子（返回函数指针的函数指针）：

```cpp
typedef int (*(*Complex_old)(char))(double);      // 几乎没法读
using Complex_new = int (*(*)(char))(double);     // 稍好一点
```

实测两者仍然等价（`is_same_v` 为 1），但 `using` 版本至少能一眼看出名字是什么。

同样的优势出现在数组和成员指针上：

```cpp
typedef int ArrOld[10];
using ArrNew = int[10];

typedef void (Widget::*MemFnOld)(int);
using MemFnNew = void (Widget::*)(int);
```

## 三、决定性优势：别名模板

这是 `using` 不可替代的地方 —— **`typedef` 不能有模板参数**。

C++98 的变通做法是用一个结构体包一层：

```cpp
// C++98：只能这样
template <class T>
struct MyAlloc_old {
    typedef std::vector<T, std::allocator<T>> type;      // 要通过 ::type 访问
};

typename MyAlloc_old<int>::type v;      // 使用时要写 typename 和 ::type
```

C++11 的别名模板：

```cpp
template <class T>
using MyAlloc_new = std::vector<T, std::allocator<T>>;

MyAlloc_new<int> v;                      // 直接用
```

实测两者类型相同，但新写法短得多。

### 实用例子

```cpp
template <class K, class V>
using Dict = std::map<K, V, std::less<K>>;

template <class T>
using Ptr = std::shared_ptr<T>;

template <class T>
using Callback = std::function<void(const T&)>;

Dict<std::string, int> scores;
Ptr<int> p = std::make_shared<int>(42);
Callback<int> cb = [](const int& x) { /* ... */ };
```

实测全部正常工作。

别名模板的典型用途：

- **给复杂的模板实例起短名字**（上面的 `Dict`）
- **部分指定模板参数**（固定 allocator、固定 comparator）
- **统一项目里的类型约定**（`using NodePtr = std::shared_ptr<Node>;`）
- **适配不同配置**（`using Storage = std::conditional_t<UseHeap, std::vector<T>, std::array<T,N>>;`）

## 四、为什么别名模板不需要 typename

这一点解释了标准库为什么要加 `_t` 后缀。

回想 [05-06](../part5_template/05-06_template_engineering.md) 讲的规则：
依赖模板参数的嵌套类型名必须加 `typename`。

```cpp
template <class T>
struct Wrapper_old { typedef T* type; };

template <class T>
void use_old() {
    typename Wrapper_old<T>::type p = nullptr;    // 必须写 typename
}
```

为什么必须写？因为 `Wrapper_old<T>::type` 可能被特化成别的东西：

```cpp
template <> struct Wrapper_old<int> { static int type; };    // type 变成了静态变量！
```

编译器在第一阶段无法知道，所以要你明确。

**但别名模板不同**：

```cpp
template <class T>
using Wrapper_new = T*;

template <class T>
void use_new() {
    Wrapper_new<T> p = nullptr;                    // 不需要 typename！
}
```

实测两者都能编译，但后者不需要 `typename`。

原因：**别名模板不能被特化**，所以 `Wrapper_new<T>` **保证**是一个类型。
编译器不需要你帮它消歧。

这个性质直接导致了标准库的 `_t` 后缀设计。

## 五、标准库的 _t / _v 惯例

同一件事在三个标准下的写法：

```cpp
// C++11：struct + ::type / ::value
typename std::remove_reference<int&>::type      // 类型
std::is_same<int, int>::value                    // 值

// C++14：_t 别名模板
std::remove_reference_t<int&>                    // 不需要 typename

// C++17：_v 变量模板
std::is_same_v<int, int>                          // 不需要 ::value
```

实测等价性全部为 1。

### 可读性的实际差别

看一段真实的 SFINAE 代码在三个标准下的样子：

```cpp
// C++11
typename std::enable_if<std::is_integral<T>::value, T>::type f(T);

// C++14
std::enable_if_t<std::is_integral<T>::value, T> f(T);

// C++17
std::enable_if_t<std::is_integral_v<T>, T> f(T);
```

从 C++11 到 C++17，同样的语义少了 `typename`、`::type`、`::value`
三处噪声。写模板元编程时这个差别很实质 ——
C++11 的代码里 `typename` 和 `::type` 常常占到字符数的一半。

### 实现方式

```cpp
// _t 是别名模板（C++14）
template <class T>
using remove_reference_t = typename remove_reference<T>::type;

// _v 是变量模板（C++17，见 05-02）
template <class T, class U>
constexpr bool is_same_v = is_same<T, U>::value;
```

两者都只是对原始 `struct` 形式的封装 —— 零开销。

**新代码一律用 `_t` / `_v` 版本。** 原始形式只在需要把 trait
本身当成模板参数传递时才用（比较少见）：

```cpp
template <template <class> class Trait>
void f();

f<std::is_integral>();        // 这里要传 struct 形式
```

## 六、自定义 trait 时也提供 _t / _v

自己写 trait 时应该遵循同样的惯例：

```cpp
// 布尔型 trait：提供 _v
template <class T>
struct is_string_like {
    static constexpr bool value =
        std::is_same_v<T, std::string> || std::is_same_v<T, const char*>;
};

template <class T>
constexpr bool is_string_like_v = is_string_like<T>::value;

// 类型变换 trait：提供 _t
template <class T>
struct add_const_ref { using type = const T&; };

template <class T>
using add_const_ref_t = typename add_const_ref<T>::type;
```

实测：

```
      is_string_like_v<std::string> = 1
      is_string_like_v<int> = 0
      add_const_ref_t<int> 是 const int& 吗: 1
```

好处：

1. **调用方代码更干净**
2. **风格与标准库一致**，别人不用猜
3. 原始 `struct` 形式仍然保留，需要时能用

布尔型 trait 还有个更好的写法 —— 继承 `std::bool_constant`：

```cpp
template <class T>
struct is_string_like : std::bool_constant<
    std::is_same_v<T, std::string> || std::is_same_v<T, const char*>> {};
```

这样它自动获得 `value`、`type`、`operator()`、隐式转换到 `bool`，
能参与标签分发（见 [05-05](../part5_template/05-05_type_traits.md)）。

## 七、using 的四种用途

容易混淆的一点：`using` 这个关键字在 C++ 里有**四种**不同的用途。

### 用途一：类型别名

```cpp
using StringVec = std::vector<std::string>;
```

### 用途二：别名模板

```cpp
template <class T> using Ptr = std::shared_ptr<T>;
```

### 用途三：using 声明 / using 指令（命名空间）

```cpp
using std::cout;                 // using 声明：引入单个名字
using namespace std;              // using 指令：引入整个命名空间（不推荐）
using deep::nested::func;
```

见 [01-01 命名空间](../part1_basics/01-01_namespace.md)。

### 用途四：在类中引入基类成员

```cpp
class Derived : public Base {
public:
    using Base::f;               // 引入基类的全部 f 重载（见 04-01）
    using Base::_value;          // 改变访问权限
    void f(const char*);          // 自己的版本
};

class WithInheritedCtor : public std::string {
public:
    using std::string::string;   // 继承构造函数（见 03-02）
};
```

实测全部有效：

```
      用途二：引入基类重载 + 改访问权限
        Base::f(int)
        Base::f(double)
        Derived::f(const char*)
        d._value = 42（原本是 protected）
      用途三：继承构造函数
        用 std::string 的构造函数: hello
```

这四种用途共用一个关键字，是 C++ 为了不引入新保留字做的取舍。
读代码时靠上下文区分：

- `using X = Y;` → 类型别名
- `template <...> using X = Y;` → 别名模板
- `using ns::name;` 在函数/命名空间作用域 → using 声明
- `using Base::name;` 在类内 → 引入基类成员

## 八、实践建议

**一律用 `using`，不要用 `typedef`。** 理由：

1. 方向更自然（名字在前）
2. 函数指针、数组等复杂类型的可读性明显更好
3. **能模板化**（`typedef` 不行）
4. 别名模板不需要 `typename`
5. 风格统一 —— 不用在两套语法间切换

`typedef` 的唯一场合是维护必须兼容 C++98 的代码。

**给项目里的常用类型组合起别名：**

```cpp
// 不好：到处重复这个长类型
std::map<std::string, std::vector<std::shared_ptr<Node>>> graph;

// 更好
using NodePtr = std::shared_ptr<Node>;
using NodeList = std::vector<NodePtr>;
using Graph = std::map<std::string, NodeList>;

Graph graph;
```

好处不只是短 —— 以后把 `shared_ptr` 换成 `unique_ptr`
只需要改一行。

**容器类型要提供成员别名：**

```cpp
template <class T>
class MyContainer {
public:
    using value_type = T;              // 泛型代码依赖这些
    using size_type = std::size_t;
    using iterator = T*;
    using const_iterator = const T*;
    using reference = T&;
};
```

见 [03-05](../part3_class/03-05_special_members.md)。

## 九、小结

| 要点 | 说明 |
|---|---|
| `typedef` vs `using` | 对简单类型完全等价；`using` 方向更自然 |
| 复杂类型 | 函数指针、数组、成员指针，`using` 可读性明显更好 |
| **别名模板** | `using` 独有的能力，`typedef` 不能有模板参数 |
| 不需要 `typename` | 别名模板**不能被特化**，所以保证是类型 |
| `_t` 后缀 `[C++14]` | 别名模板封装 `::type`，省掉 `typename` |
| `_v` 后缀 `[C++17]` | 变量模板封装 `::value` |
| 自定义 trait | 也应提供 `_t` / `_v`；布尔 trait 可继承 `bool_constant` |
| `using` 的四种用途 | 类型别名、别名模板、using 声明/指令、引入基类成员 |
| 实践 | **一律用 `using`**；给常用类型组合起别名；容器提供成员别名 |

> 下一章：[06-04 C++14 特性汇总](06-04_cpp14.md)
