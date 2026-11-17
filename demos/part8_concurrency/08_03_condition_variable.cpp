// std=c++17 flags=-pthread
// 对应章节: blog/part8_concurrency/08-03_condition_variable.md
// 主题: 条件变量、虚假唤醒、生产者消费者队列

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

std::mutex g_cout_mtx;
void log(const std::string& s) {
    std::lock_guard<std::mutex> lk(g_cout_mtx);
    std::cout << s << "\n";
}

// ---------- 1. 没有条件变量时：忙等待 ----------
namespace busy_waiting {
    void demo() {
        std::atomic<bool> ready{false};
        std::atomic<int> spin_count{0};

        std::thread consumer([&] {
            while (!ready.load()) {          // 忙等待：空转消耗 CPU
                ++spin_count;
            }
            log("        消费者看到 ready");
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ready.store(true);
        consumer.join();

        std::cout << "      忙等待空转了 " << spin_count.load() << " 次\n";
        std::cout << "      问题: 1) 浪费 CPU  2) 和其他线程抢 CPU  3) 耗电\n";
        std::cout << "      改进: sleep 一下再查 -> 但延迟和 CPU 占用难以兼顾\n";
        std::cout << "      正解: 条件变量 —— 让线程【睡到】条件满足时才被唤醒\n";
    }
}

// ---------- 2. 条件变量的基本用法 ----------
namespace basic_cv {
    void demo() {
        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;
        std::string data;

        std::thread consumer([&] {
            std::unique_lock<std::mutex> lk(mtx);         // 必须是 unique_lock
            cv.wait(lk, [&ready] { return ready; });       // 带谓词的 wait
            log("        消费者被唤醒，收到: " + data);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        {
            std::lock_guard<std::mutex> lk(mtx);
            data = "hello";
            ready = true;
        }           // 先解锁，再 notify（见正文的讨论）
        cv.notify_one();

        consumer.join();
        std::cout << "      wait 的三步: 1) 解锁 2) 睡眠等待 3) 被唤醒后重新加锁\n";
        std::cout << "      必须用 unique_lock（因为 wait 要解锁再加锁）\n";
    }
}

// ---------- 3. 虚假唤醒与必须用谓词 ----------
namespace spurious_wakeup {
    void demo() {
        std::cout << "      虚假唤醒（spurious wakeup）:\n";
        std::cout << "        wait() 可能在【没有 notify】的情况下返回\n";
        std::cout << "        原因: 1) 操作系统的实现细节（如信号中断）\n";
        std::cout << "              2) notify_all 唤醒多个线程，但只有一个能拿到资源\n";

        std::cout << "      所以【绝不能】这样写:\n";
        std::cout << "        cv.wait(lk);          // 错！被唤醒不代表条件满足\n";
        std::cout << "        use_data();\n";

        std::cout << "      正确写法一：带谓词的 wait（推荐）\n";
        std::cout << "        cv.wait(lk, [&]{ return condition; });\n";

        std::cout << "      正确写法二：手工循环（等价于上面）\n";
        std::cout << "        while (!condition) cv.wait(lk);\n";

        std::cout << "      带谓词的 wait 等价于:\n";
        std::cout << "        while (!pred()) wait(lk);\n";
        std::cout << "      所以即使虚假唤醒，也会重新检查条件后继续睡\n";
    }
}

// ---------- 4. 生产者消费者队列（完整实现）----------
namespace producer_consumer {
    template <class T>
    class BlockingQueue {
    public:
        explicit BlockingQueue(std::size_t max_size = 100) : _max_size(max_size) {}

        // 阻塞入队：队列满时等待
        void push(T value) {
            std::unique_lock<std::mutex> lk(_mtx);
            _not_full.wait(lk, [this] { return _queue.size() < _max_size || _closed; });
            if (_closed) return;
            _queue.push(std::move(value));
            lk.unlock();                    // 先解锁再通知，避免被唤醒的线程立刻又阻塞
            _not_empty.notify_one();
        }

        // 阻塞出队：队列空时等待；队列关闭且空时返回 nullopt
        std::optional<T> pop() {
            std::unique_lock<std::mutex> lk(_mtx);
            _not_empty.wait(lk, [this] { return !_queue.empty() || _closed; });
            if (_queue.empty()) return std::nullopt;        // 已关闭
            T value = std::move(_queue.front());
            _queue.pop();
            lk.unlock();
            _not_full.notify_one();
            return value;
        }

        // 关闭队列，唤醒所有等待者
        void close() {
            {
                std::lock_guard<std::mutex> lk(_mtx);
                _closed = true;
            }
            _not_empty.notify_all();        // 必须 notify_all
            _not_full.notify_all();
        }

        std::size_t size() const {
            std::lock_guard<std::mutex> lk(_mtx);
            return _queue.size();
        }

    private:
        mutable std::mutex _mtx;
        std::condition_variable _not_empty;
        std::condition_variable _not_full;
        std::queue<T> _queue;
        std::size_t _max_size;
        bool _closed = false;
    };

    void demo() {
        BlockingQueue<int> queue(10);         // 容量 10，会触发"队列满"
        std::atomic<int> consumed{0};
        std::atomic<long long> sum{0};

        constexpr int kProducers = 3;
        constexpr int kConsumers = 4;
        constexpr int kItemsPerProducer = 500;

        std::vector<std::thread> producers;
        for (int p = 0; p < kProducers; ++p) {
            producers.emplace_back([&queue, p] {
                for (int i = 0; i < kItemsPerProducer; ++i) {
                    queue.push(p * 1000 + i);
                }
            });
        }

        std::vector<std::thread> consumers;
        for (int c = 0; c < kConsumers; ++c) {
            consumers.emplace_back([&queue, &consumed, &sum] {
                while (auto item = queue.pop()) {
                    sum += *item;
                    ++consumed;
                }
            });
        }

        for (auto& t : producers) t.join();      // 等生产者全部完成
        queue.close();                            // 关闭队列，让消费者退出
        for (auto& t : consumers) t.join();

        const int expected = kProducers * kItemsPerProducer;
        std::cout << "      " << kProducers << " 生产者 x " << kItemsPerProducer
                  << " 条，" << kConsumers << " 消费者\n";
        std::cout << "        消费总数: " << consumed.load()
                  << "（期望 " << expected << "）"
                  << (consumed == expected ? " ✓" : " ✗") << "\n";
        std::cout << "        队列残留: " << queue.size() << "\n";
        std::cout << "      要点: 两个条件变量（非空/非满）、close() 用 notify_all\n";
    }
}

// ---------- 5. notify_one vs notify_all ----------
namespace notify_choice {
    void demo() {
        std::cout << "      notify_one: 唤醒【一个】等待的线程（哪个不确定）\n";
        std::cout << "      notify_all: 唤醒【所有】等待的线程\n";

        std::cout << "      什么时候用 notify_one:\n";
        std::cout << "        所有等待者都在等同一个条件，而且资源只够一个用\n";
        std::cout << "        例: 队列入队一个元素 -> 只需唤醒一个消费者\n";

        std::cout << "      什么时候用 notify_all:\n";
        std::cout << "        1. 等待者在等【不同】的条件\n";
        std::cout << "        2. 状态变化让【所有】等待者都应该重新检查（如 close）\n";
        std::cout << "        3. 一次性放出多个资源\n";

        std::cout << "      误用的后果:\n";
        std::cout << "        该用 all 却用了 one -> 有线程永远不被唤醒（挂起）\n";
        std::cout << "        该用 one 却用了 all -> 惊群效应，性能损失但不会错\n";
        std::cout << "      不确定时用 notify_all（正确性优先）\n";
    }
}

// ---------- 6. 先解锁还是先 notify ----------
namespace notify_timing {
    void demo() {
        std::cout << "      两种写法:\n";
        std::cout << "        写法 A（持锁通知）:\n";
        std::cout << "          { lock_guard lk(mtx); data = x; ready = true;\n";
        std::cout << "            cv.notify_one(); }\n";
        std::cout << "        写法 B（先解锁再通知）:\n";
        std::cout << "          { lock_guard lk(mtx); data = x; ready = true; }\n";
        std::cout << "          cv.notify_one();\n";

        std::cout << "      两者都【正确】，差别在性能:\n";
        std::cout << "        写法 A: 被唤醒的线程要等通知者释放锁，可能立刻又阻塞\n";
        std::cout << "                （这叫 hurry-up-and-wait）\n";
        std::cout << "        写法 B: 被唤醒时锁已空闲，可以直接继续\n";
        std::cout << "      所以通常推荐【写法 B】\n";
        std::cout << "      但注意: 如果 cv 对象本身的生命周期依赖那把锁，就必须用 A\n";
    }
}

// ---------- 7. wait_for / wait_until：带超时 ----------
namespace timed_wait {
    void demo() {
        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;

        auto t1 = std::chrono::steady_clock::now();
        {
            std::unique_lock<std::mutex> lk(mtx);
            bool got = cv.wait_for(lk, std::chrono::milliseconds(30),
                                   [&ready] { return ready; });
            auto t2 = std::chrono::steady_clock::now();
            std::cout << "      wait_for(30ms) 超时返回，等了 "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
                      << " ms，谓词结果 = " << got << "\n";
        }

        // 带谓词的 wait_for 返回谓词的值，不需要再判断是否超时
        std::cout << "      带谓词的 wait_for 返回【谓词的值】:\n";
        std::cout << "        true  = 条件满足\n";
        std::cout << "        false = 超时且条件仍不满足\n";
        std::cout << "      不带谓词的版本返回 cv_status::timeout / no_timeout\n";
        std::cout << "      用途: 心跳检测、超时重试、优雅退出\n";
    }
}

int main() {
    std::cout << "--- 1. 忙等待的问题 ---\n";      busy_waiting::demo();
    std::cout << "--- 2. 基本用法 ---\n";          basic_cv::demo();
    std::cout << "--- 3. 虚假唤醒 ---\n";          spurious_wakeup::demo();
    std::cout << "--- 4. 生产者消费者队列 ---\n";  producer_consumer::demo();
    std::cout << "--- 5. one vs all ---\n";        notify_choice::demo();
    std::cout << "--- 6. 通知时机 ---\n";          notify_timing::demo();
    std::cout << "--- 7. 带超时的 wait ---\n";     timed_wait::demo();
}
