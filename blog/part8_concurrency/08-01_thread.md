# 08-01 线程

> 标准：`[C++11]`
> demo：[`08_01_thread.cpp`](../../demos/part8_concurrency/08_01_thread.cpp)

## 一、解决了什么问题

C++11 之前，标准里**没有线程** —— 连"多线程程序的行为"都没有定义。
写多线程 C++ 只能用平台 API：

```c
pthread_create(&tid, nullptr, func, arg);     /* POSIX */
CreateThread(nullptr, 0, func, arg, 0, &tid); /* Windows */
```

三个问题：

1. **不可移植** —— 每个平台一套 API
2. **接口是 C 风格的** —— 只能传 `void*`，没有类型安全
3. **没有内存模型** —— 标准没规定"一个线程的写入何时对另一个线程可见"，
   编译器优化和多线程的交互是未定义的

第 3 点最根本。没有内存模型，`volatile` 和各种"双检锁"的写法
都建立在对编译器行为的猜测上（见 [03-07](../part3_class/03-07_singleton.md)
的双检锁讨论）。

C++11 一次补齐了三样东西：**线程库**、**同步设施**、**内存模型**。
本章讲线程库，内存模型见 [08-04](08-04_atomic.md)。

## 二、创建线程

`std::thread` 的构造函数接受任何可调用对象（见
[06-02](../part6_modern_core/06-02_callable.md) 的 INVOKE 语义）：

```cpp
// 普通函数
std::thread t1(free_function, 1);

// 函数对象
std::thread t2(Functor{}, 2);

// lambda（最常用）
std::thread t3([](int id) { /* ... */ }, 3);

// 成员函数：第二个参数是对象（或指针）
Worker w;
std::thread t4(&Worker::member_function, &w, 4);
```

实测四种都正常工作。

### hardware_concurrency

```cpp
std::thread::hardware_concurrency();      // 实测本机 128
```

返回硬件支持的并发线程数（通常是逻辑核心数）。
它只是个**提示** —— 可能返回 0（表示无法确定）。

实践上线程池的大小通常取这个值，但要注意：

- CPU 密集型任务：约等于核心数
- IO 密集型任务：可以远大于核心数
- 容器环境：这个值可能是宿主机的核数，而不是容器的 CPU 配额

## 三、join 与 detach

每个 `std::thread` 对象在析构前**必须**调用 `join()` 或 `detach()` 之一。

```cpp
t.join();        // 等待线程结束
t.detach();      // 分离线程，不再关心它
t.joinable();    // 是否还需要 join/detach
```

实测：

```
        join 之前 joinable = 1
        join 之后 joinable = 0
```

### 关键规则：析构时仍 joinable 会 terminate

```cpp
void bad() {
    std::thread t([] { /* ... */ });
    // 忘了 join 或 detach
}   // 析构时 joinable() 为 true -> std::terminate()
```

为什么是 `terminate` 而不是自动 join 或 detach？
标准委员会的考虑是：**两种默认行为都可能是错的**，
所以干脆要求程序员明确选择。

- 自动 join → 析构函数可能阻塞很久，而且在异常传播时阻塞很危险
- 自动 detach → 线程可能访问已销毁的局部变量

### detach 的风险

```cpp
void dangerous() {
    int local = 1;
    std::thread t(use_value, std::ref(local));
    t.detach();
}   // local 销毁，但分离的线程可能还在用它
```

**detach 后线程的生命周期不受控制** ——
它可能在 `main` 返回后还在运行（这时全局对象已开始析构，极其危险）。

实践建议：**几乎总是用 `join`，避免 `detach`**。
需要"后台任务"时，用线程池或 `std::async`
（见 [08-05](08-05_async.md)）。

## 四、传参陷阱

### 陷阱一：thread 默认拷贝参数

```cpp
void takes_ref(int& x) { x = 100; }

int value = 1;
// std::thread t(takes_ref, value);        // 编译错误！
std::thread t(takes_ref, std::ref(value));  // 必须用 std::ref
```

实测用 `std::ref` 后 `value` 变成 100。

原因：`std::thread` 把参数**拷贝**到自己的内部存储，
然后以**右值**的形式传给函数。而 `int&` 不能绑定右值。

这个设计是有意的 —— 线程可能比调用方活得久，
默认拷贝避免了悬垂引用。想传引用就得显式写 `std::ref`，等于签字确认
（见 [06-02](../part6_modern_core/06-02_callable.md)）。

### 陷阱二：隐式转换发生在新线程里

```cpp
void takes_cref(const std::string& s);

void risky() {
    char buf[100];
    std::sprintf(buf, "...");
    std::thread t(takes_cref, buf);     // 危险！
    t.detach();
}   // buf 销毁，但到 std::string 的转换可能还没发生
```

`std::thread` 保存的是 `char*` 的拷贝，
**到 `std::string` 的转换在新线程中进行** ——
如果那时 `buf` 已经销毁，就是悬垂指针。

解法：**在调用方先显式转换**：

```cpp
std::thread t(takes_cref, std::string(buf));     // 安全
```

### 陷阱三：只能移动的对象

```cpp
auto ptr = std::make_unique<int>(42);

// 方式一：初始化捕获（C++14，见 06-01）
std::thread t([p = std::move(ptr)] { use(*p); });

// 方式二：直接 move 进参数
std::thread t2(use_ptr, std::move(ptr));
```

实测方式一正常工作。

## 五、RAII 包装：thread_guard

既然必须手工 join，就应该用 RAII 保证它一定被调用
（见 [02-06](../part2_memory/02-06_raii.md)）：

```cpp
class ThreadGuard {
public:
    explicit ThreadGuard(std::thread t) : _t(std::move(t)) {}
    ~ThreadGuard() { if (_t.joinable()) _t.join(); }

    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
private:
    std::thread _t;
};

{
    ThreadGuard g(std::thread([] { /* ... */ }));
    // 中途抛异常也没关系
}   // 析构自动 join
```

实测确认析构时正确 join。

**C++20 的 `std::jthread` 内置了这个行为**（而且支持协作式取消）：

```cpp
// C++20
{
    std::jthread t([] { /* ... */ });     // 析构自动 join
}
```

C++17 下自己写一个 `ThreadGuard` 是标准做法。

## 六、数据竞争

这是多线程最根本的问题。

```cpp
int counter = 0;        // 共享变量，无保护

// 8 个线程各自 ++ 100000 次
for (int i = 0; i < 8; ++i) {
    threads.emplace_back([&counter] {
        for (int j = 0; j < 100000; ++j) ++counter;
    });
}
```

实测结果：

```
      8 线程各自 ++ 100000 次
        期望值:       800000
        无保护的结果: 201666  <- 丢失了更新！
        atomic 的结果: 800000
```

**丢失了约 75% 的更新。**

### 为什么

`++counter` **不是原子操作**，它是三步：

```
1. 读取 counter 的值到寄存器
2. 寄存器 +1
3. 写回 counter
```

两个线程同时执行时：

```
线程 A: 读到 100
线程 B: 读到 100        <- A 还没写回
线程 A: 写回 101
线程 B: 写回 101        <- 丢失了一次更新
```

### 用 ThreadSanitizer 检测

数据竞争的危险在于它**不一定出错** ——
可能在测试环境跑得好好的，上线后在不同的调度下崩溃。

ThreadSanitizer 能可靠地检测它：

```bash
$ g++ -std=c++17 -fsanitize=thread -g race.cpp -o race -pthread
$ ./race
WARNING: ThreadSanitizer: data race (pid=4094076)
SUMMARY: ThreadSanitizer: data race /tmp/race.cpp:7 in operator()
counter=23783 (期望 40000)
```

实测准确报出了竞争位置。**多线程代码应该常规性地用 TSan 跑测试。**

注意 TSan 和 ASan 不能同时用，而且会让程序慢 5-15 倍。

### 解决办法

```cpp
std::atomic<int> counter{0};        // 方式一：原子变量（见 08-04）
++counter;                           // 这是原子的

std::mutex mtx;                      // 方式二：互斥锁（见 08-02）
{
    std::lock_guard<std::mutex> lk(mtx);
    ++counter;
}
```

实测 `atomic` 版本得到正确的 800000。

对单个变量的简单操作用 `atomic`（更快），
对多个变量的复合操作用锁。

## 七、thread_local

```cpp
thread_local int tls_counter = 0;        // 每个线程一份
int shared_counter = 0;                   // 所有线程共享
```

实测：

```
        线程 0 的 tls_counter = 1000
        线程 1 的 tls_counter = 1000
        线程 2 的 tls_counter = 1000
      主线程的 tls_counter = 0（独立的一份）
```

每个线程（包括主线程）有自己独立的一份，**不需要同步**。

### 典型用途

```cpp
// 1. 线程私有的随机数引擎（见 07-05）
thread_local std::mt19937 gen(std::random_device{}());

// 2. 线程私有的缓存/缓冲区
thread_local std::vector<char> buffer(4096);

// 3. 错误码（类似 errno）
thread_local int last_error = 0;

// 4. 避免锁竞争的计数器（最后汇总）
thread_local long long local_count = 0;
```

### 注意事项

- **初始化时机**：首次在该线程中使用时（像函数内的 `static`）
- **析构时机**：线程结束时
- **不是零成本** —— 访问 `thread_local` 变量通常比普通变量慢
  （要查线程局部存储表）
- **动态库中的 thread_local** 在某些平台有坑（初始化顺序、析构时机）

## 八、线程的其他操作

```cpp
std::this_thread::get_id();                    // 当前线程 id
t.get_id();                                     // 某个线程的 id
std::this_thread::sleep_for(100ms);             // 睡一段时间
std::this_thread::sleep_until(time_point);      // 睡到某个时刻
std::this_thread::yield();                      // 主动让出 CPU
```

实测 `sleep_for(15ms)` 实际睡了 15 ms（有调度误差，通常是几百微秒）。

`sleep_for` 保证**至少**睡这么久，不保证精确 —— 它受调度器精度限制。
需要精确定时要用平台的高精度定时器。

### thread 不可拷贝，只可移动

```cpp
std::thread a([] {});
std::thread b = std::move(a);       // ok，所有权转移
// std::thread c = b;                // 编译错误
```

实测移动后 `a.joinable()` 为 0。

原因：`std::thread` 表示**唯一的执行资源**，
拷贝它意味着什么并不清楚（启动第二个线程？两个对象管同一个线程？）。
所以它是 move-only 类型（见 [02-03](../part2_memory/02-03_move_semantics.md)）。

推论：**把线程放进容器要用 `emplace_back` 或 `push_back(std::move(t))`**：

```cpp
std::vector<std::thread> threads;
threads.emplace_back(func, arg);                // 直接构造
threads.push_back(std::thread(func, arg));       // 移动进去
for (auto& t : threads) t.join();                 // 注意是 auto&
```

## 九、小结

| 要点 | 说明 |
|---|---|
| 解决什么 | C++11 之前标准没有线程，也**没有内存模型** |
| 创建 | 接受任何可调用对象；成员函数要传对象 |
| `hardware_concurrency` | 只是提示；容器环境可能返回宿主机核数 |
| **关键规则** | 析构前必须 `join()` 或 `detach()`，否则 **`std::terminate`** |
| 为什么不自动处理 | 两种默认行为都可能是错的，强制程序员明确选择 |
| `detach` 的风险 | 生命周期不可控，可能在 main 之后还在跑 → **尽量只用 join** |
| 传参陷阱一 | 默认**拷贝**参数；传引用要 `std::ref` |
| 传参陷阱二 | 隐式转换在**新线程**中进行 → 调用方先显式转换 |
| 传参陷阱三 | 只能移动的对象用初始化捕获或 `std::move` |
| RAII 包装 | 自己写 `ThreadGuard`；C++20 用 `std::jthread` |
| **数据竞争** | `++counter` 是读-改-写三步，实测丢失 75% 更新 |
| 检测工具 | **ThreadSanitizer**（`-fsanitize=thread`）能可靠报出竞争位置 |
| `thread_local` | 每线程一份，无需同步；典型用途是随机数引擎、缓冲区 |
| move-only | `thread` 不可拷贝；放容器要 `emplace_back` |

> 下一章：[08-02 互斥与锁](08-02_mutex.md)
