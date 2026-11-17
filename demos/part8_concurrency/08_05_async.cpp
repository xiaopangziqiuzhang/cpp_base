// std=c++17 flags=-pthread
// 对应章节: blog/part8_concurrency/08-05_async.md
// 主题: future/promise/packaged_task/async、线程池

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

// ---------- 1. 用裸线程取回结果有多麻烦 ----------
namespace without_future {
    void demo() {
        std::cout << "      用裸 thread 取回计算结果，要自己处理:\n";
        std::cout << "        1. 结果存哪里（共享变量 + 锁，或出参）\n";
        std::cout << "        2. 怎么知道算完了（条件变量 / 标志位）\n";
        std::cout << "        3. 异常怎么传回来（thread 中未捕获的异常会 terminate！）\n";

        // 手工版本
        int result = 0;
        std::exception_ptr error;
        std::thread t([&result, &error] {
            try {
                result = 42;
            } catch (...) {
                error = std::current_exception();   // 必须手工捕获并保存
            }
        });
        t.join();
        if (error) std::rethrow_exception(error);
        std::cout << "      手工版本拿到结果: " << result << "（代码量是逻辑的几倍）\n";
    }
}

// ---------- 2. std::async：最简单的异步 ----------
namespace async_basics {
    int compute(int x) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        return x * x;
    }

    void demo() {
        auto fut = std::async(std::launch::async, compute, 7);
        std::cout << "      提交任务后主线程可以继续做别的事\n";
        std::cout << "      future.get() 阻塞等待并取回结果: " << fut.get() << "\n";

        // 并行多个任务
        std::vector<std::future<int>> futures;
        auto t1 = std::chrono::steady_clock::now();
        for (int i = 1; i <= 4; ++i) {
            futures.push_back(std::async(std::launch::async, compute, i));
        }
        int sum = 0;
        for (auto& f : futures) sum += f.get();
        auto t2 = std::chrono::steady_clock::now();

        std::cout << "      4 个任务（各 20ms）并行执行，总耗时 "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
                  << " ms（串行要 80ms），结果和 = " << sum << "\n";

        std::cout << "      ⚠ 两种启动策略:\n";
        std::cout << "        launch::async    —— 保证在新线程中执行\n";
        std::cout << "        launch::deferred —— 延迟到 get() 时在【调用线程】执行\n";
        std::cout << "        不指定（默认）   —— 实现自己选，【可能不并行】！\n";
    }
}

// ---------- 3. 默认策略的坑 ----------
namespace launch_policy {
    void demo() {
        // deferred：不会创建线程，get() 时才执行
        auto deferred = std::async(std::launch::deferred, [] {
            return std::this_thread::get_id();
        });
        std::cout << "      主线程 id:        " << std::this_thread::get_id() << "\n";
        std::cout << "      deferred 的执行线程: " << deferred.get()
                  << "（和主线程相同！）\n";

        auto async_task = std::async(std::launch::async, [] {
            return std::this_thread::get_id();
        });
        std::cout << "      async 的执行线程:    " << async_task.get() << "（不同线程）\n";

        std::cout << "      默认策略（async|deferred）的问题:\n";
        std::cout << "        1. 不知道会不会并行 -> 无法保证性能\n";
        std::cout << "        2. 如果是 deferred 而你从不调 get()，任务【永远不执行】\n";
        std::cout << "        3. thread_local 变量的行为不确定（见 08-01）\n";
        std::cout << "      规则: 总是显式写 std::launch::async\n";
    }
}

// ---------- 4. future 的阻塞陷阱 ----------
namespace future_destructor {
    void demo() {
        std::cout << "      ⚠ std::async 返回的 future 在析构时会【阻塞等待】任务完成\n";
        auto t1 = std::chrono::steady_clock::now();
        {
            auto fut = std::async(std::launch::async, [] {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            });
            // 不调用 get()
        }   // future 析构 -> 阻塞 50ms！
        auto t2 = std::chrono::steady_clock::now();
        std::cout << "        不调用 get() 直接离开作用域，耗时 "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
                  << " ms（被 future 的析构函数阻塞了）\n";
        std::cout << "      所以这行代码是【串行】的:\n";
        std::cout << "        std::async(launch::async, f);   // 临时 future 立刻析构\n";
        std::cout << "        std::async(launch::async, g);   // 要等 f 完成才开始\n";
        std::cout << "      必须把 future 存起来才有并行:\n";
        std::cout << "        auto f1 = std::async(...); auto f2 = std::async(...);\n";
    }
}

// ---------- 5. 异常的传递 ----------
namespace exception_propagation {
    int may_throw(bool should_throw) {
        if (should_throw) throw std::runtime_error("任务内部出错");
        return 42;
    }

    void demo() {
        auto fut = std::async(std::launch::async, may_throw, true);
        try {
            fut.get();
        } catch (const std::runtime_error& e) {
            std::cout << "      异常通过 future.get() 传回: " << e.what() << "\n";
        }
        std::cout << "      这是 future 相对裸线程的最大优势之一:\n";
        std::cout << "        thread 中未捕获的异常会调用 std::terminate\n";
        std::cout << "        future 会保存异常，在 get() 时重新抛出\n";
    }
}

// ---------- 6. promise / future 配对 ----------
namespace promise_demo {
    void demo() {
        std::promise<std::string> prom;
        std::future<std::string> fut = prom.get_future();

        std::thread producer([&prom] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            prom.set_value("来自 promise 的数据");
        });

        std::cout << "      future.get() = " << fut.get() << "\n";
        producer.join();

        // promise 也能传异常
        std::promise<int> p2;
        auto f2 = p2.get_future();
        std::thread t2([&p2] {
            try {
                throw std::logic_error("promise 传递的异常");
            } catch (...) {
                p2.set_exception(std::current_exception());
            }
        });
        try {
            f2.get();
        } catch (const std::logic_error& e) {
            std::cout << "      set_exception 传回: " << e.what() << "\n";
        }
        t2.join();

        std::cout << "      promise/future 的分工:\n";
        std::cout << "        promise 是\"写端\"，future 是\"读端\"\n";
        std::cout << "        适合: 结果不是由一个函数返回，而是在某个时刻被设置\n";
        std::cout << "      注意: promise 只能 set 一次；析构时若未 set，\n";
        std::cout << "            future.get() 会抛 broken_promise\n";
    }
}

// ---------- 7. packaged_task ----------
namespace packaged_task_demo {
    void demo() {
        std::packaged_task<int(int, int)> task([](int a, int b) { return a + b; });
        std::future<int> fut = task.get_future();

        std::thread t(std::move(task), 20, 22);     // task 是可移动不可拷贝的
        std::cout << "      packaged_task 的结果: " << fut.get() << "\n";
        t.join();

        std::cout << "      packaged_task = 可调用对象 + future\n";
        std::cout << "      用途: 把任务打包后放进队列，由别的线程执行（线程池的基础）\n";
    }
}

// ---------- 8. 完整的线程池实现 ----------
namespace thread_pool {
    class ThreadPool {
    public:
        explicit ThreadPool(std::size_t threads = std::thread::hardware_concurrency()) {
            for (std::size_t i = 0; i < threads; ++i) {
                _workers.emplace_back([this] { worker_loop(); });
            }
        }

        ~ThreadPool() { shutdown(); }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        // 提交任务，返回 future
        template <class F, class... Args>
        auto submit(F&& f, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>> {
            using Ret = std::invoke_result_t<F, Args...>;

            // 用 packaged_task 包装，并用 shared_ptr 让它能放进 std::function
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

        std::size_t size() const { return _workers.size(); }

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
                task();        // 在锁外执行任务
            }
        }

        std::vector<std::thread> _workers;
        std::queue<std::function<void()>> _tasks;
        mutable std::mutex _mtx;
        std::condition_variable _cv;
        bool _stopped = false;
    };

    void demo() {
        ThreadPool pool(4);
        std::cout << "      线程池大小: " << pool.size() << "\n";

        // 提交一批任务
        std::vector<std::future<long long>> futures;
        for (int i = 1; i <= 8; ++i) {
            futures.push_back(pool.submit([](int n) -> long long {
                long long sum = 0;
                for (int k = 1; k <= n * 100000; ++k) sum += k;
                return sum;
            }, i));
        }

        long long total = 0;
        for (auto& f : futures) total += f.get();
        std::cout << "      8 个任务的结果总和: " << total << "\n";

        // 异常也能正确传回
        auto bad = pool.submit([] { throw std::runtime_error("任务抛异常"); });
        try {
            bad.get();
        } catch (const std::runtime_error& e) {
            std::cout << "      线程池中的异常传回: " << e.what() << "\n";
        }

        // 不同返回类型
        auto f_void = pool.submit([] { /* 返回 void */ });
        auto f_str = pool.submit([](const std::string& s) { return s + "!"; },
                                 std::string("hello"));
        f_void.get();
        std::cout << "      返回 string 的任务: " << f_str.get() << "\n";

        std::cout << "      实现要点:\n";
        std::cout << "        1. packaged_task 包装任意返回类型，统一成 function<void()>\n";
        std::cout << "        2. 用 shared_ptr 包 packaged_task（它不可拷贝，而 function 要求可拷贝）\n";
        std::cout << "        3. 条件变量等待任务（见 08-03）\n";
        std::cout << "        4. 任务在【锁外】执行\n";
        std::cout << "        5. 析构时 shutdown，join 所有线程（见 08-01）\n";
    }
}

// ---------- 9. 并行归约的实际例子 ----------
namespace parallel_reduce {
    void demo() {
        constexpr std::size_t kN = 20000000;
        std::vector<double> data(kN, 1.5);

        // 串行
        auto t1 = std::chrono::steady_clock::now();
        double serial = std::accumulate(data.begin(), data.end(), 0.0);
        auto t2 = std::chrono::steady_clock::now();

        // 用 async 并行归约
        const unsigned threads = std::min(8u, std::thread::hardware_concurrency());
        const std::size_t chunk = kN / threads;
        std::vector<std::future<double>> futures;
        for (unsigned i = 0; i < threads; ++i) {
            auto begin = data.begin() + static_cast<long>(i * chunk);
            auto end = (i == threads - 1) ? data.end()
                                          : data.begin() + static_cast<long>((i + 1) * chunk);
            futures.push_back(std::async(std::launch::async, [begin, end] {
                return std::accumulate(begin, end, 0.0);
            }));
        }
        double parallel = 0;
        for (auto& f : futures) parallel += f.get();
        auto t3 = std::chrono::steady_clock::now();

        auto ms = [](auto a, auto b) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
        };
        std::cout << "      " << kN << " 个 double 求和:\n";
        std::cout << "        串行:              " << ms(t1, t2) << " ms (结果 " << serial << ")\n";
        std::cout << "        " << threads << " 线程并行: " << ms(t2, t3)
                  << " ms (结果 " << parallel << ")\n";
        std::cout << "      这是手工实现的并行归约。注意浮点加法不满足结合律，\n";
        std::cout << "      所以并行结果可能与串行【略有差异】（见 07-03 的 reduce）\n";
    }
}

int main() {
    std::cout << "--- 1. 没有 future 的麻烦 ---\n";    without_future::demo();
    std::cout << "--- 2. std::async ---\n";            async_basics::demo();
    std::cout << "--- 3. 启动策略 ---\n";              launch_policy::demo();
    std::cout << "--- 4. future 析构阻塞 ---\n";       future_destructor::demo();
    std::cout << "--- 5. 异常传递 ---\n";              exception_propagation::demo();
    std::cout << "--- 6. promise/future ---\n";        promise_demo::demo();
    std::cout << "--- 7. packaged_task ---\n";         packaged_task_demo::demo();
    std::cout << "--- 8. 线程池 ---\n";                thread_pool::demo();
    std::cout << "--- 9. 并行归约 ---\n";              parallel_reduce::demo();
}
