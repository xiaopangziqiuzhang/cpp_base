# 02-05 智能指针

> 标准：`unique_ptr`/`shared_ptr`/`weak_ptr` `[C++11]`、`make_unique` `[C++14]`
> demo：[`02_05_smart_pointers.cpp`](../../demos/part2_memory/02_05_smart_pointers.cpp)

## 一、解决了什么问题

裸指针有三个独立的问题。

**问题一：忘记释放。** 这个最直观，也最容易被低估 ——
函数一长、分支一多，总会有路径漏掉 `delete`。

**问题二：异常面前手工 delete 不可靠。**

```cpp
void f() {
    Res* p = new Res;
    may_throw();        // 抛异常 -> 下面这行永远不执行
    delete p;
}
```

[01-08](../part1_basics/01-08_exception.md) 讲过，想修补就得在每个出口包 try-catch，
代码会变得没法维护。

**问题三：所有权不清。** 这个最严重，因为它是**接口层面**的缺陷：

```cpp
Res* get_resource();     // 调用方该不该 delete 返回值？签名没说
void use(Res* p);        // 这个函数会接管 p 吗？还是只是看一眼？
```

裸指针 `T*` 同时表达了太多含义：可能是所有者、可能是借用者、可能是可选参数、
可能是数组首地址。文档说了算，而文档会过时。

智能指针解决的核心问题是**把所有权写进类型**：

| 类型 | 含义 |
|---|---|
| `std::unique_ptr<T>` | 我独占这个对象，我负责释放 |
| `std::shared_ptr<T>` | 我和别人共享，最后一个走的人负责释放 |
| `std::weak_ptr<T>` | 我只观察，不持有，用之前要检查 |
| `T*` / `T&` | 我只借用，不负责释放 |

看到签名就知道责任归谁，不需要读文档。

## 二、unique_ptr：独占所有权

默认选择。零开销 —— 和裸指针一样大、一样快（在 `-O2` 下完全没有额外指令）。

```cpp
{
    auto p = std::make_unique<Res>("unique-1");
    p->use();
}   // 离开作用域自动释放
```

实测：

```
      + 获取 unique-1
      . 使用 unique-1
      (离开作用域)
      - 释放 unique-1
```

### 不可拷贝，只可移动

这是"独占"的语言层面保障：

```cpp
auto p1 = std::make_unique<Res>("x");
// auto p2 = p1;              // 编译错误：拷贝构造被删除
auto p2 = std::move(p1);      // ok：所有权转移，p1 变成空
```

实测移动后 `p1 == nullptr` 为真。

### 常用操作

```cpp
p.get();        // 返回裸指针，借用，不转移所有权
p.reset();      // 立即释放
p.reset(q);     // 释放旧的，接管 q
p.release();    // 放弃所有权，返回裸指针（之后要自己 delete）
p.swap(q);
if (p) ...      // 转 bool，判断是否非空
*p, p->member   // 像裸指针一样用
```

`release()` 和 `reset()` 容易记混：

- `reset()` —— **释放**对象（调析构函数）
- `release()` —— **放弃**所有权但不释放对象，返回裸指针，责任转移给你

### 用签名表达所有权转移

这是 `unique_ptr` 最有价值的用法：

```cpp
std::unique_ptr<Res> factory(const std::string& name);   // 返回：所有权归调用方
void takes_ownership(std::unique_ptr<Res> p);            // 按值接收：接管所有权
void borrows(const Res& r);                              // 引用：只借用
void borrows_maybe_null(const Res* p);                   // 裸指针：借用，可为空
```

```cpp
auto p = factory("owned");
borrows(*p);                      // p 仍持有
takes_ownership(std::move(p));    // 转移，之后 p 为空
```

调用方必须写 `std::move` 才能转移 —— **所有权转移在调用点可见**，这是设计上的优点。

### 管理数组

```cpp
auto arr = std::make_unique<int[]>(5);     // 特化版本，自动用 delete[]
arr[0] = 1;
```

`unique_ptr<T[]>` 有专门的特化：提供 `operator[]`、不提供 `*` 和 `->`、
析构时调用 `delete[]`。这解决了 [02-02](02-02_new_delete.md)
提到的 `delete`/`delete[]` 配对问题。

但实践中**优先用 `std::vector`** —— 它还提供大小信息、迭代器、动态扩容。
`unique_ptr<T[]>` 只在对接 C API 或确实需要固定大小的裸缓冲区时才用。

## 三、shared_ptr：共享所有权

当"谁最后用完"无法在编译期确定时，用引用计数。

```cpp
auto p1 = std::make_shared<Res>("shared-1");
std::cout << p1.use_count();      // 1
{
    auto p2 = p1;                 // 拷贝 -> 计数 +1
    std::cout << p1.use_count();  // 2
}                                 // p2 销毁 -> 计数 -1
std::cout << p1.use_count();      // 1
```

实测：

```
      创建后 use_count = 1
      拷贝后 use_count = 2
      p2 销毁后 use_count = 1
      (p1 离开作用域，计数归零才真正释放)
      - 释放 shared-1
```

### 它的开销不是零

`shared_ptr` 比 `unique_ptr` 贵，要知道贵在哪：

- **大小是裸指针的两倍** —— 一个指向对象，一个指向控制块
- **控制块**额外占内存，包含强引用计数、弱引用计数、删除器、分配器
- **引用计数的增减是原子操作** —— 线程安全的代价，比普通整数加减慢得多
- 拷贝 `shared_ptr` 不是免费的

所以默认用 `unique_ptr`，只在**真的需要共享**时才用 `shared_ptr`。
不要因为"shared 听起来更安全"就到处用。

### make_shared 比 new 好

```cpp
std::shared_ptr<Res> a(new Res("x"));      // 两次分配
auto b = std::make_shared<Res>("x");       // 一次分配
```

两个理由：

**一、一次分配。** `make_shared` 把对象和控制块放在同一块内存里，
省一次堆分配（回想 [02-01](02-01_memory_layout.md)：堆分配比栈分配慢 26 倍）。

**二、异常安全。** 考虑这个调用：

```cpp
f(std::shared_ptr<Res>(new Res), g());
```

C++17 之前，参数的求值顺序是未指定的。可能的顺序是：
`new Res` → `g()` → 构造 `shared_ptr`。如果 `g()` 抛异常，
那块 `new` 出来的内存就泄漏了 —— 它还没来得及被 `shared_ptr` 接管。
`make_shared` 是一个原子的操作，没有这个窗口。

`make_shared` 的一个**缺点**：对象和控制块在同一块内存，
所以只要还有 `weak_ptr` 存在，对象的内存就不能释放（控制块要保留）。
管理大对象且有长期 `weak_ptr` 观察时，用 `shared_ptr<T>(new T)` 反而更省内存。

## 四、循环引用：shared_ptr 的致命问题

引用计数无法处理环。

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> prev;      // 两个都是 shared_ptr
};

{
    auto a = std::make_shared<Node>();
    auto b = std::make_shared<Node>();
    a->next = b;
    b->prev = a;        // 形成环
}   // 离开作用域
```

离开作用域时：`a` 的局部变量销毁，但 `b->prev` 还指着它，计数从 2 变 1 不归零；
`b` 同理。**两个对象互相锁住，都不会被释放。**

实测输出（注意完全没有析构函数的输出）：

```
      + NodeBad A
      + NodeBad B
      a.use_count=2 b.use_count=2
      (离开作用域，但析构函数不会被调用！)
      ^ 注意上面没有任何 "- NodeBad" 输出，这就是泄漏
```

### 验证这真的是泄漏

我用 AddressSanitizer 确认过。有个细节值得记下来：

直接跑 ASan **检测不到**这个泄漏。原因是 LeakSanitizer 靠扫描"根"
（栈、寄存器、全局变量）里的指针来判断可达性，
而函数返回后栈上仍残留着指向那两个对象的旧指针值 ——
LSan 把它们当成了活跃引用。

在中间插入一个深栈帧把残留数据覆盖掉，泄漏就报出来了：

```
==3781030==ERROR: LeakSanitizer: detected memory leaks
Indirect leak of 56 byte(s) in 1 object(s) allocated from:
    ... std::make_shared<Node, int>(int&&) ...
Indirect leak of 56 byte(s) in 1 object(s) allocated from:
    ...
```

共 112 字节（2 × 56）。

这个现象有实际意义：**不要因为 ASan 没报泄漏就认为没有循环引用**。
检查循环引用要靠设计审查，或者在析构函数里打日志确认。

### 解法：反向指针用 weak_ptr

```cpp
struct Node {
    std::shared_ptr<Node> next;     // 持有下一个
    std::weak_ptr<Node> prev;       // 弱引用上一个，不增加计数
};
```

实测：

```
      + NodeGood A
      + NodeGood B
      a.use_count=1 b.use_count=2
      (离开作用域)
      - NodeGood A
      - NodeGood B
```

注意 `a.use_count()` 是 1 而不是 2 —— `weak_ptr` 不计入强引用。
于是环被打破，对象正常释放。

**判断谁该用 weak_ptr 的原则：想清楚"谁拥有谁"。**
父节点拥有子节点（`shared_ptr`），子节点只是知道父节点（`weak_ptr`）；
观察者不拥有被观察对象（`weak_ptr`）；缓存不拥有缓存项（`weak_ptr`）。

## 五、weak_ptr：观察而不持有

`weak_ptr` 不能直接使用，必须先 `lock()` 提升为 `shared_ptr`：

```cpp
std::weak_ptr<Res> weak = strong;

if (auto locked = weak.lock()) {      // 返回 shared_ptr，失败则为空
    locked->use();                     // 在 locked 的作用域内，对象保证存活
} else {
    // 对象已经被销毁
}
```

实测：

```
      对象存活时 expired=0
      lock() 成功，期间 use_count=2
      (strong 离开作用域)
      - 释放 observed
      对象销毁后 expired=1
      lock() 返回空，安全地知道对象已不在
```

`lock()` 的设计很讲究：它返回 `shared_ptr` 而不是裸指针，
这样在你使用期间对象一定不会被别的线程销毁。
这是**检查与使用之间没有竞态**的保证。

注意 `expired()` 只能用于诊断，**不要**写成：

```cpp
if (!weak.expired()) {       // 检查
    weak.lock()->use();      // 使用 —— 这中间对象可能已经没了（多线程）
}
```

正确的写法总是 `if (auto p = weak.lock())`。

### 典型用途

- 打破循环引用（上一节）
- 观察者模式：观察者列表持有 `weak_ptr`，不妨碍被观察对象销毁
- 缓存：缓存项用 `weak_ptr`，对象没人用了就自动失效
- `shared_from_this` 的实现基础

## 六、自定义删除器

智能指针不只能管内存，任何"获取-释放"配对的资源都能管。

```cpp
// 管理 FILE*
auto closer = [](std::FILE* f) { if (f) std::fclose(f); };
std::unique_ptr<std::FILE, decltype(closer)> fp(std::fopen(path, "w"), closer);
std::fputs("data", fp.get());
// 离开作用域自动 fclose

// 管理 malloc 的内存
std::unique_ptr<void, decltype(&std::free)> mem(std::malloc(64), &std::free);

// shared_ptr 的删除器不进类型参数，更灵活
std::shared_ptr<Res> p(new Res("x"), [](Res* r) { custom_cleanup(r); delete r; });
```

实测确认都正常触发：

```
    管理 FILE*:
      写入成功
      文件已关闭
    shared_ptr 的删除器不进类型，更灵活:
      自定义删除器被调用
```

两者的重要区别：

- `unique_ptr<T, Deleter>` 的删除器是**类型的一部分** ——
  零开销，但类型签名变复杂，不同删除器的 `unique_ptr` 类型不同
- `shared_ptr<T>` 的删除器存在控制块里，**不进类型** ——
  更灵活（可以把不同删除器的 `shared_ptr` 放进同一个容器），但有间接开销

这个机制让智能指针成为通用的 RAII 包装器：socket、数据库连接、
OpenGL 句柄、Windows HANDLE、互斥量，全都可以这样管。

## 七、enable_shared_from_this

有时需要在成员函数里获得指向自己的 `shared_ptr`：

```cpp
class Widget {
public:
    std::shared_ptr<Widget> self() {
        return std::shared_ptr<Widget>(this);   // 错误！会创建第二个独立的控制块
    }                                            // 导致 double free
};
```

正确做法是继承 `std::enable_shared_from_this`：

```cpp
class Widget : public std::enable_shared_from_this<Widget> {
public:
    std::shared_ptr<Widget> self() { return shared_from_this(); }
};

auto p = std::make_shared<Widget>();
auto q = p->self();        // use_count 变成 2，共享同一个控制块
```

实测 `use_count = 2`，确认共享了控制块。

两个注意点：

1. **对象必须已经由 `shared_ptr` 管理**。否则 `shared_from_this()`
   抛 `std::bad_weak_ptr`（C++17 起；C++11 下是未定义行为）
2. **不能在构造函数里调用** —— 那时 `shared_ptr` 还没建立。
   需要的话用工厂函数模式：构造函数私有，提供 `static create()` 返回 `shared_ptr`

典型用途：异步回调里需要延长自己的生命周期：

```cpp
void Session::start_read() {
    auto self = shared_from_this();      // 捕获自己，保证回调期间不被销毁
    socket.async_read([self](auto ec, auto n) { self->on_read(ec, n); });
}
```

## 八、选择指南

```
需要动态分配一个对象？
├── 不需要 -> 用栈对象 / std::vector / std::string（最好的选择）
└── 需要
    ├── 单一所有者 -> unique_ptr                     ← 默认
    ├── 确实需要共享所有权 -> shared_ptr
    └── 只观察，不拥有 -> weak_ptr（配合 shared_ptr）

只是传递一个"借用"的对象？
├── 必须非空 -> T&
└── 可以为空 -> T*（裸指针，这是它的正当用途）
```

几条实践规则：

- **优先考虑不用动态分配** —— `vector`、`string`、栈对象能解决大部分问题
- 需要时**默认 `unique_ptr`**，它零开销，而且以后要改成 `shared_ptr` 很容易（反之很难）
- 用 `make_unique` / `make_shared`，不要 `new`
- **裸指针仍然有正当用途**：表达"借用且可为空"。不要把所有 `T*` 都改成智能指针
- 函数参数用 `const T&` 或 `T*`，不要用 `const shared_ptr<T>&`
  —— 后者暴露了不必要的所有权细节
- `unique_ptr` 可以隐式转成 `shared_ptr`（单向），所以工厂函数返回 `unique_ptr`
  最灵活

## 九、小结

| 类型 | 所有权 | 开销 | 可拷贝 |
|---|---|---|---|
| `unique_ptr<T>` | 独占 | 零（和裸指针一样）| 否，只可移动 |
| `shared_ptr<T>` | 共享（引用计数）| 两倍大小 + 控制块 + 原子操作 | 是 |
| `weak_ptr<T>` | 无（观察）| 同 shared_ptr | 是 |
| `T*` | 无（借用）| 零 | 是 |

要点：

- 智能指针真正解决的是**所有权表达**，自动释放只是副产品
- `unique_ptr` 不可拷贝只可移动，用 `std::move` 显式转移，调用点可见
- `shared_ptr` 的引用计数是原子操作，不是免费的
- `make_shared` 省一次分配且异常安全；但有 `weak_ptr` 时会延迟内存释放
- **循环引用会泄漏**，反向指针用 `weak_ptr`；ASan 可能检测不到这类泄漏
- `weak_ptr` 必须 `lock()` 后使用，不要先 `expired()` 再 `lock()`
- 自定义删除器让智能指针成为通用 RAII 包装器
- `shared_from_this` 要求对象已被 `shared_ptr` 管理，不能在构造函数里用

> 下一章：[02-06 RAII 与资源管理惯用法](02-06_raii.md)
