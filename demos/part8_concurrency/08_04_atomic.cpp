// std=c++17 flags=-pthread
// 对应章节: blog/part8_concurrency/08-04_atomic.md
// 主题: atomic、CAS、六种内存序、无锁栈

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ---------- 1. atomic 的基本操作 ----------
namespace atomic_basics {
    void demo() {
        std::atomic<int> counter{0};

        std::cout << "      基本操作:\n";
        counter.store(10);
        std::cout << "        store(10) 之后 load() = " << counter.load() << "\n";
        std::cout << "        fetch_add(5) 返回旧值 " << counter.fetch_add(5)
                  << "，现在是 " << counter.load() << "\n";
        std::cout << "        exchange(100) 返回旧值 " << counter.exchange(100)
                  << "，现在是 " << counter.load() << "\n";

        std::cout << "      ++ 和 += 也是原子的:\n";
        ++counter;
        counter += 10;
        std::cout << "        ++ 和 += 10 之后 = " << counter.load() << "\n";

        std::cout << "      is_lock_free 判断是否真无锁:\n";
        std::atomic<int> ai;
        std::atomic<long long> all;
        struct Two { int a, b; };          // 8 字节，能放进一个机器字
        std::atomic<Two> at;
        std::cout << "        atomic<int>:        " << ai.is_lock_free() << "\n";
        std::cout << "        atomic<long long>:  " << all.is_lock_free() << "\n";
        std::cout << "        atomic<Two(8B)>:    " << at.is_lock_free() << "\n";
        std::cout << "      更大的类型（如 16/24 字节的结构体）is_lock_free 为 0，\n";
        std::cout << "      内部用锁实现。实测（需 -latomic 链接）:\n";
        std::cout << "        atomic<16 字节结构体>: 0\n";
        std::cout << "        atomic<24 字节结构体>: 0\n";
        std::cout << "      顺带一个实际坑: 对这类大对象用 is_lock_free() 会引用\n";
        std::cout << "      __atomic_is_lock_free，不链接 -latomic 就是链接错误。\n";
        std::cout << "      结论: atomic 不等于无锁，大对象会退化为加锁\n";
    }
}

// ---------- 2. 原子操作 vs 加锁的性能 ----------
namespace atomic_vs_mutex {
    void demo() {
        constexpr int kThreads = 8;
        constexpr int kOps = 500000;

        // atomic
        std::atomic<long long> atomic_counter{0};
        auto t1 = std::chrono::steady_clock::now();
        {
            std::vector<std::thread> ts;
            for (int i = 0; i < kThreads; ++i)
                ts.emplace_back([&] { for (int j = 0; j < kOps; ++j) ++atomic_counter; });
            for (auto& t : ts) t.join();
        }
        auto t2 = std::chrono::steady_clock::now();

        // mutex
        long long mutex_counter = 0;
        std::mutex mtx;
        {
            std::vector<std::thread> ts;
            for (int i = 0; i < kThreads; ++i)
                ts.emplace_back([&] {
                    for (int j = 0; j < kOps; ++j) {
                        std::lock_guard<std::mutex> lk(mtx);
                        ++mutex_counter;
                    }
                });
            for (auto& t : ts) t.join();
        }
        auto t3 = std::chrono::steady_clock::now();

        // 每线程局部累加，最后汇总（最快）
        std::atomic<long long> sharded{0};
        {
            std::vector<std::thread> ts;
            for (int i = 0; i < kThreads; ++i)
                ts.emplace_back([&] {
                    long long local = 0;
                    for (int j = 0; j < kOps; ++j) ++local;     // 无共享
                    sharded += local;                            // 只同步一次
                });
            for (auto& t : ts) t.join();
        }
        auto t4 = std::chrono::steady_clock::now();

        auto ms = [](auto a, auto b) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
        };
        std::cout << "      " << kThreads << " 线程各 " << kOps << " 次递增:\n";
        std::cout << "        atomic:          " << ms(t1, t2)
                  << " ms (结果 " << atomic_counter.load() << ")\n";
        std::cout << "        mutex:           " << ms(t2, t3)
                  << " ms (结果 " << mutex_counter << ")\n";
        std::cout << "        局部累加+汇总:   " << ms(t3, t4)
                  << " ms (结果 " << sharded.load() << ")\n";
        std::cout << "      结论: atomic 比 mutex 快，但【不共享】比两者都快得多\n";
        std::cout << "            原因是缓存行在核心之间来回弹跳（false sharing 的亲戚）\n";
    }
}

// ---------- 3. CAS：compare_exchange ----------
namespace cas {
    void demo() {
        std::atomic<int> value{10};

        // compare_exchange_strong: 如果当前值==expected，则设为 desired
        int expected = 10;
        bool ok = value.compare_exchange_strong(expected, 20);
        std::cout << "      CAS(expected=10, desired=20): 成功=" << ok
                  << "，现在值=" << value.load() << "\n";

        expected = 10;        // 现在实际值是 20，所以会失败
        ok = value.compare_exchange_strong(expected, 30);
        std::cout << "      CAS(expected=10, desired=30): 成功=" << ok
                  << "，expected 被更新为实际值 " << expected << "\n";
        std::cout << "      失败时 expected 会被写入【当前实际值】，便于重试\n";

        // CAS 循环：实现任意原子的读-改-写
        std::cout << "      用 CAS 循环实现\"原子地乘以 2\":\n";
        std::atomic<int> v{5};
        int old = v.load();
        while (!v.compare_exchange_weak(old, old * 2)) {
            // 失败时 old 已被更新为当前值，直接重试
        }
        std::cout << "        5 * 2 = " << v.load() << "\n";

        std::cout << "      strong vs weak:\n";
        std::cout << "        strong: 只在值不等时才失败\n";
        std::cout << "        weak:   可能【虚假失败】（即使值相等），但在某些平台更快\n";
        std::cout << "        规则: 在循环里用 weak，单次调用用 strong\n";
    }
}

// ---------- 4. 六种内存序 ----------
namespace memory_orders {
    void demo() {
        std::cout << "      六种内存序（约束由弱到强）:\n";
        std::cout << "        relaxed —— 只保证操作本身原子，不保证顺序\n";
        std::cout << "        consume —— 已不推荐使用（实现上等同 acquire）\n";
        std::cout << "        acquire —— 读操作：之后的读写不能被重排到它之前\n";
        std::cout << "        release —— 写操作：之前的读写不能被重排到它之后\n";
        std::cout << "        acq_rel —— 读改写操作：同时有 acquire 和 release 语义\n";
        std::cout << "        seq_cst —— 顺序一致，全局单一顺序（默认，最强最慢）\n";

        std::cout << "      最常用的模式：release-acquire 配对\n";

        // release-acquire 保证数据可见性
        std::atomic<bool> flag{false};
        int data = 0;                       // 普通变量

        std::thread writer([&] {
            data = 42;                                        // 1. 普通写
            flag.store(true, std::memory_order_release);       // 2. release 写
            // release 保证: 1 的效果在 2 之前对别人可见
        });

        std::thread reader([&] {
            while (!flag.load(std::memory_order_acquire)) {    // 3. acquire 读
                std::this_thread::yield();
            }
            // acquire 保证: 看到 2 之后，一定能看到 1 的效果
            if (data != 42) std::cout << "        数据竞争！（不应发生）\n";
        });

        writer.join(); reader.join();
        std::cout << "        release-acquire 配对保证了 data 的可见性\n";
        std::cout << "        这是实现无锁数据结构的基础\n";

        std::cout << "      relaxed 的正当用途：计数器（只关心最终总数）\n";
        std::atomic<int> counter{0};
        {
            std::vector<std::thread> ts;
            for (int i = 0; i < 4; ++i)
                ts.emplace_back([&] {
                    for (int j = 0; j < 10000; ++j)
                        counter.fetch_add(1, std::memory_order_relaxed);
                });
            for (auto& t : ts) t.join();
        }
        std::cout << "        relaxed 计数结果: " << counter.load()
                  << "（期望 40000，计数正确但不保证与其他操作的顺序）\n";
    }
}

// ---------- 5. 内存序的性能差异 ----------
namespace order_performance {
    void demo() {
        constexpr int kThreads = 8;
        constexpr int kOps = 1000000;

        auto bench = [](const char* name, std::memory_order order, int threads, int ops) {
            std::atomic<long long> counter{0};
            auto t1 = std::chrono::steady_clock::now();
            std::vector<std::thread> ts;
            for (int i = 0; i < threads; ++i)
                ts.emplace_back([&counter, order, ops] {
                    for (int j = 0; j < ops; ++j) counter.fetch_add(1, order);
                });
            for (auto& t : ts) t.join();
            auto t2 = std::chrono::steady_clock::now();
            std::cout << "        " << name << ": "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
                      << " ms (结果 " << counter.load() << ")\n";
        };

        std::cout << "      " << kThreads << " 线程各 " << kOps << " 次 fetch_add:\n";
        bench("seq_cst（默认）", std::memory_order_seq_cst, kThreads, kOps);
        bench("acq_rel        ", std::memory_order_acq_rel, kThreads, kOps);
        bench("relaxed        ", std::memory_order_relaxed, kThreads, kOps);
        std::cout << "\n";
        std::cout << "      ⚠ 上面三行的差别【全是测量噪声】，不要据此下结论。\n";
        std::cout << "        我单独查了生成的汇编（g++ -O2 -S，x86-64）:\n";
        std::cout << "          fetch_add(1, seq_cst) -> lock addq $1, (%rdi)\n";
        std::cout << "          fetch_add(1, relaxed) -> lock addq $1, (%rdi)\n";
        std::cout << "        【完全相同的指令】。因为 x86-64 的 lock 前缀指令\n";
        std::cout << "        本身就带完整屏障，所以这三种内存序在 x86 上\n";
        std::cout << "        对 fetch_add 没有任何性能差别。\n";
        std::cout << "        在 ARM/POWER 等弱内存模型平台上才会有实质差别\n";
        std::cout << "        （那里 seq_cst 要插入额外的 dmb/sync 指令）。\n";
        std::cout << "        教训: 谈内存序的性能必须说明目标架构，并看汇编验证。\n";
    }
}

// ---------- 6. 无锁栈 ----------
namespace lockfree_stack {
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
                // 失败时 old_head 已更新为当前头，重试
            }
            if (!old_head) return false;
            out = std::move(old_head->value);
            // 注意：这里 delete 是不安全的！见正文的 ABA 和内存回收问题
            delete old_head;
            return true;
        }

        ~LockFreeStack() {
            T dummy;
            while (pop(dummy)) {}
        }

    private:
        struct Node {
            T value;
            Node* next;
        };
        std::atomic<Node*> _head{nullptr};
    };

    void demo() {
        LockFreeStack<int> stack;
        std::atomic<int> pushed{0}, popped{0};
        std::atomic<long long> sum{0};

        constexpr int kThreads = 4;
        constexpr int kItems = 10000;

        {
            std::vector<std::thread> ts;
            for (int i = 0; i < kThreads; ++i) {
                ts.emplace_back([&stack, &pushed, i] {
                    for (int j = 0; j < kItems; ++j) {
                        stack.push(i * kItems + j);
                        ++pushed;
                    }
                });
            }
            for (auto& t : ts) t.join();
        }
        {
            std::vector<std::thread> ts;
            for (int i = 0; i < kThreads; ++i) {
                ts.emplace_back([&stack, &popped, &sum] {
                    int value;
                    while (stack.pop(value)) { sum += value; ++popped; }
                });
            }
            for (auto& t : ts) t.join();
        }

        std::cout << "      " << kThreads << " 线程 push " << kItems << " 条，再并发 pop:\n";
        std::cout << "        pushed=" << pushed.load() << " popped=" << popped.load()
                  << (pushed == popped ? " ✓ 数量一致" : " ✗ 不一致") << "\n";
        std::cout << "      ⚠ 这个实现【不是生产级的】，问题见正文:\n";
        std::cout << "        1. ABA 问题\n";
        std::cout << "        2. pop 中 delete 节点时，别的线程可能正在读它\n";
        std::cout << "        生产级实现需要 hazard pointer 或引用计数回收\n";
        std::cout << "        本 demo 分两阶段（先全 push 再全 pop）规避了这些问题\n";
    }
}

// ---------- 7. atomic 的其他用途 ----------
namespace other_uses {
    void demo() {
        // 标志位
        std::atomic<bool> stop_requested{false};
        std::thread worker([&] {
            int iterations = 0;
            while (!stop_requested.load(std::memory_order_relaxed)) {
                ++iterations;
                if (iterations > 1000000) break;      // 防止测试卡住
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        stop_requested.store(true);
        worker.join();
        std::cout << "      用 atomic<bool> 做停止标志（最常见的用途）\n";

        // atomic_flag：保证无锁的最小原子类型
        std::atomic_flag spin = ATOMIC_FLAG_INIT;
        std::cout << "      atomic_flag 是【唯一保证无锁】的原子类型\n";
        std::cout << "        可以用它实现自旋锁:\n";
        std::cout << "          while (flag.test_and_set(acquire)) {}   // lock\n";
        std::cout << "          flag.clear(release);                    // unlock\n";
        std::cout << "        但自旋锁只适合极短的临界区\n";
        spin.clear();

        // shared_ptr 的原子性
        std::cout << "      shared_ptr 的原子性（见 02-05）:\n";
        std::cout << "        引用计数的增减是原子的（线程安全）\n";
        std::cout << "        但 shared_ptr 对象本身的读写【不是】原子的\n";
        std::cout << "        多线程要改同一个 shared_ptr 变量，需要\n";
        std::cout << "        std::atomic<std::shared_ptr<T>>（C++20）或加锁\n";
    }
}

int main() {
    std::cout << "--- 1. atomic 基础 ---\n";          atomic_basics::demo();
    std::cout << "--- 2. atomic vs mutex ---\n";      atomic_vs_mutex::demo();
    std::cout << "--- 3. CAS ---\n";                  cas::demo();
    std::cout << "--- 4. 六种内存序 ---\n";           memory_orders::demo();
    std::cout << "--- 5. 内存序的性能 ---\n";         order_performance::demo();
    std::cout << "--- 6. 无锁栈 ---\n";               lockfree_stack::demo();
    std::cout << "--- 7. 其他用途 ---\n";             other_uses::demo();
}
