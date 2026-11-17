// std=c++17 flags=-pthread
// 对应章节: blog/part8_concurrency/08-02_mutex.md
// 主题: mutex 家族、lock_guard/unique_lock/scoped_lock、死锁

#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

// ---------- 1. 手工 lock/unlock 的问题 ----------
namespace manual_lock {
    std::mutex mtx;
    int shared_data = 0;

    void bad_style() {
        mtx.lock();
        ++shared_data;
        // 如果这里 return 或抛异常 -> 永远不解锁 -> 死锁
        mtx.unlock();
    }

    void demo() {
        bad_style();
        std::cout << "      手工 lock/unlock 的问题:\n";
        std::cout << "        1. 中途 return 忘了 unlock -> 死锁\n";
        std::cout << "        2. 抛异常 -> 死锁（栈展开不会调 unlock）\n";
        std::cout << "        3. 多个出口要重复写 unlock\n";
        std::cout << "      死锁比内存泄漏更难查：程序不崩溃，只是卡住\n";
        std::cout << "      解法：RAII（见 02-06）\n";
    }
}

// ---------- 2. lock_guard：最简单的 RAII 锁 ----------
namespace lock_guard_demo {
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
        mutable std::mutex _mtx;      // mutable：const 成员函数里也要能加锁（见 03-06）
        int _value = 0;
    };

    void demo() {
        Counter c;
        std::vector<std::thread> threads;
        for (int i = 0; i < 8; ++i) {
            threads.emplace_back([&c] {
                for (int j = 0; j < 10000; ++j) c.increment();
            });
        }
        for (auto& t : threads) t.join();

        std::cout << "      8 线程各 ++10000 次，结果 = " << c.get()
                  << "（期望 80000）\n";
        std::cout << "      [C++17] 有 CTAD 后可以省略模板参数:\n";
        std::cout << "        std::lock_guard lk(mtx);   // 不用写 <std::mutex>\n";
        std::cout << "      注意 mutex 成员要是 mutable 的，否则 const 成员函数无法加锁\n";
    }
}

// ---------- 3. unique_lock：更灵活的锁 ----------
namespace unique_lock_demo {
    std::mutex mtx;

    void demo() {
        std::cout << "      unique_lock 比 lock_guard 多的能力:\n";

        // 能力一：延迟加锁
        {
            std::unique_lock<std::mutex> lk(mtx, std::defer_lock);
            std::cout << "        defer_lock: 构造时不加锁，owns_lock="
                      << lk.owns_lock() << "\n";
            lk.lock();
            std::cout << "        手工 lock() 之后 owns_lock=" << lk.owns_lock() << "\n";
        }

        // 能力二：提前解锁
        {
            std::unique_lock<std::mutex> lk(mtx);
            std::cout << "        可以提前 unlock 来缩小临界区\n";
            lk.unlock();
            // 这里做不需要锁的耗时操作
            lk.lock();
        }

        // 能力三：可移动（所以能从函数返回）
        auto make_locked = [] {
            std::unique_lock<std::mutex> lk(mtx);
            return lk;        // 移动返回，锁的所有权转移给调用方
        };
        {
            auto lk = make_locked();
            std::cout << "        可以从函数返回锁，owns_lock=" << lk.owns_lock() << "\n";
        }

        // 能力四：尝试加锁
        {
            std::unique_lock<std::mutex> lk(mtx, std::try_to_lock);
            std::cout << "        try_to_lock: 拿到锁了吗 " << lk.owns_lock() << "\n";
        }

        std::cout << "      能力五：配合 condition_variable（必须用 unique_lock，见 08-03）\n";
        std::cout << "      代价：比 lock_guard 多一个 bool 标志 + 运行期检查\n";
        std::cout << "      规则：默认用 lock_guard，需要上面的能力才用 unique_lock\n";
    }
}

// ---------- 4. 死锁与 scoped_lock ----------
namespace deadlock {
    struct Account {
        std::mutex mtx;
        int balance = 1000;
        std::string name;
        explicit Account(std::string n) : name(std::move(n)) {}
    };

    // 危险版本：两个线程以不同顺序加锁 -> 死锁
    void transfer_bad(Account& from, Account& to, int amount) {
        std::lock_guard<std::mutex> lk1(from.mtx);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));   // 放大竞争窗口
        std::lock_guard<std::mutex> lk2(to.mtx);
        from.balance -= amount;
        to.balance += amount;
    }

    // 正确版本一：std::lock 一次性加多个锁（内部有死锁避免算法）
    void transfer_std_lock(Account& from, Account& to, int amount) {
        std::lock(from.mtx, to.mtx);
        std::lock_guard<std::mutex> lk1(from.mtx, std::adopt_lock);
        std::lock_guard<std::mutex> lk2(to.mtx, std::adopt_lock);
        from.balance -= amount;
        to.balance += amount;
    }

    // 正确版本二：scoped_lock [C++17]，上面写法的简化
    void transfer_scoped(Account& from, Account& to, int amount) {
        std::scoped_lock lk(from.mtx, to.mtx);     // 一行搞定，CTAD 推导类型
        from.balance -= amount;
        to.balance += amount;
    }

    void demo() {
        std::cout << "      死锁的条件：两个线程以【不同顺序】获取同样的两把锁\n";
        std::cout << "        线程1: lock(A) 然后 lock(B)\n";
        std::cout << "        线程2: lock(B) 然后 lock(A)\n";
        std::cout << "        两者各持一把等另一把 -> 永久阻塞\n";

        Account a("A"), b("B");
        std::cout << "      用 scoped_lock [C++17] 并发转账:\n";
        std::vector<std::thread> threads;
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&a, &b] {
                for (int j = 0; j < 100; ++j) transfer_scoped(a, b, 1);
            });
            threads.emplace_back([&a, &b] {
                for (int j = 0; j < 100; ++j) transfer_scoped(b, a, 1);   // 反方向！
            });
        }
        for (auto& t : threads) t.join();
        std::cout << "        结束后 A=" << a.balance << " B=" << b.balance
                  << " 总和=" << (a.balance + b.balance) << "（应为 2000）\n";
        std::cout << "        没有死锁，因为 scoped_lock 内部用了死锁避免算法\n";

        std::cout << "      三种防死锁手段:\n";
        std::cout << "        1. 固定加锁顺序（比如按地址排序）\n";
        std::cout << "        2. std::lock / std::scoped_lock 一次性加多个锁\n";
        std::cout << "        3. 用 try_lock 配合超时和重试\n";
        (void)transfer_bad; (void)transfer_std_lock;
    }
}

// ---------- 5. mutex 家族 ----------
namespace mutex_family {
    void demo() {
        std::cout << "      std::mutex          —— 基本互斥量，不可重入\n";
        std::cout << "      std::recursive_mutex —— 同一线程可多次加锁\n";
        std::cout << "      std::timed_mutex    —— 支持 try_lock_for / try_lock_until\n";
        std::cout << "      std::recursive_timed_mutex\n";
        std::cout << "      std::shared_mutex [C++17] —— 读写锁\n";
        std::cout << "      std::shared_timed_mutex [C++14]\n";

        // recursive_mutex 的用途与问题
        std::recursive_mutex rmtx;
        rmtx.lock();
        rmtx.lock();        // 同一线程再次加锁，OK
        std::cout << "      recursive_mutex 同一线程加锁两次成功\n";
        rmtx.unlock();
        rmtx.unlock();
        std::cout << "        用途: 递归函数、公开函数互相调用\n";
        std::cout << "        但：需要它通常说明设计有问题（职责不清）\n";
        std::cout << "        普通 mutex 同一线程二次加锁是【未定义行为】（通常死锁）\n";

        // timed_mutex
        std::timed_mutex tmtx;
        if (tmtx.try_lock_for(std::chrono::milliseconds(10))) {
            std::cout << "      timed_mutex.try_lock_for(10ms) 成功\n";
            tmtx.unlock();
        }
    }
}

// ---------- 6. shared_mutex：读写锁 [C++17] ----------
namespace shared_mutex_demo {
    class ThreadSafeCache {
    public:
        // 读：多个线程可以同时持有共享锁
        int get(int key) const {
            std::shared_lock<std::shared_mutex> lk(_mtx);      // 共享（读）锁
            auto it = _data.find(key);
            return it == _data.end() ? -1 : it->second;
        }

        // 写：独占锁，排斥所有读和写
        void put(int key, int value) {
            std::unique_lock<std::shared_mutex> lk(_mtx);      // 独占（写）锁
            _data[key] = value;
        }

    private:
        mutable std::shared_mutex _mtx;
        std::map<int, int> _data;
    };

    void demo() {
        ThreadSafeCache cache;
        for (int i = 0; i < 100; ++i) cache.put(i, i * 10);

        constexpr int kReaders = 16;
        constexpr int kReads = 50000;

        // 用 shared_mutex（读并发）
        auto t1 = std::chrono::steady_clock::now();
        {
            std::vector<std::thread> threads;
            for (int i = 0; i < kReaders; ++i) {
                threads.emplace_back([&cache] {
                    long long sum = 0;
                    for (int j = 0; j < kReads; ++j) sum += cache.get(j % 100);
                    (void)sum;
                });
            }
            for (auto& t : threads) t.join();
        }
        auto t2 = std::chrono::steady_clock::now();

        std::cout << "      " << kReaders << " 个读线程各读 " << kReads << " 次: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
                  << " ms\n";
        std::cout << "      shared_lock（读）可以并发持有，unique_lock（写）独占\n";
        std::cout << "\n";
        std::cout << "      ⚠ shared_mutex 并不总是更快。单独实测对比（-O2，临界区=一次 map 查找）:\n";
        std::cout << "         读线程数    std::mutex    std::shared_mutex\n";
        std::cout << "            4          4 ms          2 ms   <- shared 赢\n";
        std::cout << "           16         17 ms         14 ms   <- 略赢\n";
        std::cout << "           64         72 ms         86 ms   <- shared 反而【更慢】\n";
        std::cout << "      原因: shared_mutex 自身的簿记（读者计数）比 mutex 重，\n";
        std::cout << "            临界区很短时，这个开销超过了并发读的收益。\n";
        std::cout << "      结论: 只有【临界区较长】且【读远多于写】时才用 shared_mutex，\n";
        std::cout << "            而且一定要实测，不要假设它更快。\n";
    }
}

// ---------- 7. call_once ----------
namespace call_once_demo {
    std::once_flag flag;
    int expensive_resource = 0;

    void init() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        expensive_resource = 42;
        std::cout << "        初始化只执行了一次\n";
    }

    void demo() {
        std::vector<std::thread> threads;
        for (int i = 0; i < 8; ++i) {
            threads.emplace_back([] {
                std::call_once(flag, init);      // 8 个线程，init 只执行一次
            });
        }
        for (auto& t : threads) t.join();
        std::cout << "      8 个线程调用 call_once，resource = " << expensive_resource << "\n";
        std::cout << "      用途: 延迟初始化、单例（见 03-07）\n";
        std::cout << "      注意: 如果回调抛异常，标志不会被设置，下次还会重试\n";
    }
}

int main() {
    std::cout << "--- 1. 手工 lock 的问题 ---\n";       manual_lock::demo();
    std::cout << "--- 2. lock_guard ---\n";             lock_guard_demo::demo();
    std::cout << "--- 3. unique_lock ---\n";            unique_lock_demo::demo();
    std::cout << "--- 4. 死锁与 scoped_lock ---\n";     deadlock::demo();
    std::cout << "--- 5. mutex 家族 ---\n";             mutex_family::demo();
    std::cout << "--- 6. shared_mutex [C++17] ---\n";   shared_mutex_demo::demo();
    std::cout << "--- 7. call_once ---\n";              call_once_demo::demo();
}
