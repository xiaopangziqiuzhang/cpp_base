// std=c++17
// 对应章节: blog/part7_stdlib/07-02_containers.md
// 主题: 容器全景、array/forward_list/unordered_*、emplace、迭代器失效

#include <algorithm>
#include <array>
#include <chrono>
#include <deque>
#include <forward_list>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// ---------- 1. std::array [C++11]：替代内置数组 ----------
namespace array_demo {
    void by_value(std::array<int, 3> a) { a[0] = 999; }     // 真的按值传递
    void c_array_param(int a[3]) { a[0] = 999; }             // 其实退化成了指针！

    void demo() {
        std::array<int, 5> arr{1, 2, 3, 4, 5};
        int c_arr[5]{1, 2, 3, 4, 5};

        std::cout << "      std::array 知道自己的大小: arr.size() = " << arr.size() << "\n";
        std::cout << "      内置数组作参数会退化成指针，丢失大小信息\n";

        std::array<int, 3> a{1, 2, 3};
        by_value(a);
        std::cout << "      array 按值传递是真拷贝: a[0] = " << a[0] << "（没被改）\n";

        int ca[3]{1, 2, 3};
        c_array_param(ca);
        std::cout << "      内置数组\"按值\"其实是传指针: ca[0] = " << ca[0]
                  << "（被改了！）\n";

        std::cout << "      array 支持迭代器、at()、front()/back()、比较运算符:\n";
        std::cout << "        at(10) 会抛异常，支持范围 for: ";
        for (int x : arr) std::cout << x << " ";
        std::cout << "\n";
        std::array<int, 3> b{1, 2, 3};
        std::cout << "        a == b: " << (a == b) << "（内置数组不能直接比较）\n";
        std::cout << "      sizeof(array<int,5>) = " << sizeof(arr)
                  << " == sizeof(int[5]) = " << sizeof(c_arr) << "（零开销）\n";
    }
}

// ---------- 2. forward_list [C++11]：单向链表 ----------
namespace forward_list_demo {
    void demo() {
        std::forward_list<int> fl{1, 2, 3};
        std::list<int> dl{1, 2, 3};

        fl.push_front(0);
        std::cout << "      forward_list: ";
        for (int x : fl) std::cout << x << " ";
        std::cout << "\n";

        std::cout << "      特点: 只能单向遍历、没有 size()、没有 push_back\n";
        std::cout << "      价值: 每个节点只有一个指针，比 list 省内存\n";
        std::cout << "      插入要用 insert_after（因为无法回头找前驱）\n";
        auto it = fl.begin();
        fl.insert_after(it, 99);
        std::cout << "      insert_after 之后: ";
        for (int x : fl) std::cout << x << " ";
        std::cout << "\n";
    }
}

// ---------- 3. unordered_* [C++11]：哈希容器 ----------
namespace unordered_demo {
    void demo() {
        std::map<std::string, int> ordered{{"banana", 2}, {"apple", 1}, {"cherry", 3}};
        std::unordered_map<std::string, int> hashed{{"banana", 2}, {"apple", 1}, {"cherry", 3}};

        std::cout << "      map（红黑树，有序）:      ";
        for (const auto& [k, v] : ordered) std::cout << k << "=" << v << " ";
        std::cout << "\n";

        std::cout << "      unordered_map（哈希，无序）: ";
        for (const auto& [k, v] : hashed) std::cout << k << "=" << v << " ";
        std::cout << "\n";

        std::cout << "      复杂度: map 是 O(log n)，unordered_map 平均 O(1) 最坏 O(n)\n";
        std::cout << "      哈希表内部信息: bucket_count=" << hashed.bucket_count()
                  << " load_factor=" << hashed.load_factor()
                  << " max_load_factor=" << hashed.max_load_factor() << "\n";

        // 性能对比
        constexpr int kN = 200000;
        std::map<int, int> m;
        std::unordered_map<int, int> um;
        um.reserve(kN);

        auto t1 = std::chrono::steady_clock::now();
        for (int i = 0; i < kN; ++i) m[i] = i;
        auto t2 = std::chrono::steady_clock::now();
        for (int i = 0; i < kN; ++i) um[i] = i;
        auto t3 = std::chrono::steady_clock::now();

        long long sum1 = 0, sum2 = 0;
        auto t4 = std::chrono::steady_clock::now();
        for (int i = 0; i < kN; ++i) sum1 += m[i];
        auto t5 = std::chrono::steady_clock::now();
        for (int i = 0; i < kN; ++i) sum2 += um[i];
        auto t6 = std::chrono::steady_clock::now();

        auto us = [](auto a, auto b) {
            return std::chrono::duration_cast<std::chrono::microseconds>(b - a).count();
        };
        std::cout << "      " << kN << " 次插入: map " << us(t1, t2)
                  << " us, unordered_map " << us(t2, t3) << " us\n";
        std::cout << "      " << kN << " 次查找: map " << us(t4, t5)
                  << " us, unordered_map " << us(t5, t6) << " us\n";
        (void)sum1; (void)sum2;
    }
}

// ---------- 4. emplace vs insert/push ----------
namespace emplace_demo {
    struct Tracer {
        std::string name;
        explicit Tracer(std::string n) : name(std::move(n)) {
            std::cout << "          构造 " << name << "\n";
        }
        Tracer(const Tracer& rhs) : name(rhs.name) {
            std::cout << "          拷贝 " << name << "\n";
        }
        Tracer(Tracer&& rhs) noexcept : name(std::move(rhs.name)) {
            std::cout << "          移动\n";
        }
    };

    void demo() {
        std::vector<Tracer> v;
        v.reserve(4);

        std::cout << "        push_back(Tracer(\"a\")):\n";
        v.push_back(Tracer("a"));              // 构造临时对象 + 移动

        std::cout << "        emplace_back(\"b\"):\n";
        v.emplace_back("b");                    // 直接构造，无临时对象

        std::cout << "      emplace 系列把参数完美转发给元素的构造函数（见 02-04）\n";

        // map 的特殊情况
        std::map<int, std::string> m;
        m.emplace(1, "one");
        m.try_emplace(2, "two");               // [C++17] 键已存在时不构造值
        m.insert_or_assign(1, "ONE");          // [C++17] 存在就赋值，不存在就插入

        std::cout << "      [C++17] try_emplace: 键已存在时【不构造】值参数\n";
        std::cout << "      [C++17] insert_or_assign: 明确表达\"覆盖\"语义\n";
        for (const auto& [k, val] : m) std::cout << "        " << k << " => " << val << "\n";
    }
}

// ---------- 5. 迭代器失效规则 ----------
namespace invalidation {
    void demo() {
        std::cout << "      vector:\n";
        std::vector<int> v{1, 2, 3};
        std::cout << "        capacity=" << v.capacity() << "\n";
        auto* old_data = v.data();
        v.push_back(4);
        v.push_back(5);
        std::cout << "        push_back 后 capacity=" << v.capacity()
                  << " 数据地址" << (v.data() == old_data ? "未变" : "已变（全部迭代器失效）")
                  << "\n";
        std::cout << "        规则: 扩容 -> 全部失效；erase -> 该位置及之后失效\n";

        std::cout << "      deque:\n";
        std::cout << "        规则: 任何插入都可能使迭代器失效，但【引用】在两端插入时保持有效\n";

        std::cout << "      list / forward_list:\n";
        std::list<int> l{1, 2, 3};
        auto it = std::next(l.begin());
        l.push_front(0);
        l.push_back(4);
        std::cout << "        插入后原迭代器仍指向 " << *it << "（不失效）\n";
        std::cout << "        规则: 插入不失效；erase 只让被删元素的迭代器失效\n";

        std::cout << "      map / set:\n";
        std::map<int, int> m{{1, 1}, {2, 2}};
        auto mit = m.find(1);
        m[3] = 3;
        std::cout << "        插入后原迭代器仍指向 key=" << mit->first << "（不失效）\n";

        std::cout << "      unordered_*:\n";
        std::cout << "        规则: rehash 时迭代器全部失效，但【引用和指针】保持有效\n";
    }
}

// ---------- 6. 边遍历边删除的正确写法 ----------
namespace erase_while_iterating {
    void demo() {
        // 错误：erase 之后 it 失效
        // for (auto it = v.begin(); it != v.end(); ++it)
        //     if (*it % 2 == 0) v.erase(it);        // UB

        // 正确写法一：用 erase 的返回值
        std::vector<int> v{1, 2, 3, 4, 5, 6};
        for (auto it = v.begin(); it != v.end(); ) {
            if (*it % 2 == 0) it = v.erase(it);      // erase 返回下一个有效迭代器
            else ++it;
        }
        std::cout << "      手工循环 + erase 返回值: ";
        for (int x : v) std::cout << x << " ";
        std::cout << "\n";

        // 正确写法二：erase-remove 惯用法（vector 推荐）
        std::vector<int> v2{1, 2, 3, 4, 5, 6};
        v2.erase(std::remove_if(v2.begin(), v2.end(),
                                [](int x) { return x % 2 == 0; }),
                 v2.end());
        std::cout << "      erase-remove 惯用法: ";
        for (int x : v2) std::cout << x << " ";
        std::cout << "\n";

        // 关联容器：erase 返回下一个迭代器（C++11 起）
        std::map<int, int> m{{1, 1}, {2, 2}, {3, 3}, {4, 4}};
        for (auto it = m.begin(); it != m.end(); ) {
            if (it->first % 2 == 0) it = m.erase(it);
            else ++it;
        }
        std::cout << "      map 删除偶数键: ";
        for (const auto& [k, val] : m) std::cout << k << " ";
        std::cout << "\n";
        std::cout << "      （C++20 提供了 std::erase_if 统一这些写法）\n";
    }
}

// ---------- 7. vector 的 reserve 与内存增长 ----------
namespace vector_growth {
    void demo() {
        std::cout << "      观察 vector 的容量增长（libstdc++ 是 2 倍）:\n";
        std::vector<int> v;
        std::size_t last_cap = 0;
        for (int i = 0; i < 100; ++i) {
            v.push_back(i);
            if (v.capacity() != last_cap) {
                std::cout << "        size=" << v.size() << " capacity=" << v.capacity() << "\n";
                last_cap = v.capacity();
            }
        }

        constexpr int kN = 1000000;
        auto t1 = std::chrono::steady_clock::now();
        std::vector<int> a;
        for (int i = 0; i < kN; ++i) a.push_back(i);
        auto t2 = std::chrono::steady_clock::now();
        std::vector<int> b;
        b.reserve(kN);
        for (int i = 0; i < kN; ++i) b.push_back(i);
        auto t3 = std::chrono::steady_clock::now();

        auto us = [](auto x, auto y) {
            return std::chrono::duration_cast<std::chrono::microseconds>(y - x).count();
        };
        std::cout << "      " << kN << " 次 push_back:\n";
        std::cout << "        不 reserve: " << us(t1, t2) << " us\n";
        std::cout << "        先 reserve: " << us(t2, t3) << " us\n";

        // shrink_to_fit
        std::vector<int> big(1000000);
        std::cout << "      清空前 capacity=" << big.capacity() << "\n";
        big.clear();
        std::cout << "      clear() 后 size=" << big.size()
                  << " capacity=" << big.capacity() << "（内存没还）\n";
        big.shrink_to_fit();
        std::cout << "      shrink_to_fit() 后 capacity=" << big.capacity() << "\n";
    }
}

int main() {
    std::cout << "--- 1. std::array [C++11] ---\n";      array_demo::demo();
    std::cout << "--- 2. forward_list [C++11] ---\n";    forward_list_demo::demo();
    std::cout << "--- 3. unordered_* [C++11] ---\n";     unordered_demo::demo();
    std::cout << "--- 4. emplace ---\n";                 emplace_demo::demo();
    std::cout << "--- 5. 迭代器失效 ---\n";              invalidation::demo();
    std::cout << "--- 6. 边遍历边删除 ---\n";            erase_while_iterating::demo();
    std::cout << "--- 7. vector 的内存增长 ---\n";       vector_growth::demo();
}
