# 01-08 异常与异常安全

> 标准：`try/catch/throw` C++98、`noexcept` `[C++11]`
> demo：[`01_08_exception.cpp`](../../demos/part1_basics/01_08_exception.cpp)

## 一、解决了什么问题

C 报告错误只有三种办法：返回错误码、设置全局 `errno`、通过出参返回。
三者都有同一个根本缺陷 —— **错误可以被静默忽略**。

```c
int divide(double x, double y, double* out) {
    if (y == 0) return -1;
    *out = x / y;
    return 0;
}

double result = 0;
divide(10, 0, &result);     /* 返回值没人检查，result 还是旧值 */
use(result);                /* 继续用错的数据 */
```

实测输出：

```
  错误码风格: 10/2 = 5
  忽略错误码后 result = 5（用了未更新的值）
```

程序带着错误状态继续跑，问题在很远的地方才暴露出来。

错误码还有三个次要问题：

1. **返回值被占用** —— 函数真正的结果只能从出参返回，接口变丑
2. **构造函数无法报错** —— 构造函数没有返回值，错误码风格下只能加个
   `bool is_valid()`，然后祈祷调用方会查
3. **错误要逐层手工传递** —— 深层调用链上，每一层都要检查并转发错误码，
   业务逻辑被淹没在错误处理里

异常机制解决的就是这些：**错误无法被忽略**（不处理就终止程序）、
**返回值回归业务语义**、**构造函数也能报错**、
**错误自动沿调用栈传播**到有能力处理它的地方。

```cpp
double divide(double x, double y) {
    if (y == 0) throw std::invalid_argument("division by zero");
    return x / y;          // 返回值就是结果，不用来表达错误
}
```

## 二、三个关键字

C++ 异常处理涉及三个关键字：`throw`、`try`、`catch`。

- **throw**：当问题出现时抛出一个异常
- **try**：标识可能抛出异常的代码块，后面跟一个或多个 catch 块
- **catch**：在你想处理问题的地方捕获异常

### throw 表达式

```cpp
throw 表达式;
```

异常是一个表达式，其值的类型**可以是基本类型，也可以是类**：

```cpp
throw 42;                              // 能编译，但不推荐
throw std::string("a string");         // 能编译，但不推荐
throw std::runtime_error("推荐做法");   // 推荐
```

虽然语法上可以抛任何类型，实践中应该只抛**继承自 `std::exception` 的类型**，
理由见第四节。

### try-catch 语句块

```cpp
try {
    // 语句块
} catch (异常类型) {
    // 具体的异常处理
} catch (异常类型) {
    // ...
}
```

`catch` 可以有多个，但至少要有一个。执行过程是：

- 执行 try 块中的语句。如果没有异常抛出，执行完后跳到**最后一个 catch 块之后**
  继续执行，所有 catch 块都不会被执行
- 如果 try 块中抛出了异常，立即跳转到**第一个类型匹配**的 catch 块执行
  （称为异常被该 catch 块"捕获"），执行完后跳到最后一个 catch 块之后继续执行

### catch 的匹配规则：从上到下，第一个匹配的胜出

这一点很重要：catch 不做"最佳匹配"，而是**按书写顺序找第一个可行的**。
所以派生类必须放在基类之前：

```cpp
try {
    throw std::out_of_range("越界了");
}
catch (const std::out_of_range& e) { /* 命中这里 */ }
catch (const std::logic_error& e)  { /* 基类，到不了 */ }
catch (const std::exception& e)    { /* 更基的类，更到不了 */ }
catch (...)                        { /* 兜底 */ }
```

如果把 `catch (const std::exception&)` 写在最前面，后面所有 catch 都成了死代码
（GCC 会用 `-Woverloaded-virtual` 之外的警告提示，但不保证）。

`catch (...)` 捕获任何类型，只能放在最后，用于兜底。
它拿不到异常对象，只能配合 `throw;`（重新抛出）做清理工作。

### 按什么形式捕获

```cpp
catch (std::exception e)         // 错：按值捕获，会切片（丢掉派生类信息）+ 多一次拷贝
catch (const std::exception& e)  // 对：按 const 引用捕获
```

**一律按 const 引用捕获**。按值捕获会发生对象切片：
抛出的是 `out_of_range`，按 `std::exception` 的值捕获后，只剩基类部分。

## 三、栈展开：异常安全的核心机制

抛出异常后，控制流离开当前作用域去找 catch。在这个过程中，
**所有已完整构造的局部对象都会被逆序析构**，这叫**栈展开（stack unwinding）**。

```cpp
void inner_scope() {
    Tracer t1("inner-1");
    Tracer t2("inner-2");
    throw std::runtime_error("from inner_scope");
}

void stack_unwinding() {
    Tracer t0("outer");
    try { inner_scope(); }
    catch (const std::exception& e) { /* 处理 */ }
}
```

实测输出：

```
    构造 outer
    构造 inner-1
    构造 inner-2
    析构 inner-2        <- 栈展开，逆序析构
    析构 inner-1
    捕获: from inner_scope
    析构 outer          <- 这个是函数正常返回时的常规作用域退出
```

构造顺序 `outer → inner-1 → inner-2`，析构顺序 `inner-2 → inner-1`，严格逆序。

**栈展开是 C++ 异常机制能够安全的全部基础**：它保证了只要资源由对象管理，
异常传播时资源就一定会被释放。这直接推出了 RAII。

## 四、标准异常体系

```
std::exception
 ├── logic_error      (程序逻辑错误，原则上可以在运行前预防)
 │    ├── invalid_argument    参数无效
 │    ├── domain_error        定义域错误
 │    ├── length_error        长度超限
 │    └── out_of_range        下标越界
 ├── runtime_error    (只能在运行时检测到的错误)
 │    ├── range_error         值域错误
 │    ├── overflow_error      上溢
 │    └── underflow_error     下溢
 ├── bad_alloc        new 失败
 ├── bad_cast         dynamic_cast 引用版失败
 └── bad_typeid       typeid 作用于空指针
```

`logic_error` 和 `runtime_error` 的区分是设计意图上的：
前者表示"这是个 bug，调用方本该避免"（如传了负数当长度），
后者表示"运行环境导致的，无法事先预防"（如文件读不到、网络断开）。

所有这些都提供 `what()` 返回描述字符串。实测 `vector::at` 越界：

```cpp
std::vector<int> v{1, 2, 3};
v.at(10);
// 抛出: vector::_M_range_check: __n (which is 10) >= this->size() (which is 3)
```

libstdc++ 的消息还带上了具体数值，调试时很有用。

### 自定义异常：继承标准异常

```cpp
class ConfigError : public std::runtime_error {
public:
    ConfigError(const std::string& key, const std::string& reason)
        : std::runtime_error("config key '" + key + "': " + reason)
        , _key(key) {}
    const std::string& key() const { return _key; }    // 携带结构化信息
private:
    std::string _key;
};
```

继承标准异常的好处：调用方可以用 `catch (const std::exception&)` 兜底处理，
不需要认识你的每一个异常类型。同时你可以携带额外的结构化信息
（这里是 `key()`），让需要精细处理的调用方用得上。

要点：**异常类的拷贝构造函数不应该抛异常**。
异常对象在传播过程中可能被拷贝，这时候再抛异常就没救了。
所以异常类里尽量只放已构造好的 `std::string`，不要在拷贝时做分配。

## 五、异常安全与 RAII

### 为什么裸指针在异常面前必然泄漏

```cpp
void leaky() {
    int* raw = new int[100];
    may_throw();          // 如果这里抛异常……
    delete[] raw;         // 这行永远执行不到 -> 泄漏
}
```

想修补，就得在每个可能抛异常的地方包 try-catch：

```cpp
void still_ugly() {
    int* raw = new int[100];
    try {
        may_throw();
    } catch (...) {
        delete[] raw;     // 每个 catch 里都要写一遍
        throw;            // 重新抛出
    }
    delete[] raw;         // 正常路径也要写一遍
}
```

函数一长、资源一多，这种写法必然会漏。

### RAII：让栈展开替你清理

**RAII（Resource Acquisition Is Initialization）**：
把资源的生命周期绑定到对象的生命周期 —— 构造函数获取资源，析构函数释放资源。
既然栈展开保证局部对象一定被析构，资源就一定被释放。

```cpp
void clean() {
    auto guard = std::make_unique<int[]>(100);
    may_throw();        // 抛异常时，栈展开自动调用 guard 的析构函数
    // 不需要写任何清理代码
}
```

实测输出：

```
  裸指针：必须在每个出口手工 delete
  raii_function 抛出 "中途抛异常"，内存已被 unique_ptr 自动释放
```

这条原则适用于所有资源，不只是内存：文件句柄（`std::fstream`）、
锁（`std::lock_guard`）、socket、数据库连接。
详见 [02-05 智能指针](../part2_memory/02-05_smart_pointers.md)
和 [02-06 RAII](../part2_memory/02-06_raii.md)。

### 三级异常安全保证

这是评价一个函数在异常面前表现的标准词汇（由 Abrahams 提出）：

| 级别 | 承诺 | 例子 |
|---|---|---|
| **不抛保证**（nothrow） | 绝不抛异常 | `swap`、析构函数、`vector::size()` |
| **强保证**（strong） | 要么成功，要么状态完全不变（事务性） | `vector::push_back` |
| **基本保证**（basic） | 不泄漏资源，对象保持有效状态，但状态可能已改变 | 大多数操作的最低要求 |

低于基本保证就是"异常不安全"。

实现强保证的标准技巧是 **copy-and-swap**：

```cpp
void Widget::update(const Data& d) {
    Widget tmp(*this);      // 在副本上操作，抛异常也只影响 tmp
    tmp.do_update(d);
    swap(*this, tmp);       // swap 不抛异常，这一步是原子的
}
```

## 六、noexcept `[C++11]`

C++98 有个 `throw(...)` 异常规范，设计失败，C++11 废弃、C++17 移除。
取而代之的是 `noexcept`：

```cpp
int never_throws() noexcept;    // 承诺不抛异常
int may_throw();                // 可能抛
```

实测：

```
  noexcept(never_throws()) = 1
  noexcept(may_throw())    = 0
```

`noexcept` 既是**说明符**（声明函数不抛），也是**运算符**（查询表达式是否可能抛）。

### 违反承诺的后果

标记 `noexcept` 的函数如果真的抛了异常，不是传播出去，而是直接调用
`std::terminate` 终止程序。这不是"检查"，是"保证"—— 所以别乱标。

### noexcept 的实际价值：让编译器和标准库优化

这是 `noexcept` 最重要的用途，不是文档，而是性能。

**编译器**：知道函数不抛异常，就不需要生成栈展开所需的异常处理表，代码更小更快。

**标准库**：`std::vector` 扩容时要把元素搬到新内存。如果元素的移动构造函数是
`noexcept`，它就用移动；否则只能用拷贝 —— 因为搬到一半抛异常就无法回滚了
（破坏强保证）。

```cpp
std::is_nothrow_move_constructible<std::string>::value   // 实测 true
```

所以**移动构造函数和移动赋值运算符应该尽量标记 `noexcept`**，
否则你的类放进 `vector` 时扩容会退化成拷贝。
详见 [02-03 移动语义](../part2_memory/02-03_move_semantics.md)。

### 条件 noexcept

模板里无法预知类型参数是否抛异常，可以写条件表达式：

```cpp
template <class T>
void do_swap(T& a, T& b) noexcept(std::is_nothrow_move_constructible<T>::value) {
    // ...
}
```

### 该标 noexcept 的地方

- **析构函数**（默认已经是 `noexcept`）
- **移动构造 / 移动赋值**（对容器性能影响大）
- `swap`
- 简单的 getter、不分配内存的小函数

不确定时就不标 —— 把 `noexcept` 加到一个会抛的函数上，比不加更糟。

## 七、析构函数绝不能抛异常

```cpp
class Bad {
public:
    ~Bad() noexcept(false) { throw std::runtime_error("boom"); }
};
```

问题出在栈展开期间：已经有一个异常在传播了，这时析构函数又抛出第二个，
C++ 没有"同时处理两个异常"的机制，运行时直接调用 `std::terminate`。

C++11 起，**析构函数默认是 `noexcept`** 的，想抛必须显式写 `noexcept(false)` ——
语言在用默认值阻止你这么做。

如果析构函数里的操作确实可能失败（比如关闭文件时 flush 失败），处理方式是：

```cpp
~FileWriter() {
    try {
        flush();
    } catch (...) {
        // 记日志，或者什么都不做，但绝不让异常逃出析构函数
    }
}
```

同时提供一个显式的 `close()` 供调用方在需要处理错误时使用 ——
这就是 `std::fstream` 既有析构函数又有 `close()` 的原因。

## 八、什么时候不该用异常

异常不是万能的，下面这些场合通常不用：

- **可预期的、频繁发生的情况** —— 比如"查找不到"。用 `optional`
  （见 [07-04](../part7_stdlib/07-04_tuple_variant.md)）或返回迭代器
- **性能关键路径上的高频错误** —— 异常抛出的开销很大（通常是微秒级），
  但**不抛时开销接近零**（这叫 zero-cost exception）
- **禁用异常的环境** —— 嵌入式、游戏引擎、内核模块常用 `-fno-exceptions`
- **跨 ABI 边界** —— 异常不能穿过 C 接口，动态库边界也有风险

经验法则：**异常用于"异常情况"**，即调用方在正常流程下不期望发生的事。
文件打不开是异常（如果你认为它应该存在），查字典没找到不是异常。

## 九、小结

| 要点 | 说明 |
|---|---|
| 解决什么 | 错误码可被忽略、占用返回值、构造函数无法报错、需逐层手工传递 |
| 三个关键字 | `throw` 抛出、`try` 标识、`catch` 捕获 |
| catch 顺序 | 从上到下第一个匹配胜出，派生类必须在基类之前 |
| 捕获形式 | 一律 `const T&`，按值会切片 |
| 栈展开 | 异常传播时逆序析构所有已构造的局部对象 |
| 标准异常 | 继承 `std::exception`；`logic_error` vs `runtime_error` |
| RAII | 资源绑定对象生命周期，靠栈展开自动释放 |
| 三级保证 | 不抛 > 强（事务性）> 基本（不泄漏）|
| `noexcept` `[C++11]` | 承诺不抛；违反则 `terminate`；让容器用移动而非拷贝 |
| 析构函数 | 默认 `noexcept`，绝不能让异常逃出 |

> 下一章：[01-09 字面量与语法糖](01-09_literals.md)
