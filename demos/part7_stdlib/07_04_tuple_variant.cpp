// std=c++17
// 对应章节: blog/part7_stdlib/07-04_tuple_variant.md
// 主题: pair/tuple、optional/variant/any

#include <any>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

// ---------- 1. pair 与 tuple ----------
namespace tuple_basics {
    std::tuple<int, std::string, double> get_record() {
        return {1, "alice", 95.5};
    }

    void demo() {
        // pair
        std::pair<int, std::string> p{1, "one"};
        std::cout << "      pair: " << p.first << ", " << p.second << "\n";

        // tuple
        std::tuple<int, std::string, double> t{1, "alice", 95.5};
        std::cout << "      get<0>(t) = " << std::get<0>(t)
                  << ", get<1>(t) = " << std::get<1>(t) << "\n";
        std::cout << "      按类型取（要求类型唯一）: get<double>(t) = "
                  << std::get<double>(t) << "\n";
        std::cout << "      tuple_size = " << std::tuple_size_v<decltype(t)> << "\n";

        // C++17 CTAD：不用写类型
        std::tuple t2{1, 2.5, "three"};
        std::pair p2{1, 2.5};
        std::cout << "      [C++17] CTAD: std::tuple t2{1, 2.5, \"three\"} 不用写类型\n";

        // 三种接住返回值的方式
        std::cout << "      接住多返回值的三种写法:\n";
        auto r1 = get_record();
        std::cout << "        1. get<N>: " << std::get<1>(r1) << "\n";

        int id; std::string name; double score;
        std::tie(id, name, score) = get_record();        // C++11，变量要先声明
        std::cout << "        2. std::tie: " << name << "\n";

        auto [id2, name2, score2] = get_record();         // C++17 结构化绑定
        std::cout << "        3. 结构化绑定 [C++17]: " << name2 << "（最清楚）\n";

        // std::ignore 跳过不关心的部分
        std::string only_name;
        std::tie(std::ignore, only_name, std::ignore) = get_record();
        std::cout << "      std::ignore 跳过: " << only_name << "\n";

        (void)p2; (void)t2; (void)id; (void)score;
    }
}

// ---------- 2. std::apply [C++17] ----------
namespace apply_demo {
    int sum3(int a, int b, int c) { return a + b + c; }

    void demo() {
        auto args = std::make_tuple(1, 2, 3);
        std::cout << "      std::apply(sum3, tuple{1,2,3}) = "
                  << std::apply(sum3, args) << "\n";

        // 常用于遍历 tuple
        auto print_all = [](const auto&... args) {
            ((std::cout << args << " "), ...);     // 折叠表达式
        };
        std::cout << "      遍历 tuple: ";
        std::apply(print_all, std::make_tuple(1, "two", 3.0));
        std::cout << "\n";
        std::cout << "      apply 把 tuple 展开成函数实参（C++14 要手写 index_sequence）\n";
    }
}

// ---------- 3. std::optional [C++17] ----------
namespace optional_demo {
    // 旧做法的三种，各有缺陷
    bool find_old1(const std::vector<int>& v, int target, int& out) {   // 出参
        for (int x : v) if (x == target) { out = x; return true; }
        return false;
    }
    int find_old2(const std::vector<int>& v, int target) {               // 魔术值
        for (int x : v) if (x == target) return x;
        return -1;      // -1 表示"没找到"？如果 -1 是合法值怎么办
    }
    int* find_old3(std::vector<int>& v, int target) {                    // 指针
        for (int& x : v) if (x == target) return &x;
        return nullptr;  // 调用方可能忘记判空
    }

    // C++17：类型本身表达"可能没有值"
    std::optional<int> find_new(const std::vector<int>& v, int target) {
        for (int x : v) if (x == target) return x;
        return std::nullopt;
    }

    std::optional<int> parse_int(const std::string& s) {
        try { return std::stoi(s); }
        catch (...) { return std::nullopt; }
    }

    void demo() {
        std::vector<int> v{1, 2, 3};

        if (auto r = find_new(v, 2)) {
            std::cout << "      找到了: " << *r << "\n";
        }
        if (!find_new(v, 99)) {
            std::cout << "      没找到 99（optional 为空）\n";
        }

        std::cout << "      常用接口:\n";
        auto opt = find_new(v, 3);
        std::cout << "        has_value() = " << opt.has_value() << "\n";
        std::cout << "        *opt = " << *opt << ", opt.value() = " << opt.value() << "\n";
        std::cout << "        value_or(默认值): " << find_new(v, 99).value_or(-1) << "\n";

        // value() 在空时抛异常，* 是未定义行为
        std::optional<int> empty;
        try {
            empty.value();
        } catch (const std::bad_optional_access& e) {
            std::cout << "        空 optional 调 value() 抛: " << e.what() << "\n";
        }
        std::cout << "        （*empty 是未定义行为，不抛异常）\n";

        std::cout << "      解析字符串: parse_int(\"42\") = "
                  << parse_int("42").value_or(-1) << "\n";
        std::cout << "                  parse_int(\"abc\") = "
                  << parse_int("abc").value_or(-1) << "（失败返回默认值）\n";

        std::cout << "      sizeof(int) = " << sizeof(int)
                  << ", sizeof(optional<int>) = " << sizeof(std::optional<int>)
                  << "（多一个 bool + 对齐）\n";

        int out = 0;
        (void)find_old1(v, 2, out); (void)find_old2(v, 2); (void)find_old3(v, 2);
    }
}

// ---------- 4. std::variant [C++17] ----------
namespace variant_demo {
    // 旧做法：union 不安全，不知道当前存的是什么
    union UnsafeUnion {
        int i;
        double d;
        // std::string s;      // 不能放非平凡类型！
    };

    void demo() {
        std::variant<int, double, std::string> v = 42;

        std::cout << "      index() = " << v.index() << "（当前是第 0 个类型）\n";
        std::cout << "      get<int>(v) = " << std::get<int>(v) << "\n";

        v = 3.14;
        std::cout << "      赋值 double 后 index() = " << v.index() << "\n";

        v = std::string("hello");
        std::cout << "      赋值 string 后 index() = " << v.index()
                  << ", 值 = " << std::get<std::string>(v) << "\n";

        // 类型错误时抛异常
        try {
            std::get<int>(v);
        } catch (const std::bad_variant_access& e) {
            std::cout << "      取错类型抛: " << e.what() << "\n";
        }

        // get_if：不抛异常，失败返回 nullptr
        if (auto* p = std::get_if<std::string>(&v)) {
            std::cout << "      get_if<string> 成功: " << *p << "\n";
        }
        if (!std::get_if<int>(&v)) {
            std::cout << "      get_if<int> 返回 nullptr（安全）\n";
        }

        std::cout << "      holds_alternative<string> = "
                  << std::holds_alternative<std::string>(v) << "\n";

        // std::visit：处理所有可能的类型
        std::cout << "      std::visit 处理各种类型:\n";
        auto printer = [](const auto& value) {
            std::cout << "        收到: " << value << "\n";
        };
        std::variant<int, double, std::string> vals[] = {1, 2.5, std::string("three")};
        for (const auto& val : vals) std::visit(printer, val);

        // visit 配合 overloaded 惯用法做类型分发
        struct Visitor {
            void operator()(int x) const { std::cout << "        整型: " << x << "\n"; }
            void operator()(double x) const { std::cout << "        浮点: " << x << "\n"; }
            void operator()(const std::string& x) const {
                std::cout << "        字符串: " << x << "\n";
            }
        };
        std::cout << "      用访问者做类型分发:\n";
        for (const auto& val : vals) std::visit(Visitor{}, val);

        std::cout << "      sizeof(variant<int,double,string>) = "
                  << sizeof(std::variant<int, double, std::string>)
                  << "（最大成员 + 类型标记）\n";
    }
}

// ---------- 5. variant 作为虚函数的替代 ----------
namespace variant_vs_virtual {
    struct Circle { double r; double area() const { return 3.14159 * r * r; } };
    struct Square { double s; double area() const { return s * s; } };
    struct Triangle { double b, h; double area() const { return b * h / 2; } };

    using Shape = std::variant<Circle, Square, Triangle>;

    void demo() {
        std::vector<Shape> shapes{Circle{1.0}, Square{2.0}, Triangle{3.0, 4.0}};

        double total = 0;
        for (const auto& s : shapes) {
            total += std::visit([](const auto& shape) { return shape.area(); }, s);
        }
        std::cout << "      总面积 = " << total << "\n";

        std::cout << "      相比虚函数的优点:\n";
        std::cout << "        1. 值语义 —— 可以直接放 vector，不需要 unique_ptr\n";
        std::cout << "        2. 无虚表指针、无间接调用，可以内联\n";
        std::cout << "        3. visit 要求处理【所有】类型，漏了就编译错误\n";
        std::cout << "      缺点:\n";
        std::cout << "        1. 类型集合必须【封闭】（编译期已知全部类型）\n";
        std::cout << "        2. 大小是最大成员的大小（类型差异大时浪费内存）\n";
        std::cout << "        3. 不能跨 ABI 边界扩展\n";
        std::cout << "      （对比见 04-07）\n";
    }
}

// ---------- 6. std::any [C++17] ----------
namespace any_demo {
    void demo() {
        std::any a = 42;
        std::cout << "      any 存 int: " << std::any_cast<int>(a) << "\n";

        a = std::string("hello");
        std::cout << "      any 存 string: " << std::any_cast<std::string>(a) << "\n";

        a = 3.14;
        std::cout << "      any 存 double: " << std::any_cast<double>(a) << "\n";

        std::cout << "      has_value() = " << a.has_value()
                  << ", type().name() = " << a.type().name() << "\n";

        try {
            std::any_cast<int>(a);
        } catch (const std::bad_any_cast& e) {
            std::cout << "      取错类型抛: " << e.what() << "\n";
        }

        // 指针版本不抛异常
        if (auto* p = std::any_cast<double>(&a)) {
            std::cout << "      any_cast<double>(&a) 成功: " << *p << "\n";
        }

        a.reset();
        std::cout << "      reset() 后 has_value() = " << a.has_value() << "\n";

        std::cout << "      sizeof(any) = " << sizeof(std::any) << "\n";
        std::cout << "      与 variant 的区别:\n";
        std::cout << "        variant: 类型集合【封闭】，编译期检查，无堆分配（小对象）\n";
        std::cout << "        any:     可存【任意】类型，运行期检查，大对象要堆分配\n";
        std::cout << "      实践：优先 variant；any 只在真的需要\"任意类型\"时用\n";
    }
}

// ---------- 7. 三者的选择 ----------
namespace selection {
    void demo() {
        std::cout << "      optional<T>  —— \"可能有一个 T\"（0 或 1 个值）\n";
        std::cout << "        用途: 可能失败的查找/解析、可选的配置项、延迟初始化\n";
        std::cout << "      variant<A,B,C> —— \"是 A 或 B 或 C 之一\"（和类型）\n";
        std::cout << "        用途: 状态机、表达式树、解析结果、替代虚函数\n";
        std::cout << "      any —— \"是任意类型\"\n";
        std::cout << "        用途: 类型擦除的容器、插件系统的参数传递\n";
        std::cout << "      tuple<A,B,C> —— \"同时有 A 和 B 和 C\"（积类型）\n";
        std::cout << "        用途: 多返回值、泛型代码里的参数包存储\n";
    }
}

int main() {
    std::cout << "--- 1. pair 与 tuple ---\n";        tuple_basics::demo();
    std::cout << "--- 2. std::apply [C++17] ---\n";   apply_demo::demo();
    std::cout << "--- 3. optional [C++17] ---\n";     optional_demo::demo();
    std::cout << "--- 4. variant [C++17] ---\n";      variant_demo::demo();
    std::cout << "--- 5. variant 替代虚函数 ---\n";   variant_vs_virtual::demo();
    std::cout << "--- 6. any [C++17] ---\n";          any_demo::demo();
    std::cout << "--- 7. 怎么选 ---\n";               selection::demo();
}
