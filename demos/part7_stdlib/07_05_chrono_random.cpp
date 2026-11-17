// std=c++17
// 对应章节: blog/part7_stdlib/07-05_chrono_random.md
// 主题: chrono 的三个概念、计时惯用法、随机数引擎与分布

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <thread>
#include <vector>

// ---------- 1. C 时间 API 的问题 ----------
namespace c_time_problems {
    void demo() {
        std::cout << "      C 的做法:\n";
        std::cout << "        clock()      —— 单位是 CLOCKS_PER_SEC，平台相关\n";
        std::cout << "        time()       —— 只有秒级精度\n";
        std::cout << "        gettimeofday —— 微秒，但不跨平台\n";
        std::cout << "      核心问题：【时间单位不在类型里】\n";
        std::cout << "        void sleep_for(int duration);   // 毫秒？秒？微秒？\n";
        std::cout << "        传错单位不报错，只是行为不对\n";

        std::clock_t c = std::clock();
        std::cout << "      clock() = " << c
                  << ", CLOCKS_PER_SEC = " << CLOCKS_PER_SEC << "\n";
    }
}

// ---------- 2. chrono 的三个核心概念 ----------
namespace chrono_concepts {
    void demo() {
        using namespace std::chrono;

        std::cout << "      概念一：duration（时长）—— 一段时间\n";
        milliseconds ms(500);
        seconds s(2);
        auto total = ms + s;        // 自动统一到毫秒
        std::cout << "        500ms + 2s = " << total.count() << " ms\n";
        std::cout << "        duration 的类型里编码了单位，混用会自动换算\n";

        std::cout << "      概念二：time_point（时间点）—— 某个时刻\n";
        auto now = steady_clock::now();
        auto later = now + seconds(1);
        std::cout << "        time_point 相减得到 duration: "
                  << duration_cast<milliseconds>(later - now).count() << " ms\n";

        std::cout << "      概念三：clock（时钟）—— 提供 now()\n";
        std::cout << "        system_clock:          墙上时钟，可被调整，能转 time_t\n";
        std::cout << "        steady_clock:          单调递增，【计时专用】\n";
        std::cout << "        high_resolution_clock: 精度最高，通常是上面之一的别名\n";
        std::cout << "        steady_clock 是单调的吗: " << steady_clock::is_steady << "\n";
        std::cout << "        system_clock 是单调的吗: " << system_clock::is_steady << "\n";
    }
}

// ---------- 3. duration 的单位换算 ----------
namespace duration_units {
    void demo() {
        using namespace std::chrono;
        using namespace std::chrono_literals;

        auto d = 1h + 30min + 45s;
        std::cout << "      1h + 30min + 45s:\n";
        std::cout << "        = " << duration_cast<seconds>(d).count() << " 秒\n";
        std::cout << "        = " << duration_cast<minutes>(d).count()
                  << " 分（整数除法会截断）\n";

        // 精确换算要用浮点
        using fmin = duration<double, std::ratio<60>>;
        std::cout << "        = " << duration_cast<fmin>(d).count() << " 分（浮点，精确）\n";

        std::cout << "      自动换算（无损时隐式，有损时要显式）:\n";
        milliseconds from_sec = 2s;         // 秒 -> 毫秒，无损，隐式
        std::cout << "        2s 隐式转毫秒 = " << from_sec.count() << " ms\n";
        // seconds from_ms = 1500ms;        // 编译错误：有损，要显式
        auto to_sec = duration_cast<seconds>(1500ms);
        std::cout << "        1500ms 显式转秒 = " << to_sec.count()
                  << " s（截断了 500ms）\n";
        std::cout << "      这个设计防止了意外的精度丢失\n";

        std::cout << "      duration 的底层是 duration<Rep, Period>:\n";
        std::cout << "        seconds      = duration<long long, ratio<1>>\n";
        std::cout << "        milliseconds = duration<long long, ratio<1,1000>>\n";
        std::cout << "        自定义: duration<double, ratio<1,3>> 表示 1/3 秒为单位\n";
    }
}

// ---------- 4. 计时的正确做法 ----------
namespace timing {
    void demo() {
        using namespace std::chrono;

        // 正确：用 steady_clock
        auto t1 = steady_clock::now();
        volatile long long sum = 0;
        for (int i = 0; i < 3000000; ++i) sum += i;
        auto t2 = steady_clock::now();

        std::cout << "      耗时: "
                  << duration_cast<microseconds>(t2 - t1).count() << " us = "
                  << duration<double, std::milli>(t2 - t1).count() << " ms\n";

        std::cout << "      为什么用 steady_clock 而不是 system_clock:\n";
        std::cout << "        system_clock 可能被 NTP 校正、被用户修改、有夏令时跳变\n";
        std::cout << "        计时期间被调整，差值就是错的（甚至是负的）\n";

        // 一个可复用的计时器
        class Timer {
        public:
            Timer() : _start(steady_clock::now()) {}
            double elapsed_ms() const {
                return duration<double, std::milli>(steady_clock::now() - _start).count();
            }
            void reset() { _start = steady_clock::now(); }
        private:
            steady_clock::time_point _start;
        };

        Timer timer;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::cout << "      Timer 类测 sleep_for(10ms): "
                  << timer.elapsed_ms() << " ms\n";
    }
}

// ---------- 5. system_clock 与日历时间 ----------
namespace calendar {
    void demo() {
        using namespace std::chrono;

        auto now = system_clock::now();
        std::time_t t = system_clock::to_time_t(now);

        std::cout << "      当前时间: " << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S")
                  << "\n";

        auto since_epoch = now.time_since_epoch();
        std::cout << "      Unix 时间戳(秒): "
                  << duration_cast<seconds>(since_epoch).count() << "\n";
        std::cout << "      Unix 时间戳(毫秒): "
                  << duration_cast<milliseconds>(since_epoch).count() << "\n";

        std::cout << "      只有 system_clock 能转 time_t（因为它对应墙上时钟）\n";
        std::cout << "      C++20 才有完整的日历和时区支持（year_month_day、zoned_time）\n";
    }
}

// ---------- 6. 为什么不用 rand() ----------
namespace why_not_rand {
    void demo() {
        std::cout << "      rand() 的问题:\n";
        std::cout << "        1. 质量差 —— 低位的随机性特别糟\n";
        std::cout << "        2. RAND_MAX 可能只有 32767（本机 " << RAND_MAX << "）\n";
        std::cout << "        3. rand() % n 有【取模偏差】\n";
        std::cout << "        4. 全局状态，不是线程安全的\n";
        std::cout << "        5. 不可重现（实现相关）\n";

        // 演示取模偏差。注意本机 RAND_MAX 是 2147483647（31 位），
        // 偏差很小不易观察；但标准只保证 RAND_MAX >= 32767，
        // 在 RAND_MAX 小的平台上偏差会很显著。
        std::cout << "      取模偏差的原理（用 RAND_MAX=32767 举例，便于看清）:\n";
        std::cout << "        要生成 [0,10000)，rand() 的值域是 [0,32767]\n";
        std::cout << "        32768 = 3*10000 + 2768\n";
        std::cout << "        所以 0~2767 各有 4 个原值映射到它，2768~9999 只有 3 个\n";
        std::cout << "        于是小的数出现概率高约 33%\n";
        std::cout << "      本机 RAND_MAX = " << RAND_MAX
                  << "，偏差很小，但原理相同且无法消除\n";

        // 实测本机的偏差幅度
        constexpr int kBuckets = 3;
        const long long span = static_cast<long long>(RAND_MAX) + 1;
        std::cout << "      本机取模 " << kBuckets << " 的理论偏差: "
                  << (span % kBuckets == 0 ? "无偏差（整除）" : "有偏差") << "\n";
    }
}

// ---------- 7. C++11 随机数：引擎 + 分布 ----------
namespace random_proper {
    void demo() {
        std::cout << "      设计：把\"产生随机位\"和\"映射到分布\"分开\n";

        // 引擎：产生均匀分布的随机位
        std::mt19937 gen(42);          // 固定种子，可重现
        std::cout << "      引擎 mt19937(42) 前三个值: "
                  << gen() << " " << gen() << " " << gen() << "\n";

        // 真随机种子
        std::random_device rd;
        std::mt19937 gen2(rd());
        std::cout << "      random_device 提供真随机种子（可能较慢）\n";
        std::cout << "        rd.entropy() = " << rd.entropy() << "\n";

        // 分布：把均匀的位映射到想要的分布
        std::mt19937 g(42);
        std::uniform_int_distribution<int> dice(1, 6);
        std::cout << "      均匀整数分布 [1,6] 掷骰子 10 次: ";
        for (int i = 0; i < 10; ++i) std::cout << dice(g) << " ";
        std::cout << "\n";

        std::uniform_real_distribution<double> unit(0.0, 1.0);
        std::cout << "      均匀实数分布 [0,1): ";
        for (int i = 0; i < 3; ++i) std::cout << std::fixed << std::setprecision(4)
                                              << unit(g) << " ";
        std::cout << "\n";

        std::normal_distribution<double> normal(100.0, 15.0);    // 均值 100，标准差 15
        std::cout << "      正态分布 N(100,15): ";
        for (int i = 0; i < 3; ++i) std::cout << normal(g) << " ";
        std::cout << "\n";

        std::bernoulli_distribution coin(0.5);
        std::cout << "      伯努利分布（抛硬币）: ";
        for (int i = 0; i < 10; ++i) std::cout << (coin(g) ? "H" : "T");
        std::cout << "\n";

        std::cout << std::defaultfloat;
    }
}

// ---------- 8. 验证分布的正确性 ----------
namespace verify_distribution {
    void demo() {
        std::mt19937 gen(12345);
        std::uniform_int_distribution<int> dice(1, 6);

        std::map<int, int> counts;
        constexpr int kRolls = 600000;
        for (int i = 0; i < kRolls; ++i) ++counts[dice(gen)];

        std::cout << "      掷 " << kRolls << " 次骰子的分布（期望各 100000）:\n";
        for (const auto& [face, count] : counts) {
            double pct = 100.0 * count / kRolls;
            std::cout << "        " << face << ": " << count
                      << " (" << std::fixed << std::setprecision(2) << pct << "%)\n";
        }
        std::cout << std::defaultfloat;
        std::cout << "      各面接近 16.67%，没有取模偏差\n";
    }
}

// ---------- 9. 实用模式 ----------
namespace patterns {
    // 线程局部的引擎，避免竞争且避免每次构造
    std::mt19937& thread_local_engine() {
        static thread_local std::mt19937 gen(std::random_device{}());
        return gen;
    }

    int random_int(int lo, int hi) {
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(thread_local_engine());
    }

    void demo() {
        std::cout << "      惯用法一：thread_local 引擎（线程安全 + 不重复构造）\n";
        std::cout << "        random_int(1,100) 五次: ";
        for (int i = 0; i < 5; ++i) std::cout << random_int(1, 100) << " ";
        std::cout << "\n";

        std::cout << "      惯用法二：测试用固定种子，生产用 random_device\n";
        std::cout << "        固定种子保证测试可重现\n";

        std::cout << "      惯用法三：分布对象可以复用（构造有开销）\n";
        std::cout << "        不要在循环里反复构造 distribution\n";

        std::cout << "      常见引擎对比:\n";
        std::cout << "        mt19937        —— 梅森旋转，质量好，状态 2.5KB，默认选它\n";
        std::cout << "        mt19937_64     —— 64 位版本\n";
        std::cout << "        minstd_rand    —— 线性同余，快但质量一般\n";
        std::cout << "        ranlux48       —— 质量最好但最慢\n";
        std::cout << "        default_random_engine —— 实现定义，不保证跨平台一致\n";
    }
}

int main() {
    std::cout << "--- 1. C 时间 API 的问题 ---\n";   c_time_problems::demo();
    std::cout << "--- 2. chrono 三个概念 ---\n";     chrono_concepts::demo();
    std::cout << "--- 3. 单位换算 ---\n";            duration_units::demo();
    std::cout << "--- 4. 计时的正确做法 ---\n";      timing::demo();
    std::cout << "--- 5. 日历时间 ---\n";            calendar::demo();
    std::cout << "--- 6. 为什么不用 rand() ---\n";   why_not_rand::demo();
    std::cout << "--- 7. 引擎 + 分布 ---\n";         random_proper::demo();
    std::cout << "--- 8. 验证分布 ---\n";            verify_distribution::demo();
    std::cout << "--- 9. 实用模式 ---\n";            patterns::demo();
}
