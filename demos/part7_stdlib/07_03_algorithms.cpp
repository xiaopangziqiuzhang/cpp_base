// std=c++17 flags=-pthread
// 对应章节: blog/part7_stdlib/07-03_algorithms.md
// 主题: 迭代器分类、常用算法、erase-remove、并行算法

#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <execution>
#include <forward_list>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

// ---------- 1. 迭代器的五种分类 ----------
namespace iterator_categories {
    template <class Iter>
    void report(const char* name) {
        using Cat = typename std::iterator_traits<Iter>::iterator_category;
        std::cout << "        " << name << ": ";
        if (std::is_same_v<Cat, std::random_access_iterator_tag>) std::cout << "随机访问";
        else if (std::is_same_v<Cat, std::bidirectional_iterator_tag>) std::cout << "双向";
        else if (std::is_same_v<Cat, std::forward_iterator_tag>) std::cout << "前向";
        else if (std::is_same_v<Cat, std::input_iterator_tag>) std::cout << "输入";
        else if (std::is_same_v<Cat, std::output_iterator_tag>) std::cout << "输出";
        std::cout << "\n";
    }

    void demo() {
        std::cout << "      五种分类（能力递增）:\n";
        std::cout << "        输入: 单向读一次  输出: 单向写一次\n";
        std::cout << "        前向: 可多次读写  双向: 可 --  随机访问: 可 +n 和比较大小\n";
        std::cout << "      实测各容器的迭代器类别:\n";
        report<std::vector<int>::iterator>("vector       ");
        report<std::deque<int>::iterator>("deque        ");
        report<std::list<int>::iterator>("list         ");
        report<std::forward_list<int>::iterator>("forward_list ");
        report<std::set<int>::iterator>("set          ");
        report<int*>("int*（裸指针）");
        std::cout << "      算法要求的最低类别决定了它能用于哪些容器\n";
        std::cout << "        例: std::sort 要求随机访问，所以 list 不能用（list 有自己的 sort）\n";
    }
}

// ---------- 2. 常用算法分类 ----------
namespace common_algorithms {
    void demo() {
        std::vector<int> v{5, 2, 8, 1, 9, 3, 7};

        std::cout << "      非修改式查询:\n";
        std::cout << "        count_if(偶数) = "
                  << std::count_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; }) << "\n";
        auto it = std::find(v.begin(), v.end(), 8);
        std::cout << "        find(8) 位置 = " << std::distance(v.begin(), it) << "\n";
        std::cout << "        all_of(都>0) = "
                  << std::all_of(v.begin(), v.end(), [](int x) { return x > 0; }) << "\n";
        std::cout << "        max_element = " << *std::max_element(v.begin(), v.end())
                  << ", min_element = " << *std::min_element(v.begin(), v.end()) << "\n";
        auto [mn, mx] = std::minmax_element(v.begin(), v.end());
        std::cout << "        minmax_element = (" << *mn << ", " << *mx << ")\n";

        std::cout << "      修改式操作:\n";
        std::vector<int> dst(v.size());
        std::transform(v.begin(), v.end(), dst.begin(), [](int x) { return x * 2; });
        std::cout << "        transform(x*2): ";
        for (int x : dst) std::cout << x << " ";
        std::cout << "\n";

        std::vector<int> filled(5);
        std::iota(filled.begin(), filled.end(), 10);     // 填 10,11,12,13,14
        std::cout << "        iota(从10递增): ";
        for (int x : filled) std::cout << x << " ";
        std::cout << "\n";

        std::cout << "      排序与划分:\n";
        auto sorted = v;
        std::sort(sorted.begin(), sorted.end());
        std::cout << "        sort: ";
        for (int x : sorted) std::cout << x << " ";
        std::cout << "\n";

        auto partial = v;
        std::partial_sort(partial.begin(), partial.begin() + 3, partial.end());
        std::cout << "        partial_sort(前3小): ";
        for (int x : partial) std::cout << x << " ";
        std::cout << "（只保证前 3 个有序，比全排序快）\n";

        auto nth = v;
        std::nth_element(nth.begin(), nth.begin() + 3, nth.end());
        std::cout << "        nth_element(第4小) = " << nth[3] << "（O(n) 找第 k 小）\n";

        std::cout << "      数值算法:\n";
        std::cout << "        accumulate(求和) = "
                  << std::accumulate(v.begin(), v.end(), 0) << "\n";
        std::cout << "        accumulate(求积) = "
                  << std::accumulate(v.begin(), v.end(), 1, std::multiplies<int>{}) << "\n";
        std::cout << "        inner_product = "
                  << std::inner_product(v.begin(), v.end(), v.begin(), 0) << "\n";

        std::cout << "      二分查找（要求已排序）:\n";
        std::cout << "        binary_search(8) = "
                  << std::binary_search(sorted.begin(), sorted.end(), 8) << "\n";
        auto lb = std::lower_bound(sorted.begin(), sorted.end(), 5);
        std::cout << "        lower_bound(5) 位置 = "
                  << std::distance(sorted.begin(), lb) << "\n";
    }
}

// ---------- 3. erase-remove 惯用法 ----------
namespace erase_remove {
    void demo() {
        std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};

        // remove_if 不真正删除，只是把要保留的移到前面
        auto new_end = std::remove_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
        std::cout << "      remove_if 之后（size 没变）: ";
        for (int x : v) std::cout << x << " ";
        std::cout << " size=" << v.size() << "\n";
        std::cout << "      新的逻辑末尾在位置 " << std::distance(v.begin(), new_end) << "\n";

        v.erase(new_end, v.end());          // 这一步才真正删除
        std::cout << "      erase 之后: ";
        for (int x : v) std::cout << x << " ";
        std::cout << " size=" << v.size() << "\n";

        std::cout << "      为什么要两步：算法只有迭代器，无法改变容器大小\n";
        std::cout << "      （C++20 的 std::erase_if 一步完成）\n";

        // unique 也是同样的模式
        std::vector<int> dup{1, 1, 2, 2, 2, 3, 3};
        dup.erase(std::unique(dup.begin(), dup.end()), dup.end());
        std::cout << "      unique + erase 去重: ";
        for (int x : dup) std::cout << x << " ";
        std::cout << "\n";
        std::cout << "      注意 unique 只去除【相邻】重复，所以通常要先 sort\n";
    }
}

// ---------- 4. 迭代器适配器 ----------
namespace iterator_adapters {
    void demo() {
        std::vector<int> src{1, 2, 3};

        // back_insert_iterator：把算法的输出变成 push_back
        std::vector<int> dst;
        std::copy(src.begin(), src.end(), std::back_inserter(dst));
        std::cout << "      back_inserter: dst size=" << dst.size() << "\n";

        // 不用 back_inserter 就要先 resize
        std::vector<int> dst2(src.size());
        std::copy(src.begin(), src.end(), dst2.begin());
        std::cout << "      直接用 begin() 要求目标已有足够空间\n";

        // reverse_iterator
        std::cout << "      反向遍历: ";
        for (auto it = src.rbegin(); it != src.rend(); ++it) std::cout << *it << " ";
        std::cout << "\n";

        // ostream_iterator：直接输出到流
        std::cout << "      ostream_iterator: ";
        std::copy(src.begin(), src.end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << "\n";

        // inserter：插入关联容器
        std::set<int> s;
        std::copy(src.begin(), src.end(), std::inserter(s, s.begin()));
        std::cout << "      inserter 插入 set: size=" << s.size() << "\n";
    }
}

// ---------- 5. 并行算法 [C++17] ----------
namespace parallel_algorithms {
    void demo() {
        constexpr std::size_t kN = 10000000;
        std::vector<double> data(kN);

        std::mt19937 gen(42);
        std::uniform_real_distribution<double> dist(0.0, 1000.0);
        for (auto& x : data) x = dist(gen);

        auto bench = [](const char* name, auto&& fn) {
            auto t1 = std::chrono::steady_clock::now();
            auto result = fn();
            auto t2 = std::chrono::steady_clock::now();
            std::cout << "        " << name << ": "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
                      << " ms (结果 " << result << ")\n";
        };

        std::cout << "      " << kN << " 个 double 的 transform + 求和:\n";

        auto d1 = data;
        bench("串行 seq      ", [&d1] {
            std::transform(std::execution::seq, d1.begin(), d1.end(), d1.begin(),
                           [](double x) { return std::sqrt(x) * 2.0; });
            return std::accumulate(d1.begin(), d1.end(), 0.0);
        });

        auto d2 = data;
        bench("并行 par      ", [&d2] {
            std::transform(std::execution::par, d2.begin(), d2.end(), d2.begin(),
                           [](double x) { return std::sqrt(x) * 2.0; });
            return std::accumulate(d2.begin(), d2.end(), 0.0);
        });

        // 排序的并行效果更明显
        auto s1 = data, s2 = data;
        auto t1 = std::chrono::steady_clock::now();
        std::sort(std::execution::seq, s1.begin(), s1.end());
        auto t2 = std::chrono::steady_clock::now();
        std::sort(std::execution::par, s2.begin(), s2.end());
        auto t3 = std::chrono::steady_clock::now();

        auto ms = [](auto a, auto b) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
        };
        std::cout << "      " << kN << " 个 double 排序:\n";
        std::cout << "        串行: " << ms(t1, t2) << " ms\n";
        std::cout << "        并行: " << ms(t2, t3) << " ms\n";
        std::cout << "        结果一致: " << (s1 == s2) << "\n";

        // 对照组：手工用线程做并行归并，证明机器确实能并行加速
        auto s3 = data;
        auto t4 = std::chrono::steady_clock::now();
        {
            const std::size_t mid = s3.size() / 2;
            std::thread left([&] { std::sort(s3.begin(), s3.begin() + mid); });
            std::sort(s3.begin() + mid, s3.end());
            left.join();
            std::inplace_merge(s3.begin(), s3.begin() + mid, s3.end());
        }
        auto t5 = std::chrono::steady_clock::now();
        std::cout << "        手工 2 线程归并: " << ms(t4, t5)
                  << " ms（对照组，证明机器能并行加速）\n";
        std::cout << "        结果与串行一致: " << (s1 == s3) << "\n";

        std::cout << "      四种执行策略:\n";
        std::cout << "        seq        —— 串行，和不带策略一样\n";
        std::cout << "        par        —— 多线程并行\n";
        std::cout << "        par_unseq  —— 多线程 + 向量化\n";
        std::cout << "        unseq      —— 仅向量化 [C++20]\n";
        std::cout << "\n";
        std::cout << "      ⚠ 重要：上面 par 和 seq 的耗时【几乎相同】，因为本机没有装 TBB。\n";
        std::cout << "        libstdc++ 的并行算法依赖 Intel TBB，没有它时会【静默退化为串行】——\n";
        std::cout << "        不报错、不警告，代码照样编译运行，只是完全没有并行。\n";
        std::cout << "        要真正并行需要: apt install libtbb-dev && g++ ... -ltbb\n";
        std::cout << "        （libstdc++ 特有；MSVC 和 libc++ 的情况不同）\n";
    }
}

// ---------- 6. C++17 新增的小工具 ----------
namespace cpp17_utils {
    void demo() {
        std::cout << "      std::clamp(15, 1, 10) = " << std::clamp(15, 1, 10) << "\n";
        std::cout << "      std::clamp(-5, 1, 10) = " << std::clamp(-5, 1, 10) << "\n";
        std::cout << "      std::gcd(12, 18) = " << std::gcd(12, 18) << "\n";
        std::cout << "      std::lcm(4, 6) = " << std::lcm(4, 6) << "\n";

        // reduce：accumulate 的并行版本（不保证求和顺序）
        std::vector<int> v{1, 2, 3, 4, 5};
        std::cout << "      std::reduce = " << std::reduce(v.begin(), v.end()) << "\n";
        std::cout << "      std::transform_reduce = "
                  << std::transform_reduce(v.begin(), v.end(), 0, std::plus<>{},
                                           [](int x) { return x * x; })
                  << "（平方和）\n";
        std::cout << "      注意: reduce 不保证结合顺序，对浮点数结果可能与 accumulate 不同\n";

        std::cout << "      std::sample（随机抽样）: ";
        std::vector<int> pool(20);
        std::iota(pool.begin(), pool.end(), 1);
        std::vector<int> picked;
        std::sample(pool.begin(), pool.end(), std::back_inserter(picked), 5, std::mt19937{42});
        for (int x : picked) std::cout << x << " ";
        std::cout << "\n";
    }
}

int main() {
    std::cout << "--- 1. 迭代器分类 ---\n";         iterator_categories::demo();
    std::cout << "--- 2. 常用算法 ---\n";           common_algorithms::demo();
    std::cout << "--- 3. erase-remove ---\n";       erase_remove::demo();
    std::cout << "--- 4. 迭代器适配器 ---\n";       iterator_adapters::demo();
    std::cout << "--- 5. 并行算法 [C++17] ---\n";   parallel_algorithms::demo();
    std::cout << "--- 6. C++17 小工具 ---\n";       cpp17_utils::demo();
}
