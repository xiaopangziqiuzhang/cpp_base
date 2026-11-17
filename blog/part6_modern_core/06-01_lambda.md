# 06-01 lambda 表达式全解

> 标准：lambda `[C++11]`；泛型 lambda、初始化捕获 `[C++14]`；
> `constexpr` lambda、`[*this]` 捕获 `[C++17]`
> demo：[`06_01_lambda.cpp`](../../demos/part6_modern_core/06_01_lambda.cpp)

## 一、解决了什么问题

STL 算法需要传入"行为"。C++98 下这意味着写一个**函数对象类**：

```cpp
// C++98：想用 count_if 做自定义判断，必须写一个类
class GreaterThan {
public:
    explicit GreaterThan(int threshold) : _threshold(threshold) {}
    bool operator()(int value) const { return value > _threshold; }
private:
    int _threshold;
};

std::count_if(v.begin(), v.end(), GreaterThan(3));
```

C++11 可以就地写：

```cpp
int threshold = 3;
std::count_if(v.begin(), v.end(), [threshold](int x) { return x > threshold; });
```

实测两者结果相同（都是 3），但差别是结构性的：

1. **逻辑写在使用的地方** —— 读代码的人不用跳到别处去看一个类
2. **不需要手工写构造函数来保存状态** —— 捕获列表自动完成
3. **不污染命名空间** —— 匿名类型，不需要起名字
4. **可以捕获局部变量** —— 函数对象类需要手工传进去并存为成员

这个改变让 STL 算法从"理论上好用"变成"实际上好用"。
C++98 时代很多人宁愿手写循环，也不愿为一个判断条件写一个类。

C++98 也有 `std::bind1st`、`std::not1` 这类函数适配器试图解决这个问题，
但它们的组合能力有限、语法晦涩，最终被 lambda 彻底取代。

## 二、完整语法

```cpp
[捕获列表](参数列表) 修饰符 -> 返回类型 { 函数体 }
```

各部分都可以省略：

```cpp
auto f1 = [] { return 42; };                        // 最简形式（连参数列表都省了）
auto f2 = [](int x) { return x * 2; };              // 带参数
auto f3 = [](int x) -> double { return x / 2.0; };  // 显式返回类型
auto f4 = []() noexcept { return 1; };              // noexcept
auto f5 = [](int x) mutable { return ++x; };        // mutable
```

返回类型通常可以省略（由 `return` 推导）。
需要显式写的情况：多个 `return` 返回不同类型、
需要返回引用、需要返回与推导结果不同的类型。

## 三、本质：编译器生成的类

lambda 不是函数，而是**一个匿名类的对象**。编译器大致做这样的转换：

```cpp
int x = 10;
std::string s = "hello";
auto lam = [x, s](int y) { return x + y + static_cast<int>(s.size()); };

// 编译器生成的等价类
class __lambda_123 {
public:
    __lambda_123(int x, std::string s) : _x(x), _s(std::move(s)) {}
    int operator()(int y) const { return _x + y + static_cast<int>(_s.size()); }
private:
    int _x;
    std::string _s;        // 捕获的变量成为数据成员
};
```

实测验证两者行为一致（都返回 20），而且 `sizeof` 符合预期：

```
      sizeof(lambda)  = 40（捕获的 int + string）
      sizeof(无捕获)  = 1（空类，1 字节）
```

无捕获的 lambda 是空类，大小为 1（见 [03-01](../part3_class/03-01_class_basics.md)）。

### 每个 lambda 都是独立的类型

```cpp
auto a = [] {};
auto b = [] {};
std::is_same_v<decltype(a), decltype(b)>      // false！
```

实测确认是 0。两个写法完全相同的空 lambda 也是**不同的类型**。

推论：

- **不能用 lambda 的类型声明变量**，只能用 `auto` 或 `decltype`
- 想存放"任意可调用对象"要用 `std::function`（见 [06-02](06-02_callable.md)）
- 两个 lambda 不能互相赋值（即使看起来一样）

一个有用的性质：**无捕获的 lambda 可以隐式转换成函数指针**：

```cpp
int (*fp)(int) = [](int x) { return x * 2; };     // ok，无捕获才行
```

这让 lambda 能传给只接受函数指针的 C API。

## 四、捕获方式

```cpp
int a = 1, b = 2, c = 3;

[a, b]      // 按值捕获 a 和 b
[&a, &b]    // 按引用捕获
[=]         // 全部按值（用到的变量）
[&]         // 全部按引用
[=, &a]     // 默认按值，a 按引用
[&, b]      // 默认按引用，b 按值
[this]      // 捕获 this 指针
[*this]     // [C++17] 按值捕获整个 *this 对象
[x = expr]  // [C++14] 初始化捕获
[]          // 不捕获
```

实测各种组合：

```
      按值捕获: 3
      按引用捕获: 102（a 被改成 100）
      全部按值 [=]: 6
      全部按引用 [&]: 7（a 变成 2）
      混合 [=,&a]: 4（a 变成 2）
      混合 [&,b]: 4（a 变成 2）
```

注意 `[=]` 和 `[&]` 只捕获**实际用到的**变量，不是作用域里所有变量。

### 捕获时机：创建 lambda 时就拷贝

```cpp
int x = 1;
auto by_value = [x] { return x; };      // 这一刻拷贝了 x = 1
auto by_ref   = [&x] { return x; };

x = 100;                                 // 之后修改
```

实测：

```
        按值捕获的 lambda 返回: 1（还是创建时的值）
        按引用捕获的 lambda 返回: 100（跟着变了）
```

这个区别很重要 —— 按值捕获是**快照**，按引用捕获是**绑定**。

## 五、mutable：让 lambda 有可变状态

默认情况下，lambda 的 `operator()` 是 **const** 的，
所以不能修改按值捕获的副本：

```cpp
int counter = 0;
// auto bad = [counter] { return ++counter; };     // 编译错误
auto gen = [counter]() mutable { return ++counter; };
```

实测：

```
      调用三次: 1 2 3
      外部的 counter 仍然是: 0
```

`mutable` 让 lambda 成了一个**有状态的小对象** ——
每次调用都修改自己内部的副本，外部变量不受影响。

用途：计数器、简单的状态机、生成器。

注意 `mutable` 的位置是在参数列表之后：`[](int x) mutable { }`。
而且即使没有参数，也要写空的 `()`：`[]() mutable { }`。

## 六、最大的陷阱：悬垂引用

```cpp
std::function<int()> make_dangling() {
    int local = 42;
    return [&local] { return local; };      // local 在函数返回后销毁！
}
auto f = make_dangling();
f();        // 未定义行为
```

按值捕获就安全了：

```cpp
return [local] { return local; };            // 拷贝了 local 的值
```

实测按值版本在函数外仍正常返回 42。

**规则：lambda 的生命周期可能超出当前作用域时，必须按值捕获。**

典型的危险场景：

```cpp
// 1. 存起来的回调
std::vector<std::function<void()>> callbacks;
for (int i = 0; i < 3; ++i) {
    callbacks.push_back([&i] { use(i); });     // i 的引用，循环结束就悬垂
}

// 2. 异步任务
std::thread t([&local_data] { process(local_data); });   // 可能先于 thread 销毁

// 3. 返回 lambda 的函数
auto make() { int x = 1; return [&x] { return x; }; }    // 必错
```

`[&]` 的危险在于它**静默地**捕获所有用到的局部变量 ——
看一眼捕获列表是 `[&]`，你不知道它到底捕获了什么。
所以很多代码规范要求**显式列出捕获的变量**，不用 `[=]` 和 `[&]`。

## 七、捕获 this 的坑

```cpp
class Widget {
    int _value;
public:
    std::function<int()> get_bad() {
        return [this] { return _value; };      // 捕获的是 this 【指针】
    }
    std::function<int()> get_good() {
        return [*this] { return _value; };     // [C++17] 拷贝整个对象
    }
};
```

关键点：**`[=]` 和 `[this]` 捕获的都是 `this` 指针，不是对象**。

```cpp
std::function<int()> bad, good;
{
    Widget w(42);
    bad = w.get_bad();
    good = w.get_good();
}   // w 销毁
good();     // 安全
bad();      // 未定义行为！this 已悬垂
```

实测：

```
      对象存活时: bad=42 good=42
      对象销毁后: good=42（安全，捕获了副本）
```

这是异步代码里非常常见的 bug —— 在成员函数里注册一个回调，
对象销毁后回调才被触发。

三种解法：

```cpp
// 1. [C++17] 按值捕获整个对象
return [*this] { return _value; };

// 2. 只捕获需要的成员（C++14 初始化捕获）
return [value = _value] { return value; };

// 3. 延长对象寿命（配合 enable_shared_from_this，见 02-05）
auto self = shared_from_this();
return [self] { return self->_value; };
```

第 3 种是异步框架的标准做法。

另外注意 C++20 起 `[=]` 隐式捕获 `this` 已被**废弃**，
要写成 `[=, this]` 明确意图。

## 八、泛型 lambda `[C++14]`

C++11 的 lambda 参数必须写死类型。C++14 允许用 `auto`：

```cpp
auto add_any = [](auto a, auto b) { return a + b; };

add_any(1, 2);                               // 3
add_any(1.5, 2.5);                            // 4.0
add_any(std::string("ab"), "cd");             // "abcd"
```

实测三种类型都正确。

它等价于一个带**模板 `operator()`** 的类：

```cpp
class __lambda {
public:
    template <class T1, class T2>
    auto operator()(T1 a, T2 b) const { return a + b; }
};
```

配合完美转发可以写出通用的包装器：

```cpp
auto forwarder = [](auto&& f, auto&&... args) {
    return std::forward<decltype(f)>(f)(std::forward<decltype(args)>(args)...);
};
```

注意这里用 `decltype(f)` 来获得 `f` 的类型 ——
泛型 lambda 的参数类型没有名字，只能这样拿到。
（C++20 允许显式模板参数列表：`[]<class T>(T x) {}`，就不需要 `decltype` 了。）

实际用途：

```cpp
// 适配任意类型的容器
std::for_each(m.begin(), m.end(), [](const auto& pair) {
    std::cout << pair.first << "=" << pair.second;
});

// 配合 std::visit 处理 variant（见 07-04）
std::visit([](const auto& value) { print(value); }, my_variant);
```

## 九、初始化捕获 `[C++14]`

也叫"广义捕获"，语法是 `[名字 = 表达式]`。它解决三个问题。

### 用途一：捕获只能移动的对象

```cpp
auto ptr = std::make_unique<int>(42);
// auto bad = [ptr] { return *ptr; };            // 编译错误：unique_ptr 不可拷贝
auto good = [p = std::move(ptr)] { return *p; };  // 移动进 lambda
```

实测可行，而且原 `ptr` 变成了空。

这是 C++14 之前无法做到的事 ——
C++11 的捕获只能拷贝或引用，没有移动。
把 `unique_ptr`、`thread`、大对象移进 lambda 都需要这个特性。

### 用途二：捕获表达式的结果

```cpp
int x = 5;
auto f = [y = x * 2] { return y; };      // 捕获 10，而不是捕获 x
```

实测返回 10。这避免了每次调用都重新计算。

### 用途三：改名或改变捕获方式

```cpp
auto g = [&r = x] { return ++r; };       // 按引用捕获但改名为 r
```

实测确实修改了 `x`。

常见用法是在捕获时就做转换：

```cpp
[data = std::vector<int>(1000)] { /* ... */ }        // 直接在 lambda 里构造
[key = make_key(args)] { /* ... */ }                  // 捕获计算结果
```

## 十、constexpr lambda `[C++17]`

C++17 起，**满足条件的 lambda 的 `operator()` 隐式就是 `constexpr`**：

```cpp
auto square = [](int x) { return x * x; };
static_assert(square(5) == 25, "编译期求值");

constexpr int arr_size = square(3);
int arr[arr_size]{};                      // 可以用作数组大小
```

实测通过，数组大小是 9。

也可以显式标记（用于确认意图，不满足条件时报错）：

```cpp
auto cube = [](int x) constexpr { return x * x * x; };
```

这让 lambda 能用在编译期上下文里 —— 配合 `if constexpr`
和编译期算法，表达力增强了不少。

## 十一、实际用途

### 配合 STL 算法（最主要的用途）

```cpp
std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
std::count_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
std::for_each(v.begin(), v.end(), [&sum](int x) { sum += x; });
std::remove_if(v.begin(), v.end(), [](const auto& x) { return x.expired(); });
```

### 立即调用的 lambda（IIFE）

用于复杂初始化，特别是初始化 `const` 对象：

```cpp
const auto config = [] {
    std::vector<std::string> result;
    // 多步构造逻辑
    return result;
}();      // 注意末尾的 ()
```

实测可行。这个模式的价值是：**让复杂初始化的对象仍然能是 `const`**。
没有它就得先声明非 const 对象、填充、再想办法保证不被改。

### 自定义删除器

```cpp
auto closer = [](std::FILE* f) { if (f) std::fclose(f); };
std::unique_ptr<std::FILE, decltype(closer)> fp(std::fopen(path, "r"), closer);
```

见 [02-05](../part2_memory/02-05_smart_pointers.md)。

### 作为 scope guard

```cpp
auto guard = make_guard([&] { cleanup(); });
```

见 [02-06](../part2_memory/02-06_raii.md)。

## 十二、小结

| 特性 | 标准 | 要点 |
|---|---|---|
| 基本 lambda | C++11 | `[捕获](参数) 修饰符 -> 返回类型 { 体 }` |
| 本质 | | 编译器生成的匿名类；每个 lambda 是独立类型 |
| 无捕获的性质 | | 可隐式转成函数指针；`sizeof` 为 1 |
| `[=]` / `[&]` | | 只捕获实际用到的变量；**不推荐**，应显式列出 |
| 捕获时机 | | 按值捕获是创建时的**快照**，按引用是**绑定** |
| `mutable` | | 让 `operator()` 非 const，可修改捕获的副本 |
| **悬垂引用** | | lambda 寿命超出作用域时必须按值捕获 |
| `[this]` 的坑 | | 捕获的是**指针**；对象销毁后悬垂 |
| `[*this]` | C++17 | 按值捕获整个对象，解决上面的问题 |
| 泛型 lambda | C++14 | `auto` 参数，等价于模板 `operator()` |
| 初始化捕获 | C++14 | `[p = std::move(ptr)]`；支持移动、捕获表达式、改名 |
| `constexpr` lambda | C++17 | 满足条件时隐式 constexpr |
| IIFE | | `[]{ ... }()` 用于复杂初始化，让对象能是 const |

> 下一章：[06-02 可调用对象统一视图](06-02_callable.md)
