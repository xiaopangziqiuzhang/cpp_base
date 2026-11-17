# 08-02 互斥与锁

> 标准：`mutex`/`lock_guard`/`unique_lock`/`call_once` `[C++11]`；
> `shared_timed_mutex` `[C++14]`；`shared_mutex`/`scoped_lock` `[C++17]`
> demo：[`08_02_mutex.cpp`](../../demos/part8_concurrency/08_02_mutex.cpp)

## 一、手工 lock/unlock 的问题

```cpp
std::mutex mtx;

void bad_style() {
    mtx.lock();
    ++shared_data;
    if (error) return;        // 忘了 unlock -> 死锁
    may_throw();               // 抛异常 -> 死锁（栈展开不会调 unlock）
    mtx.unlock();
}
```

三个问题：

1. 中途 `return` 忘了 `unlock`
2. 抛异常时不会解锁
3. 多个出口要重复写 `unlock`

**死锁比内存泄漏更难查** —— 程序不崩溃，只是卡住，
而且往往只在特定的时序下发生。

解法是 RAII（见 [02-06](../part2_memory/02-06_raii.md)）。

## 二、lock_guard：最简单的 RAII 锁

```cpp
class Counter {
public:
    void increment() {
        std::lock_guard<std::mutex> lk(_mtx);      // 构造时 lock
        ++_value;
    }   // 析构时 unlock，即使抛异常也保证解锁

    int get() const {
        std::lock_guard<std::mutex> lk(_mtx);
        return _value;
    }
private:
    mutable std::mutex _mtx;      // 注意 mutable
    int _value = 0;
};
```

实测 8 线程各 ++10000 次，结果正好 80000。

两个细节：

**一、`mutex` 成员必须是 `mutable` 的** ——
否则 const 成员函数无法加锁（`lock()` 是非 const 的）。
见 [03-06](../part3_class/03-06_const_member.md)。

**二、C++17 起可以省略模板参数**（CTAD，见
[05-02](../part5_template/05-02_class_template.md)）：

```cpp
std::lock_guard<std::mutex> lk(mtx);     // C++11
std::lock_guard lk(mtx);                  // C++17
```

`lock_guard` 的特点是**极简**：只有构造和析构，没有任何其他接口，
所以零额外开销。**它应该是默认选择。**

## 三、unique_lock：更灵活的锁

`unique_lock` 比 `lock_guard` 多五种能力：

```cpp
// 能力一：延迟加锁
std::unique_lock<std::mutex> lk(mtx, std::defer_lock);
lk.owns_lock();        // false
lk.lock();             // 手工加锁

// 能力二：提前解锁（缩小临界区）
std::unique_lock<std::mutex> lk(mtx);
use_shared_data();
lk.unlock();            // 提前释放
do_expensive_work();     // 不需要锁的耗时操作
lk.lock();

// 能力三：可移动（能从函数返回）
std::unique_lock<std::mutex> make_locked() {
    std::unique_lock<std::mutex> lk(mtx);
    return lk;           // 锁的所有权转移给调用方
}

// 能力四：尝试加锁
std::unique_lock<std::mutex> lk(mtx, std::try_to_lock);
if (lk.owns_lock()) { /* 拿到了 */ }

// 能力五：配合 condition_variable（必须用 unique_lock，见 08-03）
```

实测全部正常：

```
        defer_lock: 构造时不加锁，owns_lock=0
        手工 lock() 之后 owns_lock=1
        可以提前 unlock 来缩小临界区
        可以从函数返回锁，owns_lock=1
        try_to_lock: 拿到锁了吗 1
```

**代价**：比 `lock_guard` 多一个 bool 标志（记录是否持有锁）
和相应的运行期检查。

**规则：默认用 `lock_guard`，需要上面五种能力之一才用 `unique_lock`。**

## 四、死锁与 scoped_lock

### 死锁的条件

```cpp
// 线程 1
lock(A); lock(B);

// 线程 2
lock(B); lock(A);
```

两个线程各持一把锁、等另一把 → **永久阻塞**。

典型场景是银行转账：

```cpp
void transfer(Account& from, Account& to, int amount) {
    std::lock_guard<std::mutex> lk1(from.mtx);
    std::lock_guard<std::mutex> lk2(to.mtx);
    from.balance -= amount;
    to.balance += amount;
}

// 线程 1: transfer(a, b, 100)    -> lock(a) 然后 lock(b)
// 线程 2: transfer(b, a, 100)    -> lock(b) 然后 lock(a)   <- 死锁！
```

### 解法一：std::lock 一次性加多个锁

```cpp
void transfer(Account& from, Account& to, int amount) {
    std::lock(from.mtx, to.mtx);        // 内部有死锁避免算法
    std::lock_guard<std::mutex> lk1(from.mtx, std::adopt_lock);
    std::lock_guard<std::mutex> lk2(to.mtx, std::adopt_lock);
    // ...
}
```

`std::lock` 用的是"全部获取或全部释放重试"的算法，
保证不会死锁。`std::adopt_lock` 告诉 `lock_guard`"锁已经持有了，
你只负责解锁"。

### 解法二：scoped_lock `[C++17]`

上面写法的简化版：

```cpp
void transfer(Account& from, Account& to, int amount) {
    std::scoped_lock lk(from.mtx, to.mtx);     // 一行，CTAD 推导类型
    from.balance -= amount;
    to.balance += amount;
}
```

实测 8 个线程双向并发转账 200 次：

```
        结束后 A=1000 B=1000 总和=2000（应为 2000）
        没有死锁，因为 scoped_lock 内部用了死锁避免算法
```

`scoped_lock` 可以锁 0 个、1 个或多个互斥量，
**C++17 之后它应该取代 `lock_guard` 成为默认选择**
（单个锁时两者等价，多个锁时它更安全）。

### 三种防死锁手段

| 手段 | 说明 |
|---|---|
| **固定加锁顺序** | 全局约定一个顺序（比如按互斥量的地址排序）|
| **`std::scoped_lock`** | 一次性加多个锁，内部避免死锁 |
| **`try_lock` + 超时重试** | 拿不到就放弃已有的锁，退避后重试 |

按地址排序的写法：

```cpp
void transfer(Account& a, Account& b, int amount) {
    Account* first = &a; Account* second = &b;
    if (first > second) std::swap(first, second);     // 固定顺序
    std::lock_guard<std::mutex> lk1(first->mtx);
    std::lock_guard<std::mutex> lk2(second->mtx);
}
```

还有一条设计原则：**持有锁时不要调用用户代码**（回调、虚函数）——
你不知道它会不会去拿别的锁。

## 五、mutex 家族

| 类型 | 特点 | 标准 |
|---|---|---|
| `std::mutex` | 基本互斥量，**不可重入** | C++11 |
| `std::recursive_mutex` | 同一线程可多次加锁 | C++11 |
| `std::timed_mutex` | 支持 `try_lock_for`/`try_lock_until` | C++11 |
| `std::recursive_timed_mutex` | 上面两者的组合 | C++11 |
| `std::shared_timed_mutex` | 读写锁 + 超时 | C++14 |
| `std::shared_mutex` | 读写锁 | C++17 |

### recursive_mutex

```cpp
std::recursive_mutex rmtx;
rmtx.lock();
rmtx.lock();        // 同一线程再次加锁，OK
rmtx.unlock();
rmtx.unlock();      // 要解锁同样的次数
```

实测可行。**普通 `mutex` 同一线程二次加锁是未定义行为**（通常死锁）。

用途：递归函数、公开函数互相调用。

但要注意：**需要 `recursive_mutex` 通常说明设计有问题**。
典型的情况是公开函数之间互相调用：

```cpp
class Bad {
public:
    void f() { std::lock_guard lk(_mtx); /* ... */ g(); }     // g 也要加锁 -> 死锁
    void g() { std::lock_guard lk(_mtx); /* ... */ }
};

// 更好：把实现抽到私有的不加锁函数
class Good {
public:
    void f() { std::lock_guard lk(_mtx); f_impl(); g_impl(); }
    void g() { std::lock_guard lk(_mtx); g_impl(); }
private:
    void f_impl();      // 假定已持有锁
    void g_impl();
};
```

### timed_mutex

```cpp
std::timed_mutex tmtx;
if (tmtx.try_lock_for(std::chrono::milliseconds(10))) {
    // 拿到了
    tmtx.unlock();
} else {
    // 超时，去做别的事
}
```

实测成功。用于避免无限等待 —— 在需要响应性的场合（UI 线程、
有超时要求的服务）很有用。

## 六、shared_mutex：读写锁 `[C++17]`

允许**多个读者同时持有**，但写者独占：

```cpp
class ThreadSafeCache {
public:
    int get(int key) const {
        std::shared_lock<std::shared_mutex> lk(_mtx);      // 共享（读）锁
        auto it = _data.find(key);
        return it == _data.end() ? -1 : it->second;
    }

    void put(int key, int value) {
        std::unique_lock<std::shared_mutex> lk(_mtx);      // 独占（写）锁
        _data[key] = value;
    }
private:
    mutable std::shared_mutex _mtx;
    std::map<int, int> _data;
};
```

### 重要：shared_mutex 并不总是更快

通常的说法是"读远多于写时用读写锁"。我做了实测，
结果比这个说法复杂：

```
读线程数    std::mutex    std::shared_mutex
   4          4 ms          2 ms   <- shared 赢
  16         17 ms         14 ms   <- 略赢
  64         72 ms         86 ms   <- shared 反而【更慢】
```

（`-O2` 编译，临界区是一次 `std::map` 查找。）

**64 线程时 `shared_mutex` 反而慢 19%。**

原因：`shared_mutex` 需要维护读者计数，这个簿记本身比
`mutex` 的简单加锁重。**临界区很短时，这个开销超过了并发读的收益**，
而且高并发下对读者计数的争用也成了瓶颈。

### 结论

只有同时满足两个条件才用 `shared_mutex`：

1. **临界区较长**（不是一次 map 查找这种）
2. **读远多于写**

而且**一定要实测**，不要假设它更快。

其他注意：

- 写者可能被持续的读者饿死（标准没有规定公平性）
- 不能把 `shared_lock` 升级成 `unique_lock`（会死锁），
  要先解锁再重新加
- 读多写少且数据不大时，考虑其他方案：
  原子指针 + 写时复制、RCU、或干脆每线程一份数据

## 七、call_once

```cpp
std::once_flag flag;

void init() { /* 昂贵的初始化 */ }

// 8 个线程调用，init 只执行一次
std::call_once(flag, init);
```

实测 8 个线程调用，初始化只执行了一次。

用途：延迟初始化、单例（见 [03-07](../part3_class/03-07_singleton.md)）。

特点：

- **线程安全**，而且比"加锁 + 检查标志"更高效（内部用了优化的快速路径）
- **如果回调抛异常，标志不会被设置** —— 下次调用还会重试。
  这是和静态局部变量初始化不同的地方（后者抛异常也会重试，但行为更微妙）

多数情况下 Meyers Singleton（静态局部变量）比 `call_once` 更简单，
但 `call_once` 在需要控制初始化时机或传参时有用。

## 八、锁的选择与设计原则

### 选择哪种锁

```
需要互斥？
├── 单个互斥量，简单加解锁 -> std::lock_guard（或 C++17 的 scoped_lock）
├── 多个互斥量             -> std::scoped_lock [C++17]
├── 需要延迟/提前/移动/条件变量 -> std::unique_lock
├── 读远多于写 + 临界区较长  -> std::shared_mutex [C++17]（要实测！）
└── 只需要初始化一次        -> std::call_once
```

### 设计原则

**一、临界区尽量小**

```cpp
// 不好：锁住了整个函数
void process() {
    std::lock_guard lk(_mtx);
    auto data = read_shared();
    auto result = expensive_computation(data);     // 不需要锁！
    write_shared(result);
}

// 更好
void process() {
    decltype(auto) data = [this] {
        std::lock_guard lk(_mtx);
        return read_shared();
    }();
    auto result = expensive_computation(data);      // 无锁
    {
        std::lock_guard lk(_mtx);
        write_shared(result);
    }
}
```

**二、不要在持有锁时调用未知代码**

回调、虚函数、用户提供的比较器都可能去拿别的锁。

**三、锁保护的是数据，不是代码**

把互斥量和它保护的数据放在一起（同一个类的私有成员），
而不是用全局互斥量保护散落各处的变量。

**四、避免嵌套锁**

需要同时持有多把锁时用 `scoped_lock`，
不要在一个锁的临界区内获取另一把锁。

**五、优先考虑不共享**

```cpp
// 每线程一份，最后汇总 —— 完全没有锁
thread_local long long local_sum = 0;
```

这通常比任何锁优化都有效。

## 九、小结

| 要点 | 说明 |
|---|---|
| 手工 lock/unlock | 中途 return、抛异常都会死锁 → 用 RAII |
| `lock_guard` | 最简单、零开销；**默认选择** |
| mutex 成员要 `mutable` | 否则 const 成员函数无法加锁 |
| `unique_lock` 的五种能力 | 延迟加锁、提前解锁、可移动、尝试加锁、配合条件变量 |
| 死锁条件 | 两个线程以**不同顺序**获取同样的两把锁 |
| `scoped_lock` `[C++17]` | 一次性加多个锁，内部避免死锁；**应取代 `lock_guard`** |
| 防死锁三法 | 固定加锁顺序、`scoped_lock`、`try_lock` + 重试 |
| `recursive_mutex` | 同一线程可重入；**但需要它通常说明设计有问题** |
| 普通 mutex 重入 | **未定义行为**（通常死锁）|
| **`shared_mutex` 实测** | 4 线程快 2 倍，但 **64 线程反而慢 19%** |
| 何时用 shared_mutex | 临界区**较长** + 读远多于写；**必须实测** |
| `call_once` | 线程安全的一次性初始化；回调抛异常则会重试 |
| 设计原则 | 临界区小、不在锁内调未知代码、锁和数据放一起、优先不共享 |

> 下一章：[08-03 条件变量与同步](08-03_condition_variable.md)
