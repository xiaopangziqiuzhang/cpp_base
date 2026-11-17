// std=c++17
// 对应章节: blog/part7_stdlib/07-01_string.md
// 主题: C 风格字符串的问题、std::string 接口、SSO 实测、string_view

#include <charconv>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// ---------- 1. C 风格字符串的问题 ----------
namespace c_string_problems {
    void demo() {
        char str[] = "hello";
        const char* pstr = "world";

        std::cout << "      求长度: strlen(str) = " << std::strlen(str)
                  << "（O(n) 遍历，每次都重新算）\n";

        // 拼接：必须手工算大小、手工管理内存
        char* ptmp = static_cast<char*>(
            std::malloc(std::strlen(str) + std::strlen(pstr) + 1));
        std::strcpy(ptmp, str);
        std::strcat(ptmp, pstr);
        std::cout << "      拼接: " << ptmp << "（要手工 malloc + 算大小 + free）\n";
        std::free(ptmp);

        std::cout << "      问题清单:\n";
        std::cout << "        1. 长度是 O(n)（要扫描到 '\\0'）\n";
        std::cout << "        2. 内存要手工管理，容易泄漏\n";
        std::cout << "        3. 缓冲区大小要手工算，strcpy/strcat 是缓冲区溢出的头号来源\n";
        std::cout << "        4. 不能用 == 比较（比的是指针）\n";
        std::cout << "        5. 结尾必须有 '\\0'，忘了就越界\n";
        std::cout << "        6. 不能直接放进容器、不能安全返回\n";
    }
}

// ---------- 2. std::string 的常用接口 ----------
namespace string_api {
    void demo() {
        std::string s1 = "hello";
        std::string s2("world");
        std::string s3(5, 'x');                      // 5 个 'x'
        std::string s4(s1.begin(), s1.begin() + 3);  // 迭代器区间

        std::cout << "      构造: s3=" << s3 << " s4=" << s4 << "\n";
        std::cout << "      长度: size()=" << s1.size()
                  << " length()=" << s1.length() << "（两者等价，O(1)）\n";
        std::cout << "      容量: capacity()=" << s1.capacity() << "\n";
        std::cout << "      拼接: s1 + \",\" + s2 = " << (s1 + "," + s2) << "\n";

        std::string s = s1;
        s += ", ";
        s.append(s2);
        std::cout << "      += 和 append: " << s << "\n";

        std::cout << "      查找: s.find(\"world\") = " << s.find("world") << "\n";
        std::cout << "      未找到返回 npos: " << (s.find("xyz") == std::string::npos) << "\n";
        std::cout << "      子串: s.substr(7) = " << s.substr(7) << "\n";
        std::cout << "      比较: (s1 == \"hello\") = " << (s1 == "hello")
                  << "（可以直接用 ==）\n";

        // [] 不检查越界，at() 检查
        std::cout << "      s1[0] = " << s1[0] << ", s1.at(0) = " << s1.at(0) << "\n";
        try {
            s1.at(100);
        } catch (const std::out_of_range& e) {
            std::cout << "      at(100) 抛出 out_of_range\n";
        }
        std::cout << "      （operator[] 越界是未定义行为，不会抛异常）\n";

        // C++11 起 c_str() 和 data() 都保证以 '\0' 结尾
        std::cout << "      转 C 风格: c_str() = " << s1.c_str() << "\n";
    }
}

// ---------- 3. SSO 实测 ----------
namespace sso_demo {
    void demo() {
        std::cout << "      sizeof(std::string) = " << sizeof(std::string)
                  << "（libstdc++ 的 SSO 实现）\n";

        // 通过比较字符串数据的地址和对象自身的地址，判断是否用了 SSO
        auto uses_sso = [](const std::string& s) {
            auto obj_addr = reinterpret_cast<const char*>(&s);
            auto data_addr = s.data();
            // 数据在对象内部 => SSO
            return data_addr >= obj_addr && data_addr < obj_addr + sizeof(std::string);
        };

        std::cout << "      逐步加长字符串，观察何时从栈内缓冲切到堆:\n";
        for (std::size_t n : {0u, 5u, 15u, 16u, 20u, 100u}) {
            std::string s(n, 'x');
            std::cout << "        长度 " << n << ": "
                      << (uses_sso(s) ? "SSO（数据在对象内）" : "堆分配")
                      << ", capacity=" << s.capacity() << "\n";
        }
        std::cout << "      libstdc++ 的阈值是 15 个字符（16 字节含 '\\0'）\n";
    }
}

// ---------- 4. SSO 的性能意义 ----------
namespace sso_perf {
    void demo() {
        constexpr int kTimes = 2000000;

        auto bench = [](const char* name, std::size_t len, int times) {
            std::string src(len, 'x');
            auto t1 = std::chrono::steady_clock::now();
            std::size_t total = 0;
            for (int i = 0; i < times; ++i) {
                std::string copy = src;         // 拷贝
                total += copy.size();
            }
            auto t2 = std::chrono::steady_clock::now();
            std::cout << "        " << name << ": "
                      << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()
                      << " us\n";
            return total;
        };

        std::cout << "      " << kTimes << " 次字符串拷贝:\n";
        bench("短字符串(10 字符，走 SSO) ", 10, kTimes);
        bench("长字符串(100 字符，走堆)  ", 100, kTimes);
        std::cout << "      短字符串不需要堆分配，所以快得多（见 02-01 的堆/栈实测）\n";
    }
}

// ---------- 5. 常见性能陷阱 ----------
namespace perf_traps {
    void demo() {
        constexpr int kN = 100000;

        // 陷阱一：循环中反复 += 不预留容量
        auto t1 = std::chrono::steady_clock::now();
        std::string s1;
        for (int i = 0; i < kN; ++i) s1 += 'x';
        auto t2 = std::chrono::steady_clock::now();

        std::string s2;
        s2.reserve(kN);                      // 预留
        for (int i = 0; i < kN; ++i) s2 += 'x';
        auto t3 = std::chrono::steady_clock::now();

        auto us = [](auto a, auto b) {
            return std::chrono::duration_cast<std::chrono::microseconds>(b - a).count();
        };
        std::cout << "      " << kN << " 次 += 字符:\n";
        std::cout << "        不 reserve: " << us(t1, t2) << " us\n";
        std::cout << "        先 reserve: " << us(t2, t3) << " us\n";

        // 陷阱二：substr 会拷贝
        std::string long_str(1000, 'a');
        std::cout << "      substr 会【拷贝】子串（C++17 可用 string_view 避免）\n";

        // 陷阱三：按值传参导致不必要的拷贝
        std::cout << "      传参: 只读用 const string&（或 string_view），要存就按值传 + move\n";
    }
}

// ---------- 6. string_view [C++17] ----------
namespace string_view_demo {
    // 旧写法：三个重载，或者强制调用方构造 string
    void process_old(const std::string& s) { (void)s; }

    // 新写法：一个函数接受所有字符串形式，且零拷贝
    std::size_t count_chars(std::string_view sv, char c) {
        std::size_t n = 0;
        for (char ch : sv) if (ch == c) ++n;
        return n;
    }

    void demo() {
        std::string str = "hello world";
        const char* cstr = "hello world";

        std::cout << "      一个 string_view 参数接受所有形式:\n";
        std::cout << "        std::string:  " << count_chars(str, 'l') << "\n";
        std::cout << "        const char*:  " << count_chars(cstr, 'l') << "\n";
        std::cout << "        字面量:       " << count_chars("hello world", 'l') << "\n";
        std::cout << "        子串（零拷贝）: "
                  << count_chars(std::string_view(str).substr(0, 5), 'l') << "\n";

        std::cout << "      sizeof(string_view) = " << sizeof(std::string_view)
                  << "（一个指针 + 一个长度）\n";

        // 零拷贝的子串操作
        std::string_view sv = str;
        std::cout << "      sv.substr(6) = " << sv.substr(6) << "（不分配内存）\n";
        sv.remove_prefix(6);
        std::cout << "      remove_prefix(6) 后: " << sv << "\n";

        // 性能对比
        constexpr int kTimes = 1000000;
        std::string source(200, 'x');
        auto t1 = std::chrono::steady_clock::now();
        std::size_t n1 = 0;
        for (int i = 0; i < kTimes; ++i) n1 += source.substr(10, 50).size();   // 拷贝
        auto t2 = std::chrono::steady_clock::now();
        std::size_t n2 = 0;
        for (int i = 0; i < kTimes; ++i)
            n2 += std::string_view(source).substr(10, 50).size();               // 零拷贝
        auto t3 = std::chrono::steady_clock::now();

        auto us = [](auto a, auto b) {
            return std::chrono::duration_cast<std::chrono::microseconds>(b - a).count();
        };
        std::cout << "      " << kTimes << " 次取子串:\n";
        std::cout << "        string::substr:      " << us(t1, t2) << " us\n";
        std::cout << "        string_view::substr: " << us(t2, t3) << " us\n";
        (void)n1; (void)n2;
    }
}

// ---------- 7. string_view 的悬垂陷阱 ----------
namespace sv_dangling {
    // 这个函数【故意】是错的，只作为反例展示，不被调用。
    // 调用它并使用返回值就是未定义行为。
    [[maybe_unused]] std::string_view bad() {
        std::string local = "temporary";
        return local;            // 危险！返回后 local 销毁，view 悬垂
    }

    void demo() {
        std::cout << "      陷阱一：指向临时对象\n";
        std::cout << "        std::string_view sv = get_string();  // 临时 string 立刻销毁\n";
        std::cout << "        std::string_view sv = s1 + s2;        // 同样危险\n";

        std::cout << "      陷阱二：返回局部 string 的 view（上面的 bad()）\n";

        std::cout << "      陷阱三：不保证以 '\\0' 结尾\n";
        std::string s = "hello world";
        std::string_view sv(s.data(), 5);
        std::cout << "        sv = " << sv << "（打印正常，因为知道长度）\n";
        std::cout << "        但 sv.data() 不能直接传给需要 C 字符串的函数！\n";
        std::cout << "        （没有 c_str()，这是故意的设计）\n";

        std::cout << "      规则:\n";
        std::cout << "        1. string_view 只做【函数参数】，不做成员、不做返回值\n";
        std::cout << "        2. 被指向的字符串必须比 view 活得久\n";
        std::cout << "        3. 需要 C 字符串时，先构造 std::string\n";
    }
}

// ---------- 8. C++17 的字符串/数值转换 ----------
namespace conversions {
    void demo() {
        // C++11 的 to_string / stoi
        std::string s = std::to_string(42);
        int n = std::stoi("123");
        double d = std::stod("3.14");
        std::cout << "      to_string(42) = " << s << ", stoi(\"123\") = " << n
                  << ", stod(\"3.14\") = " << d << "\n";

        try {
            std::stoi("not a number");
        } catch (const std::invalid_argument&) {
            std::cout << "      stoi 失败抛 invalid_argument（有异常开销）\n";
        }

        // C++17 的 from_chars：无异常、无分配、无 locale，最快
        const char* text = "12345abc";
        int value = 0;
        auto [ptr, ec] = std::from_chars(text, text + std::strlen(text), value);
        std::cout << "      from_chars: value=" << value
                  << " 剩余=\"" << ptr << "\""
                  << " 成功=" << (ec == std::errc{}) << "\n";

        char buf[32];
        auto [out_ptr, out_ec] = std::to_chars(buf, buf + sizeof(buf), 54321);
        *out_ptr = '\0';
        std::cout << "      to_chars: \"" << buf << "\" 成功="
                  << (out_ec == std::errc{}) << "\n";
        std::cout << "      from_chars/to_chars 是性能最好的转换方式（无异常、无分配）\n";
    }
}

int main() {
    std::cout << "--- 1. C 风格字符串的问题 ---\n";  c_string_problems::demo();
    std::cout << "--- 2. std::string 接口 ---\n";     string_api::demo();
    std::cout << "--- 3. SSO 实测 ---\n";             sso_demo::demo();
    std::cout << "--- 4. SSO 的性能意义 ---\n";       sso_perf::demo();
    std::cout << "--- 5. 性能陷阱 ---\n";             perf_traps::demo();
    std::cout << "--- 6. string_view [C++17] ---\n";  string_view_demo::demo();
    std::cout << "--- 7. string_view 的陷阱 ---\n";   sv_dangling::demo();
    std::cout << "--- 8. 字符串/数值转换 ---\n";      conversions::demo();
}
