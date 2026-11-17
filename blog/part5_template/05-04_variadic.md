# 05-04 可变模板参数

> 标准：可变参数模板 `[C++11]`、折叠表达式 `[C++17]`
> demo：[`05_04_variadic.cpp`](../../demos/part5_template/05_04_variadic.cpp)

可变模板参数（variadic templates）是 C++11 新增的最强大的特性之一。
它对参数进行了高度泛化，能表示 **0 到任意个数、任意类型**的参数。
它也是 C++11 中最难理解和掌握的特性之一。

## 一、解决了什么问题

C 的可变参数方案是 `varargs`：

```c
int sum_c(int count, ...) {
    va_list args;
    va_start(args, count);
    int total = 0;
    for (int i = 0; i < count; ++i) total += va_arg(args, int);
    va_end(args);
    return total;
}
sum_c(3, 1, 2, 3);     /* 6 */
```

四个问题：

1. **没有类型信息** —— `va_arg(args, int)` 要你自己说类型，说错了就是未定义行为
2. **必须手工传个数或哨兵** —— `sum_c(3, ...)` 的 `3` 和实际参数个数不一致就崩
3. **不能传类对象** —— 只能传 POD 类型，`std::string` 不行
4. **错误无法检查** —— `printf("%d", 1.5)` 这类错误只能靠编译器对 printf 特判

C++98 的替代方案是重载一堆函数：

```cpp
template <class T1> void f(T1);
template <class T1, class T2> void f(T1, T2);
template <class T1, class T2, class T3> void f(T1, T2, T3);
// ... 写到多少个为止？
```

Boost 库用宏生成几十个这样的重载。`std::tuple` 在 C++03 下只能支持固定上限的元素数。

可变参数模板一次解决了所有问题：**类型安全、个数任意、支持任何类型、编译期检查**。

## 二、参数包

声明可变参数模板，在 `typename` 或 `class` 后带上省略号：

```cpp
template <class... Args>       // Args 是【模板参数包】
void func(Args... args);        // args 是【函数参数包】
```

术语：

- **`Args`** 是模板参数包，表示可以接受任意多个类型作为模板参数，
  编译器把多个模板参数打包成"单个"的模板参数包
- **`args`** 是函数参数包，表示函数可以接受多个任意类型的参数

### 省略号的位置规则

这是初学时最容易混的点：

- **声明**一个变量/标识符为参数包时，**省略号在它左侧**：`class... Args`、`Args... args`
- **使用**参数包时，**省略号在名字右侧**：`args...` —— 表示**立即展开**（解包）

```cpp
template <class... Args>        // 声明：省略号在 Args 左边
void f(Args... args) {          // 声明：省略号在 args 左边
    g(args...);                 // 使用：省略号在 args 右边 -> 展开
}
```

C++11 要求函数参数包必须唯一，且是函数的最后一个参数；模板参数包则没有这个限制。

### sizeof...：求参数包的长度

```cpp
template <class... Args>
void count_args(Args... args) {
    std::cout << sizeof...(Args) << " " << sizeof...(args);
}
```

实测：

```
        sizeof...(Args) = 0, sizeof...(args) = 0      <- count_args()
        sizeof...(Args) = 1, sizeof...(args) = 1      <- count_args(3.14159)
        sizeof...(Args) = 2, sizeof...(args) = 2      <- count_args('a', "hello")
        sizeof...(Args) = 4, sizeof...(args) = 4
```

`sizeof...` 是编译期常量，可以用在 `static_assert`、数组大小、
`if constexpr` 条件里。

## 三、展开方式一：递归

这是最直观也是 C++11 下的标准做法：**提供一个参数包展开函数和一个递归终止函数**。

```cpp
// 递归终止函数（必须有）
void print() { std::cout << "\n"; }

// 递归展开：取出第一个，剩下的递归处理
template <class T, class... Args>
void print(T first, Args... rest) {
    std::cout << first;
    if (sizeof...(rest) > 0) std::cout << ", ";
    print(rest...);          // 展开剩余参数包，递归调用
}

print(1, 2.5, "three", 'x');
```

实测输出：

```
        1, 2.5, three, x
        42
        （空行，对应 print()）
```

展开过程：

```
print(1, 2.5, "three", 'x')     T=int,         rest = {2.5, "three", 'x'}
  print(2.5, "three", 'x')      T=double,      rest = {"three", 'x'}
    print("three", 'x')         T=const char*, rest = {'x'}
      print('x')                T=char,        rest = {}
        print()                 终止
```

每一层递归实例化一个新的函数，所以 `sizeof...(rest)` 在每层都是编译期常量。

**优点**：直观，C++11 就能用。
**缺点**：必须写一个重载的递归终止函数；每层都实例化一个函数，编译变慢。

## 四、展开方式二：逗号表达式 + 初始化列表

递归是标准做法，但必须有终止函数这点很烦。
另一种方式借助**逗号表达式**和**初始化列表**，不需要递归：

```cpp
template <class T>
void print_one(const T& t) { std::cout << t << " "; }

template <class... Args>
void print_all(Args... args) {
    int dummy[] = {(print_one(args), 0)...};      // 关键这一行
    (void)dummy;
}
```

原理拆开看：

- `(print_one(args), 0)` 是逗号表达式：先执行 `print_one(args)`（副作用是打印），
  整个表达式的值是 `0`
- `...` 把它展开成 `{(print_one(a1),0), (print_one(a2),0), (print_one(a3),0)}`
- 于是 `dummy` 是一个全 0 的数组，而副作用是依次打印了所有参数

实测：

```
        1 a 12.12 hello
```

### 空参数包的陷阱

参数包为空时，`int dummy[] = {};` 是**零长度数组**，不合法。
解法是补一个元素：

```cpp
int dummy[] = {0, (print_one(args), 0)...};      // 第一个 0 保证数组非空
```

实测空包版本正常工作。

### 求值顺序的保证

这个技巧的一个重要优势：**初始化列表的元素求值顺序是从左到右，标准保证**。
而函数参数的求值顺序在 C++17 之前是未指定的，所以不能写成：

```cpp
f(print_one(args)...);     // 求值顺序不确定！
```

这个写法在 C++11/14 下输出顺序可能是乱的。

## 五、展开方式三：折叠表达式 `[C++17]`

C++17 引入**折叠表达式（fold expression）**，把上面两种技巧都变得不必要。

### 四种形式

```cpp
// 一元右折叠: (pack op ...)   ->  a1 op (a2 op (a3 op a4))
template <class... Args> auto sum_right(Args... args) { return (args + ...); }

// 一元左折叠: (... op pack)   ->  ((a1 op a2) op a3) op a4
template <class... Args> auto sum_left(Args... args) { return (... + args); }

// 二元右折叠: (pack op ... op init)
// 二元左折叠: (init op ... op pack)   <- 带初值，能处理空包
template <class... Args> auto sum_init(Args... args) { return (0 + ... + args); }
```

实测：

```
      sum_right(1,2,3,4) = 10
      sum_left(1,2,3,4)  = 10
      sum_with_init()    = 0（空包也能工作）
```

**一元折叠不能处理空包**（除了 `&&`、`||`、`,` 这三个有默认值的运算符），
**二元折叠可以** —— 所以实践中优先用带初值的形式。

### 实用例子

```cpp
// 用逗号运算符折叠：对每个参数执行操作
template <class... Args>
void print_all(Args... args) {
    ((std::cout << args << " "), ...);
}

// 逻辑运算
template <class... Args> bool all_true(Args... args) { return (args && ...); }
template <class... Args> bool any_true(Args... args) { return (args || ...); }

// 拼接字符串
template <class... Args>
std::string concat(const Args&... args) {
    std::ostringstream oss;
    (oss << ... << args);                   // 二元左折叠
    return oss.str();
}
```

实测：

```
      print_all: 1 a 2.5 str
      all_true(true,true,false) = 0
      any_true(false,false,true) = 1
      concat("a", 1, "-", 2.5) = a1-2.5
```

对比三种方式实现同一个 `print_all`：

```cpp
// C++11 递归：需要 2 个函数 + 终止条件
void print() {}
template <class T, class... Args> void print(T f, Args... r) { /* ... */ print(r...); }

// C++11 逗号技巧：1 个函数，但要用数组 trick + 处理空包
template <class... Args> void print(Args... args) { int d[]={0,(f(args),0)...}; (void)d; }

// C++17 折叠：一行
template <class... Args> void print(Args... args) { ((std::cout << args), ...); }
```

折叠表达式是 C++17 在模板编程上最实用的改进之一。

支持的运算符：几乎所有二元运算符，包括 `+ - * / % ^ & | << >> == != < > && || , . ->*`
和各种复合赋值。

## 六、包扩展表达式的形式

假设 `args` 是函数参数包，扩展方式有：

| 写法 | 展开结果 |
|---|---|
| `f(args...)` | `f(a1, a2, ..., aN)` —— 整个包作为实参列表 |
| `f(args)...` | `f(a1), f(a2), ..., f(aN)` —— 对每个参数分别调用 |
| `(f(args), 0)...` | `(f(a1),0), (f(a2),0), ...` —— 逗号表达式形式 |
| `f(g(args)...)` | `f(g(a1), g(a2), ..., g(aN))` —— 先变换再传递 |

**包扩展 `exp...` 相当于把省略号左侧的表达式 `exp` 视为一个整体来展开。**

实测：

```
        f(args...)        -> 参数个数 3
        f(args)...        -> 1 2 3
        f(square(args))...-> 1 4 9        <- 每个参数先平方再打印
```

最后一种形式很有用 —— 可以在转发前对每个参数做变换：

```cpp
template <class... Args>
void log_all(Args&&... args) {
    print(to_string(std::forward<Args>(args))...);     // 每个都转成字符串
}
```

## 七、与完美转发的固定搭配

可变参数模板最常见的用法是配合完美转发
（见 [02-04](../part2_memory/02-04_perfect_forwarding.md)）：

```cpp
template <class T, class... Args>
std::unique_ptr<T> my_make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
```

实测转发到不同的构造函数：

```
        Widget()
        Widget(int 42)
        Widget(int 42, string hello)
```

注意 `Args&&... args` 和 `std::forward<Args>(args)...` 这个组合：

- 声明时 `Args&&...` —— 万能引用的参数包
- 使用时 `std::forward<Args>(args)...` ——
  展开时 `Args` 和 `args` **同步展开**，第 i 个 `forward<Args_i>(args_i)`

**这是现代 C++ 的固定搭配**，出现在 `make_unique`、`make_shared`、
`emplace_back`、`std::thread` 的构造函数、`std::function`、
`std::bind`、`std::invoke` 里。

另一个常见模式是通用包装器：

```cpp
template <class F, class... Args>
auto invoke_and_report(F&& f, Args&&... args) {
    // 前后可以加日志、计时、异常处理
    return std::forward<F>(f)(std::forward<Args>(args)...);
}
```

## 八、可变参数类模板：tuple 的原理

类模板也可以有参数包。`std::tuple` 的基本思路是**递归继承**：

```cpp
template <class... Types> class MiniTuple;

template <> class MiniTuple<> {};                  // 递归终止：空 tuple

template <class Head, class... Tail>
class MiniTuple<Head, Tail...> : private MiniTuple<Tail...> {
public:
    MiniTuple(Head h, Tail... t) : MiniTuple<Tail...>(t...), _head(std::move(h)) {}
    Head& head() { return _head; }
    MiniTuple<Tail...>& tail() { return *this; }
private:
    Head _head;
};

MiniTuple<int, double, std::string> t(1, 2.5, "three");
```

实测：

```
        head = 1
        tail().head() = 2.5
        tail().tail().head() = three
```

每一层存一个元素，剩下的交给基类。`MiniTuple<int,double,string>`
的继承链是：

```
MiniTuple<int, double, string>  存 int
  └─ MiniTuple<double, string>  存 double
       └─ MiniTuple<string>     存 string
            └─ MiniTuple<>      空
```

真实的 `std::tuple` 实现更复杂（用多重继承而非线性继承，
以便 `std::get<N>` 是 O(1) 而不是 O(N) 的递归），但思路相同。

实测标准库版本：`std::get<2>(st)` 得到 `three`。

## 九、小结

| 概念 | 要点 |
|---|---|
| 解决什么 | C 的 varargs 无类型安全、要传个数、不能传对象 |
| 模板参数包 | `template <class... Args>` |
| 函数参数包 | `void f(Args... args)` |
| 省略号位置 | **声明在左**（`class... Args`），**使用在右**（`args...` 表示展开）|
| `sizeof...` | 求包的长度，编译期常量 |
| 展开方式一：递归 | 需要终止函数；C++11 标准做法；每层实例化一个函数 |
| 展开方式二：逗号技巧 | `int d[] = {0, (f(args),0)...};` 不需递归，但要处理空包 |
| 展开方式三：折叠 `[C++17]` | `(args + ...)` 一行搞定；优先用带初值的二元形式 |
| 折叠的四种形式 | 一元右/左、二元右/左；**只有二元形式能处理空包** |
| 包扩展形式 | `f(args...)` vs `f(args)...` vs `f(g(args)...)` |
| **固定搭配** | `Args&&... args` + `std::forward<Args>(args)...` |
| 可变参数类模板 | 递归继承；`std::tuple` 的实现基础 |

> 下一章：[05-05 编译期计算与 type_traits](05-05_type_traits.md)
