// std=c++17
// 对应章节: blog/part4_oop/04-07_crtp.md
// 主题: 虚函数的运行期开销实测、CRTP 静态多态

#include <chrono>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

// ---------- 1. 动态多态：虚函数 ----------
namespace dynamic_poly {
    class Shape {
    public:
        virtual ~Shape() = default;
        virtual double area() const = 0;
    };
    class Circle : public Shape {
    public:
        explicit Circle(double r) : _r(r) {}
        double area() const override { return 3.14159 * _r * _r; }
    private:
        double _r;
    };
    class Square : public Shape {
    public:
        explicit Square(double s) : _s(s) {}
        double area() const override { return _s * _s; }
    private:
        double _s;
    };
}

// ---------- 2. 静态多态：CRTP ----------
namespace static_poly {
    // 基类模板以派生类作为模板参数（Curiously Recurring Template Pattern）
    template <class Derived>
    class Shape {
    public:
        // 非虚函数，但能调用派生类的实现
        double area() const {
            return static_cast<const Derived*>(this)->area_impl();
        }

        // 也可以提供默认实现，派生类可选择覆盖
        std::string describe() const {
            return "面积 = " + std::to_string(area());
        }
    };

    class Circle : public Shape<Circle> {
    public:
        explicit Circle(double r) : _r(r) {}
        double area_impl() const { return 3.14159 * _r * _r; }
    private:
        double _r;
    };

    class Square : public Shape<Square> {
    public:
        explicit Square(double s) : _s(s) {}
        double area_impl() const { return _s * _s; }
    private:
        double _s;
    };

    // 泛型函数：在编译期确定具体类型，调用可以内联
    template <class T>
    double total_area(const std::vector<T>& shapes) {
        double sum = 0;
        for (const auto& s : shapes) sum += s.area();     // 非虚调用，可内联
        return sum;
    }
}

// ---------- 3. 性能对比 ----------
void performance() {
    constexpr int kCount = 2000000;

    // 动态多态：基类指针的容器
    std::vector<std::unique_ptr<dynamic_poly::Shape>> dyn;
    dyn.reserve(kCount);
    for (int i = 0; i < kCount; ++i) {
        if (i % 2 == 0) dyn.push_back(std::make_unique<dynamic_poly::Circle>(1.0));
        else            dyn.push_back(std::make_unique<dynamic_poly::Square>(2.0));
    }

    // 静态多态：同类型元素的容器
    std::vector<static_poly::Circle> circles(kCount / 2, static_poly::Circle(1.0));
    std::vector<static_poly::Square> squares(kCount / 2, static_poly::Square(2.0));

    auto t1 = std::chrono::steady_clock::now();
    double dyn_sum = 0;
    for (const auto& s : dyn) dyn_sum += s->area();       // 每次都查虚表
    auto t2 = std::chrono::steady_clock::now();

    double static_sum = static_poly::total_area(circles) + static_poly::total_area(squares);
    auto t3 = std::chrono::steady_clock::now();

    auto us = [](auto a, auto b) {
        return std::chrono::duration_cast<std::chrono::microseconds>(b - a).count();
    };

    std::cout << "      " << kCount << " 次 area() 调用:\n";
    std::cout << "        动态多态（虚函数）: " << us(t1, t2) << " us，结果 " << dyn_sum << "\n";
    std::cout << "        静态多态（CRTP）  : " << us(t2, t3) << " us，结果 " << static_sum << "\n";
    std::cout << "      注意事项:\n";
    std::cout << "        1. 本 demo 由构建脚本用 -O0 编译，差距被放大；\n";
    std::cout << "           用 -O2 重测，两者分别约 6400 us 和 1600 us（仍约 4 倍）\n";
    std::cout << "        2. 这个对比并不完全公平：动态版本的元素分散在堆上，\n";
    std::cout << "           静态版本是连续数组，缓存局部性也不同\n";
    std::cout << "        3. 结论只说明：虚调用无法内联，且多一次间接跳转\n";
}

// ---------- 4. 对象大小对比 ----------
void size_comparison() {
    std::cout << "      动态多态:\n";
    std::cout << "        sizeof(dynamic_poly::Circle) = "
              << sizeof(dynamic_poly::Circle) << "（double + 虚表指针）\n";
    std::cout << "      静态多态:\n";
    std::cout << "        sizeof(static_poly::Circle)  = "
              << sizeof(static_poly::Circle) << "（只有 double，空基类优化）\n";
    std::cout << "      CRTP 没有虚表指针，对象更小\n";
}

// ---------- 5. CRTP 的典型用途一：给类注入通用能力 ----------
namespace crtp_mixin {
    // 混入比较运算符：派生类只需提供 operator<
    template <class Derived>
    class Comparable {
    public:
        friend bool operator>(const Derived& a, const Derived& b)  { return b < a; }
        friend bool operator<=(const Derived& a, const Derived& b) { return !(b < a); }
        friend bool operator>=(const Derived& a, const Derived& b) { return !(a < b); }
        friend bool operator!=(const Derived& a, const Derived& b) {
            return (a < b) || (b < a);
        }
        friend bool operator==(const Derived& a, const Derived& b) {
            return !(a < b) && !(b < a);
        }
    };

    class Version : public Comparable<Version> {
    public:
        Version(int major, int minor) : _major(major), _minor(minor) {}
        friend bool operator<(const Version& a, const Version& b) {
            return a._major != b._major ? a._major < b._major : a._minor < b._minor;
        }
        std::string str() const {
            return std::to_string(_major) + "." + std::to_string(_minor);
        }
    private:
        int _major, _minor;
    };

    void demo() {
        Version v1(1, 2), v2(1, 5);
        std::cout << "        " << v1.str() << " < "  << v2.str() << " : " << (v1 < v2) << "\n";
        std::cout << "        " << v1.str() << " > "  << v2.str() << " : " << (v1 > v2) << "\n";
        std::cout << "        " << v1.str() << " <= " << v2.str() << " : " << (v1 <= v2) << "\n";
        std::cout << "        " << v1.str() << " == " << v2.str() << " : " << (v1 == v2) << "\n";
        std::cout << "      只写了 operator<，其余 5 个由 CRTP 基类生成\n";
        std::cout << "      （C++20 的 <=> 运算符让这个用法大部分变得不必要）\n";
    }
}

// ---------- 6. CRTP 的典型用途二：统计实例个数 ----------
namespace crtp_counter {
    template <class Derived>
    class Counted {
    public:
        Counted() { ++_count; }
        Counted(const Counted&) { ++_count; }
        ~Counted() { --_count; }
        static int count() { return _count; }
    private:
        static inline int _count = 0;     // 每个 Derived 有独立的计数器！
    };

    class Widget : public Counted<Widget> {};
    class Gadget : public Counted<Gadget> {};

    void demo() {
        Widget w1, w2, w3;
        Gadget g1;
        std::cout << "        Widget::count() = " << Widget::count() << "\n";
        std::cout << "        Gadget::count() = " << Gadget::count() << "\n";
        std::cout << "      因为 Counted<Widget> 和 Counted<Gadget> 是不同的类型，\n";
        std::cout << "      所以各有一份独立的静态成员 —— 这是 CRTP 的独特能力\n";
    }
}

// ---------- 7. 两种多态的取舍 ----------
void tradeoffs() {
    std::cout << "      动态多态（虚函数）适合:\n";
    std::cout << "        - 运行期才知道具体类型（插件、配置驱动、用户输入）\n";
    std::cout << "        - 需要把不同类型放进同一个容器\n";
    std::cout << "        - 需要跨编译单元/动态库的接口（ABI 边界）\n";
    std::cout << "        - 类型数量多且经常变化\n";
    std::cout << "      静态多态（CRTP / 模板）适合:\n";
    std::cout << "        - 编译期就知道类型\n";
    std::cout << "        - 性能关键路径（需要内联）\n";
    std::cout << "        - 给一批类注入通用能力（mixin）\n";
    std::cout << "        - 不需要异构容器\n";
    std::cout << "      代价对比:\n";
    std::cout << "        虚函数：对象多一个指针、调用多一次间接跳转、无法内联\n";
    std::cout << "        CRTP  ：代码膨胀、编译变慢、错误信息难读、无法异构容器\n";
}

int main() {
    std::cout << "--- 1. 性能对比 ---\n";        performance();
    std::cout << "--- 2. 对象大小 ---\n";        size_comparison();
    std::cout << "--- 3. CRTP 混入能力 ---\n";   crtp_mixin::demo();
    std::cout << "--- 4. CRTP 实例计数 ---\n";   crtp_counter::demo();
    std::cout << "--- 5. 取舍 ---\n";            tradeoffs();
}
