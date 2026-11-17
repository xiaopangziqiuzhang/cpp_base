# 08-03 条件变量与同步

> 标准：`[C++11]`
> demo：[`08_03_condition_variable.cpp`](../../demos/part8_concurrency/08_03_condition_variable.cpp)

## 一、解决了什么问题

互斥量解决的是"**不要同时访问**"，条件变量解决的是"**等待某个条件成立**"。

没有条件变量时，等待只能靠**忙等待**：

```cpp
std::atomic<bool> ready{false};

// 消费者
while (!ready.load()) {
    // 空转
}
use_data();
```

实测空转次数：

```
      忙等待空转了 4813108 次
```

20 毫秒的等待空转了 **480 万次**。三个问题：

1. **浪费 CPU** —— 一个核心被完全占用
2. **和其他线程抢 CPU** —— 包括那个本该设置 `ready` 的线程
3. **耗电** —— 移动设备上尤其严重

改进成"睡一下再查"：

```cpp
while (!ready.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
```

但这又有延迟和 CPU 占用的权衡 —— 睡得久延迟高，睡得短 CPU 还是浪费。

条件变量的做法是：**让线程真正睡眠，直到有人通知它条件可能满足了**。
操作系统负责在通知到来时唤醒它，期间不消耗 CPU。

## 二、基本用法

```cpp
std::mutex mtx;
std::condition_variable cv;
bool ready = false;
std::string data;

// 消费者
{
    std::unique_lock<std::mutex> lk(mtx);          // 必须是 unique_lock
    cv.wait(lk, [&ready] { return ready; });        // 带谓词的 wait
    use(data);
}

// 生产者
{
    std::lock_guard<std::mutex> lk(mtx);
    data = "hello";
    ready = true;
}
cv.notify_one();
```

实测消费者正确收到 "hello"。

### wait 的三步

```cpp
cv.wait(lk, pred);
```

1. **解锁** `lk`（这样生产者才能拿到锁去修改条件）
2. **睡眠等待**通知
3. 被唤醒后**重新加锁**，然后检查 `pred`

**必须用 `unique_lock`** 而不是 `lock_guard` ——
因为 `wait` 需要在中途解锁再加锁，而 `lock_guard` 没有这个接口
（见 [08-02](08-02_mutex.md)）。

### 为什么条件变量必须配合互斥量

条件变量保护的不是自己，而是**条件所依赖的共享数据**。

如果没有锁：

```cpp
// 消费者
if (!ready) {                  // 1. 检查条件
    cv.wait(lk);               // 3. 等待
}

// 生产者（在 1 和 3 之间插进来）
ready = true;                  // 2. 设置条件
cv.notify_one();               //    通知 —— 但消费者还没开始 wait！
```

消费者会错过通知，然后永久等待。这叫 **lost wakeup**。

互斥量保证了"检查条件"和"开始等待"是原子的 ——
`wait` 在解锁的同一时刻把自己注册到等待队列上。

## 三、虚假唤醒：必须用谓词

**`wait()` 可能在没有 `notify` 的情况下返回** —— 这叫**虚假唤醒**。

原因有两类：

1. **操作系统的实现细节** —— 比如线程被信号中断后重新进入 wait，
   POSIX 明确允许 `pthread_cond_wait` 虚假返回
2. **`notify_all` 唤醒了多个线程**，但资源只够一个用 ——
   其他线程醒来发现条件已经不满足了

所以**绝不能**这样写：

```cpp
cv.wait(lk);        // 错！被唤醒不代表条件满足
use_data();          // 可能用到还没准备好的数据
```

### 两种正确写法

```cpp
// 写法一：带谓词的 wait（推荐）
cv.wait(lk, [&] { return condition; });

// 写法二：手工循环（完全等价）
while (!condition) cv.wait(lk);
```

带谓词的 `wait` 在标准里**就是定义为**：

```cpp
while (!pred()) wait(lk);
```

所以即使虚假唤醒，也会重新检查条件后继续睡。

**规则：永远用带谓词的版本。** 这不只是防虚假唤醒 ——
它同时处理了"通知在 wait 之前到达"和"多个等待者竞争"的情况。

## 四、生产者消费者队列（完整实现）

这是条件变量最经典的应用。完整实现要处理：
队列空、队列满、优雅关闭。

```cpp
template <class T>
class BlockingQueue {
public:
    explicit BlockingQueue(std::size_t max_size = 100) : _max_size(max_size) {}

    void push(T value) {
        std::unique_lock<std::mutex> lk(_mtx);
        _not_full.wait(lk, [this] { return _queue.size() < _max_size || _closed; });
        if (_closed) return;
        _queue.push(std::move(value));
        lk.unlock();                    // 先解锁再通知（见第六节）
        _not_empty.notify_one();
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lk(_mtx);
        _not_empty.wait(lk, [this] { return !_queue.empty() || _closed; });
        if (_queue.empty()) return std::nullopt;        // 已关闭且空
        T value = std::move(_queue.front());
        _queue.pop();
        lk.unlock();
        _not_full.notify_one();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lk(_mtx);
            _closed = true;
        }
        _not_empty.notify_all();        // 必须 notify_all
        _not_full.notify_all();
    }

private:
    mutable std::mutex _mtx;
    std::condition_variable _not_empty;
    std::condition_variable _not_full;
    std::queue<T> _queue;
    std::size_t _max_size;
    bool _closed = false;
};
```

实测 3 生产者 × 500 条、4 消费者：

```
        消费总数: 1500（期望 1500） ✓
        队列残留: 0
```

我还用 ThreadSanitizer 跑了 5 轮压测（4 生产者 × 800 条、6 消费者）：

```
round 0: consumed=3200 expected=3200 residual=0 OK
round 1: consumed=3200 expected=3200 residual=0 OK
round 2: consumed=3200 expected=3200 residual=0 OK
round 3: consumed=3200 expected=3200 residual=0 OK
round 4: consumed=3200 expected=3200 residual=0 OK
```

**没有报出任何数据竞争**，计数和残留都正确。

### 设计要点

**一、两个条件变量** —— `_not_empty` 和 `_not_full`。

用一个条件变量也能工作（都用 `notify_all`），但效率低：
入队时会唤醒其他生产者（它们等的是"非满"），白醒一次。

**二、`close()` 必须用 `notify_all`**

关闭是状态变化，**所有**等待者都应该重新检查条件并退出。
用 `notify_one` 会让其他线程永久挂起。

**三、`pop()` 返回 `std::optional`**

用它区分"取到了元素"和"队列已关闭" ——
比用 `bool pop(T& out)` 的出参形式清楚（见
[07-04](../part7_stdlib/07-04_tuple_variant.md)）。

**四、有界队列提供背压**

`_max_size` 限制让生产者在队列满时阻塞，
避免生产快于消费时内存无限增长。

## 五、notify_one vs notify_all

| | 唤醒 | 用于 |
|---|---|---|
| `notify_one` | 一个等待线程（哪个不确定）| 所有等待者等同一条件，资源只够一个用 |
| `notify_all` | 所有等待线程 | 等待者等不同条件；状态变化影响所有人 |

### 什么时候必须用 notify_all

1. **等待者在等不同的条件** —— 只唤醒一个可能唤醒"不该醒的那个"
2. **状态变化让所有等待者都应该重新检查**（如 `close()`）
3. **一次性放出多个资源**

### 误用的后果

| 误用 | 后果 |
|---|---|
| 该用 `all` 却用了 `one` | **有线程永远不被唤醒**（挂起）—— 严重 bug |
| 该用 `one` 却用了 `all` | 惊群效应，性能损失但**不会错** |

**不确定时用 `notify_all`** —— 正确性优先于性能。

## 六、先解锁还是先 notify

两种写法：

```cpp
// 写法 A：持锁通知
{
    std::lock_guard lk(mtx);
    data = x;
    ready = true;
    cv.notify_one();
}

// 写法 B：先解锁再通知
{
    std::lock_guard lk(mtx);
    data = x;
    ready = true;
}
cv.notify_one();
```

**两者都正确**，差别在性能：

- **写法 A**：被唤醒的线程要等通知者释放锁，可能立刻又阻塞一次
  （这叫 "hurry up and wait"）
- **写法 B**：被唤醒时锁已空闲，可以直接继续

所以通常推荐**写法 B**。

但有个例外：如果条件变量对象本身的生命周期依赖那把锁
（比如它是某个被锁保护的对象的成员，而通知后对象可能被销毁），
就必须在持锁时通知。

## 七、wait_for / wait_until：带超时

```cpp
std::unique_lock<std::mutex> lk(mtx);
bool got = cv.wait_for(lk, std::chrono::milliseconds(30),
                       [&ready] { return ready; });
```

实测：

```
      wait_for(30ms) 超时返回，等了 30 ms，谓词结果 = 0
```

**带谓词的版本返回谓词的值**：

- `true` = 条件满足
- `false` = 超时且条件仍不满足

不带谓词的版本返回 `std::cv_status::timeout` / `no_timeout`，
但那个版本仍然要自己处理虚假唤醒，所以不推荐。

用途：

```cpp
// 心跳检测
while (running) {
    if (!cv.wait_for(lk, 1s, [&] { return has_heartbeat; })) {
        log("心跳超时");
    }
}

// 优雅退出
cv.wait_for(lk, shutdown_timeout, [&] { return all_done || stop_requested; });
```

注意 `wait_for` 用的是**相对时间**，在循环里用会累积误差
（每次虚假唤醒都重置超时）。需要精确的截止时间用 `wait_until`：

```cpp
auto deadline = std::chrono::steady_clock::now() + 5s;
cv.wait_until(lk, deadline, pred);        // 绝对时间，不会累积
```

## 八、其他同步设施

### C++20 新增（供对比）

C++17 下只有条件变量这一种"等待"设施。C++20 补充了几个更专门的：

```cpp
// std::latch：一次性的倒计数（等 N 个任务完成）
std::latch done(4);
// 工作线程: done.count_down();
done.wait();

// std::barrier：可重复使用的同步点（所有线程到齐才继续）
std::barrier sync(4);
// 每个线程: sync.arrive_and_wait();

// std::counting_semaphore：计数信号量
std::counting_semaphore<10> sem(3);
sem.acquire(); /* ... */ sem.release();
```

C++17 下这些都要用条件变量自己实现。
`latch` 的实现很简单：

```cpp
class Latch {
public:
    explicit Latch(int count) : _count(count) {}
    void count_down() {
        std::lock_guard lk(_mtx);
        if (--_count == 0) _cv.notify_all();
    }
    void wait() {
        std::unique_lock lk(_mtx);
        _cv.wait(lk, [this] { return _count == 0; });
    }
private:
    std::mutex _mtx;
    std::condition_variable _cv;
    int _count;
};
```

### condition_variable_any

`std::condition_variable` 只能配合 `std::unique_lock<std::mutex>`。
需要配合其他锁类型（如 `shared_mutex`）时用
`std::condition_variable_any`：

```cpp
std::condition_variable_any cv;
std::shared_lock<std::shared_mutex> lk(smtx);
cv.wait(lk, pred);
```

代价是性能稍差（它需要额外的内部互斥量）。

## 九、小结

| 要点 | 说明 |
|---|---|
| 解决什么 | 等待条件成立；替代忙等待（实测 20ms 空转 480 万次）|
| 必须配合互斥量 | 保证"检查条件"和"开始等待"原子，避免 lost wakeup |
| 必须用 `unique_lock` | `wait` 需要中途解锁再加锁 |
| `wait` 的三步 | 解锁 → 睡眠 → 唤醒后重新加锁 |
| **虚假唤醒** | `wait` 可能无通知就返回 → **必须用带谓词的版本** |
| 带谓词的 wait | 等价于 `while (!pred()) wait(lk);` |
| 生产者消费者队列 | 两个条件变量、`close()` 用 `notify_all`、`pop` 返回 `optional`、有界提供背压 |
| 实测验证 | TSan 压测 5 轮无数据竞争，计数正确 |
| `notify_one` vs `all` | 该用 all 用了 one → **线程永久挂起**；反之只是性能损失 |
| 不确定时 | 用 `notify_all`（正确性优先）|
| 通知时机 | 先解锁再通知通常更快（避免 hurry-up-and-wait）|
| `wait_for` 带谓词 | 返回**谓词的值**；循环里用 `wait_until` 避免累积误差 |
| C++20 补充 | `latch`、`barrier`、`counting_semaphore`（C++17 下自己实现）|

> 下一章：[08-04 原子操作与内存模型](08-04_atomic.md)
