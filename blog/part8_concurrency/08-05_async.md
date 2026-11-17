# 08-05 异步任务

> 标准：`[C++11]`；`std::apply`、`invoke_result_t` `[C++17]`
> demo：[`08_05_async.cpp`](../../demos/part8_concurrency/08_05_async.cpp)

## 一、解决了什么问题

`std::thread` 只能"启动一个函数"，**没有办法取回结果**。
用裸线程要自己处理三件事：

```cpp
int result = 0;
std::exception_ptr error;

std::thread t([&result, &error] {
    try {
        result = compute();
    } catch (...) {
        error = std::current_exception();      // 必须手工捕获并保存
    }
});
t.join();
if (error) std::rethrow_exception(error);       // 手工重新抛出
```

三个问题：

1. **结果存哪里** —— 共享变量 + 同步，或者出参
2. **怎么知道算完了** —— 条件变量或标志位
3. **异常怎么传回来** —— **`thread` 中未捕获的异常会调用 `std::terminate`**，
   必须手工捕获并通过 `exception_ptr` 传递

第 3 点最严重：裸线程里漏一个 try-catch，整个程序就会崩溃。

`std::future` 把这三件事都封装了。

## 二、std::async：最简单的异步

```cpp
auto fut = std::async(std::launch::async, compute, 7);
// 主线程可以继续做别的事
int result = fut.get();        // 阻塞等待并取回结果
```

实测并行效果：

```
      4 个任务（各 20ms）并行执行，总耗时 20 ms（串行要 80ms），结果和 = 30
```

四个 20ms 的任务并行执行，总耗时 20ms —— 确实并行了。

### future 的接口

```cpp
fut.get();                      // 阻塞等待并取结果（只能调用一次！）
fut.wait();                      // 只等待，不取结果
fut.wait_for(100ms);             // 带超时的等待
fut.wait_until(deadline);
fut.valid();                     // 是否还持有结果
```

`get()` **只能调用一次** —— 之后 `future` 变成无效状态。
需要多次读取结果时用 `std::shared_future`。

## 三、启动策略：必须显式指定

```cpp
std::async(std::launch::async, f);        // 保证在新线程中执行
std::async(std::launch::deferred, f);     // 延迟到 get() 时在【调用线程】执行
std::async(f);                             // 默认 = async|deferred，实现自己选
```

实测 `deferred` 的行为：

```
      主线程 id:        140090042976064
      deferred 的执行线程: 140090042976064（和主线程相同！）
      async 的执行线程:    140089946531392（不同线程）
```

`deferred` 根本不创建线程 —— 它把任务存起来，`get()` 时在调用线程执行。
本质上是"惰性求值"，不是并发。

### 默认策略的三个问题

1. **不知道会不会并行** —— 无法保证性能
2. **如果是 `deferred` 而你从不调 `get()`，任务永远不执行**
3. **`thread_local` 变量的行为不确定** ——
   不知道任务在哪个线程执行（见 [08-01](08-01_thread.md)）

还有一个更隐蔽的问题：如果用 `wait_for` 轮询一个 `deferred` 任务，
它永远返回 `std::future_status::deferred`，循环永不结束：

```cpp
auto fut = std::async(f);        // 可能是 deferred
while (fut.wait_for(0s) != std::future_status::ready) {   // 死循环！
    do_other_work();
}
```

**规则：总是显式写 `std::launch::async`。**

## 四、future 析构会阻塞（重要陷阱）

`std::async` 返回的 `future` 在**析构时会阻塞等待任务完成**。

实测：

```cpp
{
    auto fut = std::async(std::launch::async, [] {
        std::this_thread::sleep_for(50ms);
    });
    // 不调用 get()
}   // future 析构 -> 阻塞 50ms
```

```
        不调用 get() 直接离开作用域，耗时 50 ms（被 future 的析构函数阻塞了）
```

### 后果：这样写是串行的

```cpp
std::async(std::launch::async, f);     // 临时 future 立刻析构 -> 阻塞等 f 完成
std::async(std::launch::async, g);     // 所以 g 要等 f 完成才开始
```

两个任务**完全串行**。必须把 `future` 存起来：

```cpp
auto f1 = std::async(std::launch::async, f);
auto f2 = std::async(std::launch::async, g);      // 现在才并行
f1.get(); f2.get();
```

这个行为是 C++11 的一个有争议的设计决定 ——
它保证了任务不会在程序退出后还在跑，
但代价是"忘记保存返回值"会静默地变成串行。

注意只有 `std::async` 返回的 `future` 有这个行为。
`promise`/`packaged_task` 的 `future` 析构不阻塞。

## 五、异常的自动传递

```cpp
auto fut = std::async(std::launch::async, may_throw, true);
try {
    fut.get();
} catch (const std::runtime_error& e) {
    // 异常从任务线程传回来了
}
```

实测：

```
      异常通过 future.get() 传回: 任务内部出错
```

**这是 `future` 相对裸线程最大的优势**：

| | 裸 `thread` | `future` |
|---|---|---|
| 未捕获的异常 | **`std::terminate`**（程序崩溃）| 保存在 future 里 |
| 取回方式 | 手工 `exception_ptr` | `get()` 时自动重新抛出 |

机制：`future` 内部保存一个 `std::exception_ptr`，
`get()` 时如果有异常就 `rethrow_exception`。

## 六、promise / future 配对

`std::async` 适合"执行一个函数并取回返回值"。
但有时结果不是函数返回的，而是**在某个时刻被设置**：

```cpp
std::promise<std::string> prom;
std::future<std::string> fut = prom.get_future();

std::thread producer([&prom] {
    // 做一些事...
    prom.set_value("数据");          // 写端
});

fut.get();                            // 读端
```

实测正常工作。

`promise` 也能传异常：

```cpp
try {
    throw std::logic_error("错误");
} catch (...) {
    prom.set_exception(std::current_exception());
}
```

实测异常正确传回。

### 分工

- **`promise` 是写端**，`future` 是读端
- 一个 `promise` 对应一个 `future`（通过 `get_future()` 获取，只能调一次）

### 两个注意点

**一、`set_value` 只能调一次** —— 第二次会抛
`std::future_error`（`promise_already_satisfied`）。

**二、`promise` 析构时若未设置值，`future.get()` 会抛 `broken_promise`**：

```cpp
{
    std::promise<int> p;
    auto f = p.get_future();
    // 忘了 set_value
}   // p 析构
f.get();      // 抛 std::future_error: Broken promise
```

这个设计避免了"永久等待" —— 至少你会得到一个异常而不是卡死。

### 典型用途

```cpp
// 一次性的事件通知
std::promise<void> ready;
auto ready_fut = ready.get_future();
// 某处: ready.set_value();
ready_fut.wait();        // 等待事件

// 回调风格的 API 转成 future
void async_api(std::function<void(int)> callback);

std::future<int> to_future() {
    auto prom = std::make_shared<std::promise<int>>();
    auto fut = prom->get_future();
    async_api([prom](int result) { prom->set_value(result); });
    return fut;
}
```

最后这个模式很实用 —— 把回调风格的异步 API 包装成 `future`。

## 七、packaged_task

**`packaged_task` = 可调用对象 + future**：

```cpp
std::packaged_task<int(int, int)> task([](int a, int b) { return a + b; });
std::future<int> fut = task.get_future();

std::thread t(std::move(task), 20, 22);      // task 不可拷贝，只可移动
fut.get();                                     // 42
```

实测得到 42。

它和 `std::async` 的区别：**`packaged_task` 不决定在哪执行**。
你可以把它放进队列，由任意线程取出执行 ——
这正是**线程池的基础**。

```cpp
std::queue<std::packaged_task<void()>> task_queue;
// 生产者: task_queue.push(std::move(task));
// 消费者: auto t = std::move(queue.front()); queue.pop(); t();
```

## 八、完整的线程池实现

把前面的工具组合起来。

### 为什么需要线程池

`std::async` 每次调用可能创建一个新线程
（标准不要求它用线程池，libstdc++ 确实每次都新建）。
创建线程的开销在微秒级，对于大量小任务是不可接受的。

线程池预先创建固定数量的线程，任务排队执行。

### 实现

```cpp
class ThreadPool {
public:
    explicit ThreadPool(std::size_t threads = std::thread::hardware_concurrency()) {
        for (std::size_t i = 0; i < threads; ++i)
            _workers.emplace_back([this] { worker_loop(); });
    }

    ~ThreadPool() { shutdown(); }

    template <class F, class... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using Ret = std::invoke_result_t<F, Args...>;

        // 用 shared_ptr 包 packaged_task：它不可拷贝，而 std::function 要求可拷贝
        auto task = std::make_shared<std::packaged_task<Ret()>>(
            [f = std::forward<F>(f),
             tup = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                return std::apply(std::move(f), std::move(tup));    // [C++17]
            });

        std::future<Ret> result = task->get_future();
        {
            std::lock_guard<std::mutex> lk(_mtx);
            if (_stopped) throw std::runtime_error("线程池已关闭");
            _tasks.emplace([task] { (*task)(); });
        }
        _cv.notify_one();
        return result;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lk(_mtx);
            if (_stopped) return;
            _stopped = true;
        }
        _cv.notify_all();
        for (auto& w : _workers) if (w.joinable()) w.join();
    }

private:
    void worker_loop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lk(_mtx);
                _cv.wait(lk, [this] { return _stopped || !_tasks.empty(); });
                if (_stopped && _tasks.empty()) return;
                task = std::move(_tasks.front());
                _tasks.pop();
            }
            task();        // 在【锁外】执行任务
        }
    }

    std::vector<std::thread> _workers;
    std::queue<std::function<void()>> _tasks;
    mutable std::mutex _mtx;
    std::condition_variable _cv;
    bool _stopped = false;
};
```

### 实测

```
      线程池大小: 4
      8 个任务的结果总和: 1020001800000
      线程池中的异常传回: 任务抛异常
      返回 string 的任务: hello!
```

支持任意返回类型（包括 `void`）、异常正确传回。

我还用 ThreadSanitizer 压测了 3 轮（每轮 6 线程 × 2000 个任务）：

```
round 0: done=2000/2000 sum=3998000 expect=3998000 OK
round 1: done=2000/2000 sum=3998000 expect=3998000 OK
round 2: done=2000/2000 sum=3998000 expect=3998000 OK
```

**没有报出数据竞争**，计数和求和都正确。

### 五个实现要点

**一、`packaged_task` 统一返回类型**

任务的返回类型各不相同，但队列只能存一种类型。
`packaged_task` 把结果转移到 `future` 里，
于是队列只需要存 `std::function<void()>`。

**二、用 `shared_ptr` 包 `packaged_task`**

`packaged_task` 是 move-only 的，而 `std::function` 要求可拷贝
（见 [06-02](../part6_modern_core/06-02_callable.md)）。
用 `shared_ptr` 绕过这个限制。

（C++23 有了 `std::move_only_function` 就不需要这个技巧了。）

**三、用 `std::apply` 展开参数** `[C++17]`

要在 lambda 里保存不定个数的参数，先打包成 tuple，
调用时用 `std::apply` 展开（见
[07-04](../part7_stdlib/07-04_tuple_variant.md)）。
C++14 下要手写 `index_sequence`。

**四、任务在锁外执行**

```cpp
{
    std::unique_lock lk(_mtx);
    task = std::move(_tasks.front());
    _tasks.pop();
}        // 先解锁
task();  // 再执行
```

否则一个任务执行期间其他工作线程全被阻塞 —— 线程池就退化成单线程了。

**五、析构时 shutdown**

`shutdown()` 设置标志、`notify_all` 唤醒所有等待的工作线程、
然后 join 它们。这保证了 RAII（见
[02-06](../part2_memory/02-06_raii.md)）。

注意 `_cv.wait` 的谓词是 `_stopped || !_tasks.empty()`，
而退出条件是 `_stopped && _tasks.empty()` ——
这保证了**已提交的任务会被执行完**才退出。

### 这个实现的局限

真实的线程池还需要考虑：

- **任务窃取**（work stealing）—— 单个任务队列在高并发下是瓶颈
- **任务优先级**
- **动态调整线程数**
- **避免递归提交导致的死锁**（任务里提交任务并等待它）
- **异常安全** —— 工作线程里的异常会怎样（`packaged_task` 已经处理了）

生产环境建议用成熟实现：Intel TBB、folly、Asio 的 `thread_pool`、
或者 BS::thread_pool 这类单头文件库。

## 九、并行归约的实际例子

```cpp
constexpr std::size_t kN = 20000000;
std::vector<double> data(kN, 1.5);

// 串行
double serial = std::accumulate(data.begin(), data.end(), 0.0);

// 并行：分块 + async
const unsigned threads = 8;
const std::size_t chunk = kN / threads;
std::vector<std::future<double>> futures;
for (unsigned i = 0; i < threads; ++i) {
    auto begin = data.begin() + i * chunk;
    auto end = (i == threads - 1) ? data.end() : data.begin() + (i + 1) * chunk;
    futures.push_back(std::async(std::launch::async, [begin, end] {
        return std::accumulate(begin, end, 0.0);
    }));
}
double parallel = 0;
for (auto& f : futures) parallel += f.get();
```

实测：

```
      20000000 个 double 求和:
        串行:              139 ms
        8 线程并行: 18 ms
```

**快 7.7 倍**（8 线程，接近线性加速）。

对比 [07-03](../part7_stdlib/07-03_algorithms.md) 里的并行算法 ——
那里 `std::execution::par` 因为缺少 TBB 完全没有加速，
而这个手工实现确实并行了。这说明在 TBB 不可用的环境里，
手工分块 + `async` 是可靠的替代方案。

注意最后一行的提醒：**浮点加法不满足结合律**，
所以并行结果可能与串行略有差异。

## 十、怎么选

```
需要异步执行一个任务？
├── 一次性的、简单的          -> std::async(std::launch::async, f)
├── 大量小任务                -> 线程池
├── 结果在某个时刻被设置       -> promise / future
├── 需要把任务放进队列         -> packaged_task
└── 只是启动一个不关心结果的线程 -> std::thread（但记得 join）
```

| 工具 | 适合 |
|---|---|
| `std::async` | 少量、一次性的异步任务 |
| 线程池 | 大量小任务（避免反复创建线程）|
| `promise`/`future` | 把回调风格 API 转成 future；一次性事件通知 |
| `packaged_task` | 实现线程池、任务队列 |
| `std::thread` | 长期运行的工作线程 |

## 十一、小结

| 要点 | 说明 |
|---|---|
| 解决什么 | 裸 `thread` 无法取回结果；**未捕获的异常会 terminate** |
| `std::async` | 最简单的异步；返回 `future` |
| `future.get()` | 阻塞取结果，**只能调一次**；多次读用 `shared_future` |
| **启动策略** | **必须显式写 `std::launch::async`**；默认策略可能不并行 |
| `deferred` 的坑 | 不创建线程；不调 `get()` 就永不执行；`wait_for` 轮询会死循环 |
| **future 析构阻塞** | `async` 的 future 析构会等任务完成 → 不保存返回值就是**串行** |
| 异常传递 | 自动保存，`get()` 时重新抛出 —— 相对裸线程最大的优势 |
| `promise`/`future` | 写端/读端分离；`set_value` 只能一次；未设置则抛 `broken_promise` |
| `packaged_task` | 可调用对象 + future；不决定执行位置，是线程池的基础 |
| 线程池五要点 | `packaged_task` 统一返回类型、`shared_ptr` 绕过 move-only、`std::apply` 展开参数、**锁外执行任务**、析构时 shutdown |
| 实测验证 | TSan 压测 6000 个任务无数据竞争 |
| 并行归约 | 手工分块 + `async` 实测快 **7.7 倍**（比缺 TBB 的并行算法可靠）|
| 生产环境 | 用成熟的线程池库，不要自己写 |

Part 8 到此结束。

> 下一章：[09-01 特性速查表](../part9_appendix/09-01_cheatsheet.md)
