# 06-02 可调用对象统一视图

> 标准：`std::function`、`std::bind`、`std::ref` `[C++11]`；`std::invoke` `[C++17]`
> demo：[`06_02_callable.cpp`](../../demos/part6_modern_core/06_02_callable.cpp)

## 一、C++ 里有五种可调用对象

这是 C++ 的一个历史包袱：**"能被调用的东西"有五种，语法各不相同**。

```cpp
// 一、普通函数
int plain_function(int x) { return x * 2; }

// 二、函数指针
int (*fp)(int) = plain_function;

// 三、函数对象（仿函数）
struct Functor { int operator()(int x) const { return x * 3; } };

// 四、lambda（见 06-01，本质是匿名的函数对象）
auto lam = [](int x) { return x * 4; };

// 五、成员函数指针 / 成员变量指针
struct Widget {
    int value = 10;
    int triple() const { return value * 3; }
};
int (Widget::*mem_fn)() const = &Widget::triple;
int Widget::*mem_var = &Widget::value;
```

实测调用：

```
      1. 普通函数:       42
      2. 函数指针:       42
      3. 函数对象:       42
      4. lambda:         40
      5. 成员函数指针:   30
         成员变量指针:   10
```

前四种的调用语法是统一的 `f(args)`，但**成员指针完全不同**：

```cpp
(w.*mem_fn)()        // 通过对象
(pw->*mem_fn)()      // 通过指针
(w.*mem_var)         // 成员变量指针
```

注意那对括号不能省 —— `.*` 的优先级低于 `()`。

这种不一致是真问题。写泛型代码时，你不知道调用方会传哪一种：

```cpp
template <class F, class... Args>
auto call_it(F f, Args... args) {
    return f(args...);           // 对成员指针不工作！
}
```

C++11 加入 `std::function` 统一了**存储**，
C++17 加入 `std::invoke` 统一了**调用**。

## 二、std::function：统一的可调用对象容器

```cpp
std::function<int(int)> f;       // 签名：接受 int，返回 int

f = plain_function;              // 装普通函数
f = Functor{};                   // 装函数对象
f = [](int x) { return x + 3; }; // 装 lambda
```

实测全部可行。

### 它解决的核心问题：lambda 不能放进容器

每个 lambda 都是独立类型（见 [06-01](06-01_lambda.md)），
所以这样不行：

```cpp
// std::vector<???> handlers;     // lambda 的类型没法写出来
```

`std::function` 做了**类型擦除**，于是：

```cpp
std::vector<std::function<int(int)>> handlers;
handlers.push_back(plain_function);
handlers.push_back(Functor{});
handlers.push_back([](int x) { return x * 10; });
```

实测可以统一存储和遍历调用。

这是 `std::function` 最主要的价值 ——
回调注册表、事件系统、策略模式、命令模式都依赖它。

### 空状态与异常

```cpp
std::function<void()> empty;
static_cast<bool>(empty);        // false
empty();                          // 抛 std::bad_function_call
```

实测确认。所以调用前应该检查：

```cpp
if (f) f();
```

### 三、std::function 的代价

类型擦除不是免费的。我做了实测，结果比预期更有意思。

`-O0`（本书构建脚本的默认配置）：

```
        直接调用 lambda    : 7241 us
        通过函数指针       : 13351 us
        通过 std::function : 97203 us
```

`-O2`：

```
        直接调用 lambda    : 0 us
        通过函数指针       : 6683 us
        通过 std::function : 6675 us
```

两个发现：

**一、`-O2` 下直接调用的耗时变成 0** ——
编译器内联了 lambda，发现整个循环的结果可以直接算出来，于是把循环优化掉了。

**二、`-O2` 下函数指针和 `std::function` 的开销几乎相同**（6683 vs 6675 us）。

这说明：**`std::function` 的真正代价是"阻止内联"，而不是间接调用本身。**
函数指针同样阻止内联，所以两者相当。而 `-O0` 下的 13 倍差距
主要来自 `std::function` 内部的几层转发没有被优化掉。

### 其他代价

```cpp
sizeof(lambda)         // 1（无捕获）
sizeof(std::function)  // 32
```

- **可能堆分配**：捕获的状态超过内部缓冲区（libstdc++ 是 16 字节）时，
  `std::function` 会在堆上分配
- **拷贝开销**：拷贝 `std::function` 可能触发堆分配和被存对象的拷贝

### 结论

**不要默认用 `std::function`。** 它的适用场合是明确需要类型擦除时：

| 场景 | 用什么 |
|---|---|
| 存进容器、作为类成员 | `std::function` |
| 跨 ABI 边界的回调 | `std::function` 或函数指针 |
| 运行期可替换的策略 | `std::function` |
| **函数参数**（立即调用） | **模板参数** `template <class F> void f(F callback)` |
| 性能关键路径 | 模板参数，保留内联能力 |

```cpp
// 不好：强制类型擦除，阻止内联
void for_each_item(const std::vector<int>& v, std::function<void(int)> f);

// 更好：模板参数，可以内联
template <class F>
void for_each_item(const std::vector<int>& v, F f);
```

STL 算法全都用后者。

## 四、std::bind：绑定参数

```cpp
using namespace std::placeholders;

int add3(int a, int b, int c) { return a + b + c; }

auto bound = std::bind(add3, 1, 2, _1);        // 固定前两个参数
bound(3);                                       // 6

auto reordered = std::bind(add3, _3, _1, _2);  // 重排参数顺序
reordered(1, 2, 3);                             // 6

// 绑定成员函数
auto member_bound = std::bind(&Widget::add, &w, _1);
member_bound(5);
```

实测全部正常。

`_1`、`_2` 是**占位符**，表示"调用时的第 N 个参数填这里"。

### 现代建议：用 lambda 代替 bind

```cpp
auto bound = std::bind(add3, 1, 2, _1);         // bind 版本
auto lambda = [](int c) { return add3(1, 2, c); };   // lambda 版本
```

实测两者结果相同（都是 6），但 lambda 更好：

| | `std::bind` | lambda |
|---|---|---|
| 可读性 | 要理解占位符 | 就是普通代码 |
| 内联 | 难（通过 `std::function` 式的间接）| 容易 |
| 错误信息 | 极其糟糕（模板嵌套几十层）| 正常 |
| 灵活性 | 只能绑定参数 | 可以写任意逻辑 |
| 嵌套 | `bind(f, bind(g, _1))` 语义微妙 | 直接嵌套调用 |

`std::bind` 有几个真实的陷阱：

```cpp
// 陷阱一：默认拷贝参数（见下一节）
std::bind(increment, counter);        // 改不到 counter

// 陷阱二：嵌套 bind 的求值时机
std::bind(f, std::bind(g, _1));       // 内层 bind 的结果会被当成可调用对象调用

// 陷阱三：重载函数无法绑定
void h(int); void h(double);
// std::bind(h, _1);                  // 编译错误：不知道绑哪个重载
```

**C++11 之后，`std::bind` 基本只在维护老代码时才需要。**
Scott Meyers 在《Effective Modern C++》里明确建议"优先使用 lambda"。

## 五、std::ref / std::cref

### 问题：bind 和 thread 默认拷贝参数

```cpp
int counter = 0;
auto bad = std::bind(increment, counter);     // counter 被拷贝
bad(); bad();
// counter 仍然是 0
```

实测确认。`std::bind` 把参数**拷贝**进返回的对象里，
所以修改的是副本。

### 解法：std::ref 包装

```cpp
auto good = std::bind(increment, std::ref(counter));
good(); good();
// counter == 2
```

实测确认。`std::ref` 返回一个 `std::reference_wrapper<T>`，
它是个"看起来像引用的对象"——可以拷贝，但指向同一个目标。

`std::cref` 是 const 版本。

### 另一个用途：让引用能放进容器

```cpp
// std::vector<int&> refs;        // 非法：容器要求元素可赋值，引用不可重新绑定

int a = 1, b = 2, c = 3;
std::vector<std::reference_wrapper<int>> refs{a, b, c};
for (int& r : refs) r *= 10;
// a=10, b=20, c=30
```

实测确认修改成功。这是"把引用放进容器"的标准解法
（见 [01-03](../part1_basics/01-03_reference.md)）。

`std::reference_wrapper` 有隐式转换到 `T&`，
所以大部分时候用起来就像引用。

### std::thread 也需要它

```cpp
std::thread t(increment, std::ref(counter));    // 不加 ref 就改不到原变量
```

`std::thread` 同样拷贝参数（而且它必须这么做 ——
线程可能比调用方活得更久，持有引用很危险）。
详见 [08-01](../part8_concurrency/08-01_thread.md)。

## 六、std::invoke：统一的调用语法 `[C++17]`

回到本章开头的问题 —— 五种可调用对象的调用语法不统一。

```cpp
free_func(1)                 // 普通函数
(w.*&Widget::triple)()       // 成员函数：语法完全不同
(w.*&Widget::value)          // 成员变量
```

`std::invoke` 把它们统一了：

```cpp
std::invoke(free_func, 1);                 // 2
std::invoke(&Widget::triple, w);           // 126
std::invoke(&Widget::value, w);            // 42
std::invoke([](int x) { return x * 100; }, 1);   // 100
```

实测全部正确。

规则很简单：**第一个参数是可调用对象，如果它是成员指针，
第二个参数就是对象（或对象指针、或 `reference_wrapper`）**。

### 为什么重要

写泛型代码时不用再分情况处理：

```cpp
// 没有 invoke：要为成员指针写特殊分支
template <class F, class... Args>
auto call(F&& f, Args&&... args) {
    if constexpr (std::is_member_pointer_v<std::decay_t<F>>) {
        // 特殊处理...
    } else {
        return std::forward<F>(f)(std::forward<Args>(args)...);
    }
}

// 有 invoke：一行
template <class F, class... Args>
auto call(F&& f, Args&&... args) {
    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
}
```

标准库内部大量使用它。`std::thread`、`std::async`、
`std::function`、`std::bind` 的行为都用"INVOKE 操作"来定义 ——
这也是为什么 `std::thread(&Widget::method, &w)` 能工作。

### 配套的 is_invocable `[C++17]`

```cpp
std::is_invocable_v<decltype(free_func), int>          // true
std::is_invocable_v<decltype(free_func), std::string>  // false
std::is_invocable_r_v<int, F, Args...>                  // 还检查返回类型
```

实测分别是 1 和 0。这在模板约束里很有用：

```cpp
template <class F>
void register_callback(F f) {
    static_assert(std::is_invocable_v<F, const Event&>,
                  "回调必须接受 const Event&");
}
```

比 `enable_if` 的错误信息清楚得多（见 [05-05](../part5_template/05-05_type_traits.md)）。

## 七、实战：事件总线

把本章的工具组合起来：

```cpp
class EventBus {
public:
    using Handler = std::function<void(const std::string&)>;

    void subscribe(const std::string& event, Handler h) {
        _handlers[event].push_back(std::move(h));
    }

    void publish(const std::string& event, const std::string& data) const {
        auto it = _handlers.find(event);
        if (it == _handlers.end()) return;
        for (const auto& h : it->second) h(data);
    }

private:
    std::map<std::string, std::vector<Handler>> _handlers;
};
```

三种不同来源的回调统一注册：

```cpp
bus.subscribe("click", free_handler);                           // 普通函数
bus.subscribe("click", [](const std::string& d) { /* ... */ }); // lambda
bus.subscribe("click", [&logger](const std::string& d) {         // 成员函数
    logger.on_event(d);
});
bus.publish("click", "button-1");
```

实测三个回调都被调用：

```
        [free function] button-1
        [lambda] button-1
        [Logger::on_event] button-1
```

这里 `std::function` 是必需的 —— 要把不同类型的回调存进同一个容器。
注意绑定成员函数时用的是 lambda 而不是 `std::bind`，更清楚。

实际项目里还要考虑：

- **生命周期**：`[&logger]` 捕获引用，logger 必须比 bus 活得久。
  不确定时用 `weak_ptr`（见 [02-05](../part2_memory/02-05_smart_pointers.md)）
- **取消订阅**：需要返回一个句柄
- **线程安全**：`_handlers` 的并发访问要加锁（见 [08-02](../part8_concurrency/08-02_mutex.md)）

## 八、小结

| 工具 | 用途 |
|---|---|
| 五种可调用对象 | 普通函数、函数指针、函数对象、lambda、成员指针 |
| 成员指针的语法 | `(obj.*ptr)(args)`，括号不能省 |
| `std::function` | **类型擦除**：统一存储任意可调用对象；让 lambda 能进容器 |
| function 的代价 | 真正的代价是**阻止内联**（实测 -O2 下与函数指针相当）；可能堆分配 |
| function 的用法 | 存进容器/作成员时用；**函数参数优先用模板参数** |
| `std::bind` | 绑定参数、重排顺序；**现代代码优先用 lambda** |
| bind 的陷阱 | 默认拷贝参数、嵌套语义微妙、无法绑定重载函数 |
| `std::ref` / `cref` | 在 bind/thread 中传引用；让引用能放进容器 |
| `std::invoke` `[C++17]` | **统一调用语法**，泛型代码不用为成员指针写特殊分支 |
| `is_invocable_v` `[C++17]` | 检测可调用性，比 `enable_if` 的错误信息清楚 |

> 下一章：[06-03 类型别名与 using](06-03_using.md)
