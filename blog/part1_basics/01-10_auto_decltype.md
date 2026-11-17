# 01-10 类型推导：auto 与 decltype

> 标准：`auto` `decltype` 尾置返回类型 `[C++11]`，`decltype(auto)` 返回类型推导 `[C++14]`
> demo：[`demos/part1_basics/01_10_auto_basic.cpp`](../../demos/part1_basics/01_10_auto_basic.cpp)、
> [`demos/part1_basics/01_10_decltype.cpp`](../../demos/part1_basics/01_10_decltype.cpp)

## 一、这两个关键字解决了什么问题

C++98 时代，变量的类型必须由程序员完整写出来。在模板和 STL 普及之前，这不算负担；
但当类型由库推导出来、嵌套了好几层之后，类型名会迅速膨胀到比业务逻辑还长。

```cpp
std::map<std::string, std::vector<int>> scores;

// C++98：遍历一个 map 需要先写出 26 个字符的迭代器类型
for (std::map<std::string, std::vector<int>>::const_iterator it = scores.begin();
     it != scores.end(); ++it) { /* ... */ }
```

这里有三个层次的问题，`auto` 和 `decltype` 分别解决其中一部分：

1. **写不下** —— 类型名冗长但其实是确定的，编译器完全知道它是什么。→ `auto`
2. **写不对** —— 比如漏写 `const`，导致多一次隐式转换和一次拷贝；
   或者把 `std::map<K,V>` 的元素误写成 `std::pair<K,V>`（实际是 `std::pair<const K,V>`）。→ `auto`
3. **写不出** —— 在模板里，返回类型取决于参数类型，而参数在返回类型之后才声明。
   这在 C++98 里**根本没有办法表达**。→ `decltype` + 尾置返回类型

第 3 点是质的差别，前两点是量的差别。很多人把 `auto` 当作"少打字"的工具，
其实它更重要的身份是"让类型由编译器唯一确定"，从而消除一类手写类型带来的错误。

## 二、auto 的推导规则

`auto` 使用的是**函数模板实参推导**的规则。把 `auto x = expr;` 想象成
`template <class T> void f(T x); f(expr);`，`T` 推出什么，`auto` 就是什么。

记住一条就够了：**`auto` 单独使用时，会丢弃引用和顶层 const**。

```cpp
int x = 10;
const int  cx = x;
const int& rx = x;

auto  a1 = cx;  // int        —— 顶层 const 被丢弃
auto  a2 = rx;  // int        —— 引用被丢弃，const 随之丢弃
auto& a3 = cx;  // const int& —— 写了 & 就保留 const
auto&& a4 = x;  // int&       —— 万能引用，绑定左值得到左值引用
```

为什么丢弃是合理的？因为 `a1` 是一个**新对象**，它是 `cx` 的副本。
副本自己能不能被修改，和源对象是不是常量没有关系。而一旦写成 `auto&`，
`a3` 就不再是副本而是别名了，这时候丢掉 `const` 就会破坏 `cx` 的只读承诺，所以必须保留。

`a4` 的 `auto&&` 是**万能引用**（forwarding reference），它的推导涉及引用折叠，
是移动语义和完美转发的基础，详见 [02-04 完美转发](../part2_memory/02-04_perfect_forwarding.md)。

指针的情况：

```cpp
int v = 42;
int* p = &v;

auto       p1 = p;  // int*
auto*      p2 = p;  // int*，等价，但意图更清晰
const auto* p3 = p; // const int*
```

`auto p1 = p` 和 `auto* p2 = p` 推出同样的类型。推荐写 `auto*`：
读代码的人不必回头去看 `p` 的声明，就知道这是个指针。

## 三、三个必须知道的陷阱

### 陷阱一：`{}` 初始化

```cpp
auto n1 = 1;        // int
auto n2{1};         // int
auto n3 = {1};      // std::initializer_list<int>
auto n4 = {1,2,3};  // std::initializer_list<int>
```

规则是：**带 `=` 的花括号（拷贝初始化）推出 `initializer_list`，
不带 `=` 的直接初始化推出元素类型**。

这里有段历史：C++11/14 的标准文本原本规定 `auto n2{1}` 也是
`initializer_list<int>`，后来由决议 N3922 改为 `int`。GCC 和 Clang 把该决议
追溯应用到了 `-std=c++11`，所以在本机实测中，c++11/14/17 三种模式下 `n2` 都是 `int`：

```
n1=1 n2=1 n3.size=1 n4.size=3
```

实践上的建议：需要 `initializer_list` 时就显式写出类型，别依赖这条规则。

### 陷阱二：代理对象

`std::vector<bool>` 不是真正的 `vector`，它为了省内存按位存储，
`operator[]` 返回的不是 `bool&` 而是一个代理对象：

```cpp
std::vector<bool> flags{true, false, true};

auto proxy = flags[0];  // std::vector<bool>::reference，不是 bool！
bool real  = flags[0];  // 这才是 bool

flags[0] = false;       // 改了底层的那个 bit
// proxy 仍然指向那个 bit，于是它的值也跟着变了
```

实测输出：

```
proxy=0 real=1
```

`real` 保住了修改前的值 `1`，`proxy` 却变成了 `0`。写 `auto` 的时候你以为拿到了一个值，
实际拿到的是一个引用语义的代理。遇到返回代理对象的库（`vector<bool>`、
部分矩阵库的 `operator()`、表达式模板），要么显式写类型，要么用 `static_cast` 强制落地：

```cpp
auto real2 = static_cast<bool>(flags[0]);
```

### 陷阱三：丢引用导致的意外拷贝

这是实际项目里最常见、也最容易造成性能问题的一个：

```cpp
struct Heavy {
    std::vector<int> data = std::vector<int>(1000, 7);
    int id = 0;
};
std::vector<Heavy> items(3);

for (auto item : items) {  // 每轮拷贝 1000 个 int！
    item.id = 1;           // 改的是副本，原对象没变
}
```

实测输出证实了修改确实丢了：

```
after copy-loop, items[0].id = 0
after ref-loop,  items[0].id = 1
```

范围 for 的三种写法，按用途选：

| 写法 | 语义 | 用途 |
|---|---|---|
| `for (auto x : c)` | 拷贝 | 元素是基本类型，或确实需要副本 |
| `for (auto& x : c)` | 可写引用 | 要修改容器元素 |
| `for (const auto& x : c)` | 只读引用 | 只读访问，默认首选 |

## 四、decltype：从表达式取出类型

`decltype(expr)` 推导表达式 `expr` 的类型，**不求值**，并且**完整保留引用和 const**。
这正是它和 `auto` 的分工：`auto` 用来声明新变量（要值语义），
`decltype` 用来复刻已有表达式的类型（要精确）。

```cpp
int x = 0;
const int& rx = x;

auto         a = rx;      // int
decltype(rx) d = x;       // const int&
```

一个容易考到的细节 —— **加不加括号结果不同**：

```cpp
struct Point { int x = 1; int y = 2; };
Point p;

decltype(p.x)   v1 = 10;   // int   —— 成员访问，取的是声明类型
decltype((p.x)) v2 = p.x;  // int&  —— 多一层括号就变成左值表达式，取左值引用
v2 = 99;                   // 于是改到了 p.x
```

规则：`decltype(e)` 中，若 `e` 是**未加括号的标识符或类成员访问**，结果是其声明类型；
否则按表达式的值类别决定 —— 左值得 `T&`，将亡值得 `T&&`，纯右值得 `T`。
多写的那一对括号把标识符变成了普通表达式，于是走第二条规则。

## 五、尾置返回类型：C++98 做不到的事

回到本章开头的第 3 个问题。写一个泛型的 `add`，返回类型应该是 `t + u` 的类型：

```cpp
// C++98：无法表达。返回类型位于参数列表之前，此处 t 和 u 还不在作用域内
template <class T, class U>
??? add(T t, U u) { return t + u; }
```

C++11 给出的答案是**尾置返回类型**：用 `auto` 占住返回类型的位置，
把真正的返回类型挪到参数列表之后，这时 `t`、`u` 已经可见：

```cpp
template <class T, class U>
auto add(T t, U u) -> decltype(t + u) {
    return t + u;
}
```

C++14 进一步允许省略，由函数体中的 `return` 语句推导：

```cpp
template <class T, class U>
auto add(T t, U u) {      // [C++14] 返回类型自动推导
    return t + u;
}
```

两者都能正确处理混合类型：

```
int+double = 3.5
string+char* = hello,world
```

需要注意，C++14 的 `auto` 返回类型要求函数定义**对调用点可见**（因为要推导），
所以它不适合放在只有声明的头文件接口里；而尾置返回类型可以照常分离声明与定义。

## 六、decltype(auto)：补上 auto 丢引用的缺口 `[C++14]`

`auto` 作返回类型时，同样遵循"丢弃引用和 const"的规则，这会悄悄把引用返回变成值返回：

```cpp
std::vector<int> g_data{1, 2, 3};

auto           get_by_auto()     { return g_data[0]; } // int，返回副本
decltype(auto) get_by_decltype() { return g_data[0]; } // int&，返回引用

get_by_decltype() = 42;   // 能写回原容器
// get_by_auto() = 42;    // 编译错误：给右值赋值
```

`decltype(auto)` 的含义是"用 `decltype` 的规则去做 `auto` 的推导"，
于是引用和 const 都被保留。它最典型的用途是写**转发函数**：
你并不知道被包装的函数返回值还是引用，但你希望原样传出去。

```cpp
template <class Container, class Index>
decltype(auto) element_at(Container&& c, Index i) {
    return std::forward<Container>(c)[i];
}

std::vector<std::string> v{"a", "b"};
element_at(v, 0) = "modified";  // 返回的是引用，所以能赋值
```

这里如果把返回类型写成 `auto`，`element_at(v, 0) = "modified"` 会编译失败 ——
`operator[]` 的 `std::string&` 被降级成了 `std::string` 副本。

## 七、什么时候该用、什么时候不该用

**推荐用 `auto`：**
- 迭代器、lambda 对象、模板中间结果等类型冗长或无法书写的场合
- 范围 for 中（配 `const auto&` / `auto&`）
- 用 `auto x = T{...}` 的形式初始化，保证类型只出现一次，避免隐式转换

**不要用 `auto`：**
- 类型是接口契约的一部分时（公开函数的返回类型、结构体成员）
- 可能遇到代理对象时
- 读者需要知道具体类型才能理解代码时。`auto result = compute();` 对维护者毫无信息量

**用 `decltype` / `decltype(auto)`：**
- 需要复刻某个表达式的精确类型（含引用、const）
- 写泛型转发层、返回类型依赖参数的模板

## 八、小结

| 工具 | 推导规则 | 典型用途 |
|---|---|---|
| `auto` | 模板实参推导，**丢弃**引用和顶层 const | 声明局部变量、范围 for |
| `auto&` / `auto&&` | 保留 const；`&&` 触发引用折叠 | 绑定已有对象、完美转发 |
| `decltype(e)` | **保留**引用与 const；标识符/成员访问取声明类型，其余按值类别 | 复刻类型、尾置返回 |
| `auto f() -> decltype(...)` | 尾置返回 `[C++11]` | 返回类型依赖参数类型 |
| `auto f()` | 由 return 推导 `[C++14]` | 简短的模板函数 |
| `decltype(auto) f()` | 按 decltype 规则推导 `[C++14]` | 转发函数，需保留引用 |

> 下一章：[01-11 统一初始化](01-11_uniform_init.md)
