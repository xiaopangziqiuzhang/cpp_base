# 08-04 原子操作与内存模型

> 标准：`[C++11]`
> demo：[`08_04_atomic.cpp`](../../demos/part8_concurrency/08_04_atomic.cpp)

C++11 最容易被低估的贡献不是 `std::thread`，而是**内存模型** ——
它第一次在标准层面定义了"多线程程序的行为"。
没有它，所有多线程代码都建立在对编译器行为的猜测上。

## 一、atomic 的基本操作

```cpp
std::atomic<int> counter{0};

counter.store(10);                  // 原子写
counter.load();                      // 原子读
counter.fetch_add(5);                // 原子加，返回旧值
counter.exchange(100);               // 原子替换，返回旧值
++counter;  counter += 10;           // 这些也是原子的
```

实测：

```
        store(10) 之后 load() = 10
        fetch_add(5) 返回旧值 10，现在是 15
        exchange(100) 返回旧值 15，现在是 100
```

### atomic 不等于无锁

```cpp
std::atomic<int>{}.is_lock_free();              // 1
std::atomic<long long>{}.is_lock_free();        // 1
std::atomic<Two>{}.is_lock_free();              // 1（8 字节结构体）
```

但更大的类型就不是了。实测（需要链接 `-latomic`）：

```
        atomic<16 字节结构体>: 0
        atomic<24 字节结构体>: 0
```

**超过机器字长的类型，`std::atomic` 内部用锁实现** ——
通常是一个全局的锁表，按地址哈希。所以：

- `atomic<T>` 对大对象既不快也不无锁
- 不同对象可能共享同一把内部锁（伪竞争）

顺带一个实际的坑：对这类大对象调用 `is_lock_free()` 会引用
`__atomic_is_lock_free`，**不链接 `-latomic` 就是链接错误**：

```
undefined reference to `__atomic_is_lock_free'
```

`std::atomic_flag` 是**唯一保证无锁**的原子类型（标准规定）。

## 二、atomic vs mutex vs 不共享

实测 8 线程各 50 万次递增：

```
        atomic:          21 ms (结果 4000000)
        mutex:           280 ms (结果 4000000)
        局部累加+汇总:   0 ms (结果 4000000)
```

两个结论：

**一、`atomic` 比 `mutex` 快 13 倍** ——
原子指令不需要系统调用，也不涉及线程阻塞/唤醒。

**二、"不共享"比两者都快得多** ——
局部累加版本快到测不出来（0 ms）：

```cpp
std::thread([&] {
    long long local = 0;
    for (int j = 0; j < kOps; ++j) ++local;     // 无共享，编译器甚至能优化掉循环
    sharded += local;                            // 只同步一次
});
```

原因是**缓存行在核心之间来回弹跳**。多个核心争用同一个缓存行时，
每次写入都要让其他核心的缓存失效（MESI 协议的 invalidate），
这个代价远超指令本身。

**这是并发优化最重要的一条：减少共享，而不是优化同步。**

相关概念是 **false sharing**：两个不相关的变量恰好在同一缓存行，
导致无谓的争用。解法是填充到缓存行边界：

```cpp
struct alignas(64) PaddedCounter {      // 64 字节 = 典型缓存行大小
    std::atomic<long long> value{0};
};
// C++17 提供了 std::hardware_destructive_interference_size
```

## 三、CAS：compare_exchange

CAS（Compare-And-Swap）是所有无锁算法的基础：

```cpp
std::atomic<int> value{10};
int expected = 10;

// 如果当前值 == expected，就设为 desired，返回 true
// 否则把【当前实际值】写入 expected，返回 false
bool ok = value.compare_exchange_strong(expected, 20);
```

实测：

```
      CAS(expected=10, desired=20): 成功=1，现在值=20
      CAS(expected=10, desired=30): 成功=0，expected 被更新为实际值 20
```

**失败时 `expected` 被更新为当前实际值** —— 这个设计让重试很自然。

### CAS 循环：实现任意原子的读-改-写

`fetch_add` 只能做加法。要原子地做任意操作，用 CAS 循环：

```cpp
// 原子地乘以 2
std::atomic<int> v{5};
int old = v.load();
while (!v.compare_exchange_weak(old, old * 2)) {
    // 失败时 old 已被更新为当前值，直接重试
}
```

实测得到 10。这个模式可以实现任意的原子更新：

```cpp
// 通用形式
T old = atomic_value.load();
T desired;
do {
    desired = compute_new_value(old);
} while (!atomic_value.compare_exchange_weak(old, desired));
```

### strong vs weak

| | 行为 |
|---|---|
| `compare_exchange_strong` | 只在值不相等时才失败 |
| `compare_exchange_weak` | **可能虚假失败**（即使值相等），但某些平台更快 |

`weak` 的虚假失败来自某些架构的 LL/SC 指令（load-linked/store-conditional）——
中间被中断或别的核心访问了同一缓存行就会失败。

**规则**：

- 在**循环里**用 `weak`（反正要重试，虚假失败无所谓）
- **单次调用**用 `strong`（否则要自己写循环处理虚假失败）

## 四、六种内存序

这是 C++ 内存模型的核心，也是最难的部分。

| 内存序 | 约束 |
|---|---|
| `relaxed` | 只保证操作本身原子，**不保证顺序** |
| `consume` | 已不推荐使用（实现上等同 `acquire`）|
| `acquire` | **读**操作：之后的读写不能被重排到它之前 |
| `release` | **写**操作：之前的读写不能被重排到它之后 |
| `acq_rel` | **读改写**操作：同时有 acquire 和 release 语义 |
| `seq_cst` | 顺序一致，所有线程看到**单一的全局顺序**（默认，最强）|

### 最常用的模式：release-acquire 配对

```cpp
std::atomic<bool> flag{false};
int data = 0;                       // 普通变量，不是原子的

// 写线程
data = 42;                                         // 1. 普通写
flag.store(true, std::memory_order_release);        // 2. release 写

// 读线程
while (!flag.load(std::memory_order_acquire)) { }   // 3. acquire 读
assert(data == 42);                                  // 4. 保证能看到 1 的效果
```

实测没有出现数据竞争。

保证是这样建立的：

- **release**（2）保证它**之前**的所有写（包括 1）不会被重排到它之后
- **acquire**（3）保证它**之后**的所有读（包括 4）不会被重排到它之前
- 当 3 读到了 2 写入的值时，**1 的效果一定对 4 可见**

这叫 **synchronizes-with** 关系。它是实现无锁数据结构、
以及理解"为什么双检锁需要内存序"（见
[03-07](../part3_class/03-07_singleton.md)）的关键。

注意：`data` 是**普通变量**而不是原子变量 ——
release-acquire 保护的是它周围的普通内存访问。
这正是内存序的用途：用少量原子操作为大量普通数据建立可见性保证。

### relaxed 的正当用途

```cpp
std::atomic<int> counter{0};
counter.fetch_add(1, std::memory_order_relaxed);        // 只关心最终总数
```

实测 4 线程各 1 万次，得到正确的 40000。

`relaxed` 适合**只关心计数正确、不关心与其他操作的相对顺序**的场合：
统计计数器、引用计数的增加（减少到 0 时需要 acq_rel）。

**不适合**用来传递数据 —— 它不建立任何可见性保证。

## 五、内存序的性能：一个重要的更正

我最初实测了三种内存序的 `fetch_add` 性能，得到：

```
        seq_cst（默认）: 58 ms
        acq_rel        : 58 ms
        relaxed        : 74 ms        <- relaxed 更慢？
```

这个结果反直觉，所以我做了进一步验证。交替顺序测 5 轮：

```
轮次  seq_cst  acq_rel  relaxed
  0      41       43       32
  1      32       31       30
  2      33       31       32
  3      30       37       34
  4      38       38       37
```

**三者没有稳定的差别，全是噪声。**

然后我查了生成的汇编（`g++ -O2 -S`，x86-64）：

```asm
; fetch_add(1, memory_order_seq_cst)
lock addq $1, (%rdi)
ret

; fetch_add(1, memory_order_relaxed)
lock addq $1, (%rdi)
ret
```

**完全相同的指令。**

原因：**x86-64 的 `lock` 前缀指令本身就带完整屏障**。
x86 是强内存模型架构，它本来就不做很多重排，
所以 `relaxed` 和 `seq_cst` 在这条指令上生成同样的代码。

在 **ARM / POWER** 这类弱内存模型平台上差别就很大了 ——
那里 `seq_cst` 需要插入额外的 `dmb` / `sync` 屏障指令，
而 `relaxed` 不需要。

**教训：谈内存序的性能必须说明目标架构，而且要看汇编验证。**
"用 relaxed 更快"这个说法在 x86 上对 `fetch_add` 是不成立的。

（屏障对 `load`/`store` 的影响在 x86 上确实存在：
`seq_cst` 的 store 会生成 `xchg` 或 `mfence`，而 `release` store
就是普通的 `mov`。所以结论是"看具体操作"，不能一概而论。）

## 六、无锁栈：一个教学示例

```cpp
template <class T>
class LockFreeStack {
public:
    void push(T value) {
        Node* new_node = new Node{std::move(value), nullptr};
        Node* old_head = _head.load(std::memory_order_relaxed);
        do {
            new_node->next = old_head;
        } while (!_head.compare_exchange_weak(old_head, new_node,
                                              std::memory_order_release,
                                              std::memory_order_relaxed));
    }

    bool pop(T& out) {
        Node* old_head = _head.load(std::memory_order_acquire);
        while (old_head &&
               !_head.compare_exchange_weak(old_head, old_head->next,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed)) {
        }
        if (!old_head) return false;
        out = std::move(old_head->value);
        delete old_head;        // ⚠ 这里不安全！见下文
        return true;
    }
private:
    struct Node { T value; Node* next; };
    std::atomic<Node*> _head{nullptr};
};
```

实测 4 线程 push 1 万条再并发 pop：

```
        pushed=40000 popped=40000 ✓ 数量一致
```

### 这个实现不是生产级的

**问题一：ABA 问题**

```
线程 A: 读到 head = X，准备 CAS(X -> X->next)
线程 B: pop 了 X，pop 了 Y，又 push 了一个【恰好地址也是 X】的新节点
线程 A: CAS 成功了（因为值还是 X），但 X->next 已经完全不同了
```

解法：带版本号的指针（tagged pointer）或 `atomic<shared_ptr>`。

**问题二：内存回收**

```cpp
out = std::move(old_head->value);
delete old_head;                      // 别的线程可能正在读这个节点！
```

线程 A 在 `pop` 中读到 `old_head` 之后、CAS 之前，
线程 B 可能已经 pop 并 delete 了那个节点 ——
线程 A 接着读 `old_head->next` 就是 use-after-free。

解法：

- **hazard pointer** —— 每个线程声明"我正在用这些指针"，
  回收时跳过它们
- **引用计数** —— `std::atomic<std::shared_ptr<T>>`（C++20）
- **epoch-based reclamation** —— 延迟回收到所有线程都过了某个时间点
- **永不回收** —— 用内存池复用（规避问题但有内存占用）

本 demo 通过**分两阶段**（先全部 push 完，再全部 pop）规避了这些问题，
所以能跑出正确结果 —— 但真实的并发 push/pop 混合场景会出问题。

### 结论：不要手写无锁数据结构

无锁编程的难度远超一般预期。真实项目里应该：

1. **优先用锁** —— 现代互斥量在无争用时很快（futex 的快速路径是纯用户态）
2. 需要无锁时用成熟库 —— Boost.Lockfree、folly、moodycamel::ConcurrentQueue
3. 真要自己写，必须配合 TSan、形式化验证、以及大量压测

本节的价值在于**理解原理**，而不是提供可用代码。

## 七、atomic 的实际用途

### 用途一：停止标志（最常见）

```cpp
std::atomic<bool> stop_requested{false};

// 工作线程
while (!stop_requested.load(std::memory_order_relaxed)) {
    do_work();
}

// 主线程
stop_requested.store(true);
```

这是 `atomic` 最常见也最合适的用法。
（C++20 的 `std::stop_token` 提供了更完整的协作式取消。）

### 用途二：atomic_flag 实现自旋锁

```cpp
std::atomic_flag spin = ATOMIC_FLAG_INIT;

// lock
while (spin.test_and_set(std::memory_order_acquire)) { }
// unlock
spin.clear(std::memory_order_release);
```

`atomic_flag` 是唯一保证无锁的类型。但**自旋锁只适合极短的临界区** ——
等待的线程会一直占用 CPU。临界区稍长就该用 `mutex`
（它会让线程睡眠）。

实践中纯自旋锁很少用，通常用"先自旋几次再睡眠"的混合策略 ——
而这正是 `std::mutex` 的实现方式。

### 用途三：shared_ptr 的原子性（常见误解）

```cpp
std::shared_ptr<T> p = ...;
```

**引用计数的增减是原子的**（线程安全），所以：

```cpp
// 线程安全：多个线程各自拷贝同一个 shared_ptr
auto local = global_ptr;        // 如果 global_ptr 本身不被修改
```

**但 `shared_ptr` 对象本身的读写不是原子的**：

```cpp
// 数据竞争！
std::shared_ptr<T> global;
// 线程 A: global = make_shared<T>();
// 线程 B: auto p = global;
```

多线程要修改同一个 `shared_ptr` 变量，需要：

- `std::atomic<std::shared_ptr<T>>`（C++20）
- `std::atomic_load`/`atomic_store` 的 shared_ptr 重载（C++11，C++20 废弃）
- 或者加锁

见 [02-05](../part2_memory/02-05_smart_pointers.md)。

## 八、小结

| 要点 | 说明 |
|---|---|
| C++11 的核心贡献 | **内存模型** —— 第一次定义了多线程程序的行为 |
| atomic 基本操作 | `load`/`store`/`fetch_add`/`exchange`；`++` `+=` 也是原子的 |
| **atomic ≠ 无锁** | 超过机器字长的类型内部用锁；`atomic_flag` 是唯一保证无锁的 |
| 实际坑 | 大对象调 `is_lock_free()` 要链接 `-latomic` |
| **性能实测** | atomic 比 mutex 快 **13 倍**；但**不共享**比两者都快得多 |
| 为什么 | 缓存行在核心间弹跳的代价远超指令本身 |
| 最重要的优化 | **减少共享**，而不是优化同步 |
| CAS | 失败时 `expected` 被更新为实际值，便于重试 |
| CAS 循环 | 可实现任意原子的读-改-写 |
| strong vs weak | 循环里用 `weak`，单次调用用 `strong` |
| 六种内存序 | relaxed < acquire/release < acq_rel < seq_cst |
| **release-acquire** | 最常用的配对；用原子操作为**普通变量**建立可见性保证 |
| **内存序的性能** | x86-64 上 `fetch_add` 三种内存序**生成相同指令**（已查汇编）；ARM 上才有差别 |
| 无锁栈的问题 | **ABA 问题** + **内存回收**（use-after-free）|
| 实践建议 | **不要手写无锁结构**；优先用锁，需要时用成熟库 |
| atomic 的合适用途 | 停止标志、简单计数器、自旋锁（极短临界区）|
| shared_ptr | 引用计数原子，但**对象本身的读写不原子** |

> 下一章：[08-05 异步任务](08-05_async.md)
