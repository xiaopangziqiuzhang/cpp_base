// std=c++17 flags=-pthread
// 对应章节: blog/part8_concurrency/08-01_thread.md
// 主题: thread 的创建、join/detach、传参陷阱、thread_local

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex g_print_mutex;      // 仅用于让输出不交错

void safe_print(const std::string& msg) {
    std::lock_guard<std::mutex> lk(g_print_mutex);
    std::cout << msg << "\n";
}

// ---------- 1. 创建线程的几种方式 ----------
namespace creation {
    void free_function(int id) {
        safe_print("        普通函数，收到 id=" + std::to_string(id));
    }

    struct Functor {
        void operator()(int id) const {
            safe_print("        函数对象，收到 id=" + std::to_string(id));
        }
    };

    class Worker {
    public:
        void member_function(int id) const {
            safe_print("        成员函数，收到 id=" + std::to_string(id));
        }
    };

    void demo() {
        std::thread t1(free_function, 1);
        std::thread t2(Functor{}, 2);
        std::thread t3([](int id) {
            safe_print("        lambda，收到 id=" + std::to_string(id));
        }, 3);

        Worker w;
        std::thread t4(&Worker::member_function, &w, 4);   // 成员函数要传对象

        t1.join(); t2.join(); t3.join(); t4.join();

        std::cout << "      四种可调用对象都能作为线程函数（见 06-02 的 INVOKE）\n";
        std::cout << "      硬件并发度 hardware_concurrency() = "
                  << std::thread::hardware_concurrency() << "\n";
    }
}

// ---------- 2. join vs detach ----------
namespace join_detach {
    void demo() {
        std::cout << "      join(): 等待线程结束\n";
        std::thread t1([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            safe_print("        被 join 的线程完成");
        });
        t1.join();
        safe_print("        join() 返回，确定线程已结束");

        std::cout << "      detach(): 分离线程，不再管它\n";
        std::thread t2([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            safe_print("        被 detach 的线程完成（可能在主线程之后）");
        });
        t2.detach();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));   // 等它一下

        std::cout << "      joinable(): 判断是否还需要 join\n";
        std::thread t3([] {});
        std::cout << "        join 之前 joinable = " << t3.joinable() << "\n";
        t3.join();
        std::cout << "        join 之后 joinable = " << t3.joinable() << "\n";

        std::cout << "      ⚠ 关键规则: thread 析构时若仍 joinable -> std::terminate\n";
        std::cout << "        必须在析构前调用 join() 或 detach()\n";
    }
}

// ---------- 3. 传参陷阱 ----------
namespace argument_pitfalls {
    void takes_ref(int& x) { x = 100; }
    void takes_cref(const std::string& s) {
        safe_print("        收到字符串: " + s);
    }

    void demo() {
        // 陷阱一：thread 默认【拷贝】参数
        int value = 1;
        std::thread t1(takes_ref, std::ref(value));    // 必须用 std::ref
        t1.join();
        std::cout << "      用 std::ref 后 value = " << value << "（改到了）\n";
        std::cout << "      不用 std::ref 的话会编译错误（thread 的参数是右值）\n";

        // 陷阱二：悬垂引用
        std::cout << "      陷阱：传局部变量的引用给 detach 的线程\n";
        std::cout << "        void f() { int local = 1; std::thread t(g, std::ref(local));\n";
        std::cout << "                   t.detach(); }  // local 销毁后线程还在用它\n";

        // 陷阱三：隐式转换发生在新线程里
        std::cout << "      陷阱：隐式转换在【新线程】中进行\n";
        {
            char buf[] = "hello";
            std::thread t(takes_cref, std::string(buf));   // 显式转换，安全
            t.join();
            std::cout << "        传 const char* 时，到 string 的转换在新线程里做，\n";
            std::cout << "        如果原指针先失效就会出错 -> 调用方先显式转换\n";
        }

        // 陷阱四：移动语义
        auto ptr = std::make_unique<int>(42);
        std::thread t2([p = std::move(ptr)] {           // 初始化捕获（见 06-01）
            safe_print("        线程拿到了 unique_ptr，值=" + std::to_string(*p));
        });
        t2.join();
        std::cout << "      只能移动的对象用初始化捕获或 std::move 传入\n";
    }
}

// ---------- 4. RAII 包装：thread_guard ----------
namespace raii_thread {
    class ThreadGuard {
    public:
        explicit ThreadGuard(std::thread t) : _t(std::move(t)) {}
        ~ThreadGuard() { if (_t.joinable()) _t.join(); }

        ThreadGuard(const ThreadGuard&) = delete;
        ThreadGuard& operator=(const ThreadGuard&) = delete;
    private:
        std::thread _t;
    };

    void demo() {
        {
            ThreadGuard g(std::thread([] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                safe_print("        ThreadGuard 管理的线程完成");
            }));
            safe_print("        主线程继续做别的事");
        }   // 析构自动 join
        safe_print("        ThreadGuard 析构后，确定线程已结束");

        std::cout << "      即使中途抛异常，析构也会 join（见 02-06 的 RAII）\n";
        std::cout << "      C++20 的 std::jthread 内置了这个行为\n";
    }
}

// ---------- 5. 数据竞争的演示 ----------
namespace data_race {
    void demo() {
        constexpr int kThreads = 8;
        constexpr int kIncrements = 100000;

        // 无保护：有数据竞争
        int unsafe_counter = 0;
        {
            std::vector<std::thread> threads;
            for (int i = 0; i < kThreads; ++i) {
                threads.emplace_back([&unsafe_counter] {
                    for (int j = 0; j < kIncrements; ++j) ++unsafe_counter;   // 竞争！
                });
            }
            for (auto& t : threads) t.join();
        }

        // 用 atomic
        std::atomic<int> safe_counter{0};
        {
            std::vector<std::thread> threads;
            for (int i = 0; i < kThreads; ++i) {
                threads.emplace_back([&safe_counter] {
                    for (int j = 0; j < kIncrements; ++j) ++safe_counter;
                });
            }
            for (auto& t : threads) t.join();
        }

        const int expected = kThreads * kIncrements;
        std::cout << "      " << kThreads << " 线程各自 ++ " << kIncrements << " 次\n";
        std::cout << "        期望值:       " << expected << "\n";
        std::cout << "        无保护的结果: " << unsafe_counter
                  << (unsafe_counter == expected ? "" : "  <- 丢失了更新！") << "\n";
        std::cout << "        atomic 的结果: " << safe_counter.load() << "\n";
        std::cout << "      ++counter 不是原子的：它是【读-改-写】三步\n";
        std::cout << "      两个线程同时读到同一个值，就会丢失一次更新\n";
    }
}

// ---------- 6. thread_local ----------
namespace thread_local_demo {
    thread_local int tls_counter = 0;        // 每个线程一份
    int shared_counter = 0;                   // 所有线程共享

    void work(int id) {
        for (int i = 0; i < 1000; ++i) ++tls_counter;
        std::lock_guard<std::mutex> lk(g_print_mutex);
        std::cout << "        线程 " << id << " 的 tls_counter = " << tls_counter << "\n";
    }

    void demo() {
        std::vector<std::thread> threads;
        for (int i = 0; i < 3; ++i) threads.emplace_back(work, i);
        for (auto& t : threads) t.join();

        std::cout << "      主线程的 tls_counter = " << tls_counter
                  << "（独立的一份）\n";
        std::cout << "      用途: 线程私有的缓存、随机数引擎（见 07-05）、错误码\n";
        std::cout << "      注意: thread_local 对象在线程结束时析构\n";
        (void)shared_counter;
    }
}

// ---------- 7. 线程的其他操作 ----------
namespace misc {
    void demo() {
        std::cout << "      当前线程 id: " << std::this_thread::get_id() << "\n";

        std::thread t([] {
            std::cout << "        子线程 id: " << std::this_thread::get_id() << "\n";
        });
        std::cout << "      通过 thread 对象获取 id: " << t.get_id() << "\n";
        t.join();

        std::cout << "      sleep_for / sleep_until:\n";
        auto t1 = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        auto t2 = std::chrono::steady_clock::now();
        std::cout << "        sleep_for(15ms) 实际睡了 "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
                  << " ms（有调度误差）\n";

        std::cout << "      yield(): 主动让出 CPU，提示调度器切换\n";
        std::this_thread::yield();

        std::cout << "      thread 不可拷贝，只可移动（它表示唯一的执行资源）\n";
        std::thread a([] {});
        std::thread b = std::move(a);       // ok
        // std::thread c = b;                // 编译错误
        std::cout << "        移动后原对象 joinable = " << a.joinable() << "\n";
        b.join();
    }
}

int main() {
    std::cout << "--- 1. 创建线程 ---\n";       creation::demo();
    std::cout << "--- 2. join vs detach ---\n"; join_detach::demo();
    std::cout << "--- 3. 传参陷阱 ---\n";       argument_pitfalls::demo();
    std::cout << "--- 4. RAII 包装 ---\n";      raii_thread::demo();
    std::cout << "--- 5. 数据竞争 ---\n";       data_race::demo();
    std::cout << "--- 6. thread_local ---\n";   thread_local_demo::demo();
    std::cout << "--- 7. 其他操作 ---\n";       misc::demo();
}
