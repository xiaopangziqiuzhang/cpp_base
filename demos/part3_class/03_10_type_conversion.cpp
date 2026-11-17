// std=c++17
// 对应章节: blog/part3_class/03-10_type_conversion.md
// 主题: 转换构造函数、类型转换运算符、explicit、二义性

#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// ---------- 1. 由其他类型 -> 自定义类型：转换构造函数 ----------
namespace to_class {
    class Meters {
    public:
        // 单参数构造函数就是"转换构造函数"
        Meters(double value) : _value(value) {
            std::cout << "        Meters(double) 被调用\n";
        }
        double value() const { return _value; }
    private:
        double _value;
    };

    void takes_meters(const Meters& m) {
        std::cout << "        收到 " << m.value() << " 米\n";
    }

    void demo() {
        Meters m1(5.0);              // 直接构造
        Meters m2 = 3.0;             // 隐式转换（拷贝初始化）
        takes_meters(7.0);            // 实参的隐式转换

        std::cout << "      单参数构造函数会成为【隐式转换通道】\n";
        std::cout << "      m2.value() = " << m2.value() << "\n";
        std::cout << "      多参数构造函数在 C++11 后也能通过 {} 参与隐式转换\n";
        (void)m1;
    }
}

// ---------- 2. explicit 禁止隐式转换 ----------
namespace explicit_ctor {
    class Seconds {
    public:
        explicit Seconds(double v) : _value(v) {}
        double value() const { return _value; }
    private:
        double _value;
    };

    void takes_seconds(const Seconds& s) {
        std::cout << "        收到 " << s.value() << " 秒\n";
    }

    void demo() {
        Seconds s1(30.0);            // 直接构造，ok
        // Seconds s2 = 30.0;        // 编译错误
        // takes_seconds(30.0);      // 编译错误
        takes_seconds(Seconds(30.0)); // 必须显式

        std::cout << "      加了 explicit 后，隐式转换被禁止\n";
        std::cout << "      这防止了 takes_seconds(30.0) 这种\"单位不明\"的调用\n";
        std::cout << "      s1.value() = " << s1.value() << "\n";
    }
}

// ---------- 3. 由自定义类型 -> 其他类型：类型转换运算符 ----------
namespace from_class {
    class Fraction {
    public:
        Fraction(int num, int den) : _num(num), _den(den) {}

        // 类型转换运算符：operator 目标类型()
        //   注意：没有返回类型声明（返回类型就是运算符名）
        operator double() const {
            std::cout << "        operator double() 被调用\n";
            return static_cast<double>(_num) / _den;
        }

        // 也可以转成其他类类型
        operator std::string() const {
            return std::to_string(_num) + "/" + std::to_string(_den);
        }

    private:
        int _num, _den;
    };

    void demo() {
        Fraction f(3, 4);

        double d = f;                        // 隐式调用 operator double()
        std::cout << "      double d = f 得到 " << d << "\n";

        double sum = f + 1.0;                // 也会隐式转换
        std::cout << "      f + 1.0 = " << sum << "\n";

        std::string s = f;                   // 调用 operator string()
        std::cout << "      string s = f 得到 " << s << "\n";

        std::cout << "      转换运算符的语法: operator 类型() const\n";
        std::cout << "        没有返回类型（运算符名就是返回类型）\n";
        std::cout << "        没有参数\n";
        std::cout << "        通常应该是 const 的\n";
    }
}

// ---------- 4. 隐式转换运算符的危险 ----------
namespace implicit_danger {
    class BadBool {
    public:
        explicit BadBool(int v) : _value(v) {}
        operator bool() const { return _value != 0; }     // 隐式转 bool：危险
        int raw() const { return _value; }
    private:
        int _value;
    };

    void demo() {
        BadBool a(5), b(3);

        std::cout << "      隐式 operator bool 允许这些无意义的操作:\n";
        std::cout << "        a + b  = " << (a + b)
                  << "  <- 两个 bool 相加！不是 5+3\n";
        std::cout << "        a < b  = " << (a < b)
                  << "  <- 比较的是 true < true\n";
        std::cout << "        a == b = " << (a == b)
                  << "  <- 都转成 true，所以相等\n";
        std::cout << "      因为 bool 可以继续提升为 int，所以什么算术都能做\n";
    }

    // 正确做法：explicit operator bool
    class GoodBool {
    public:
        explicit GoodBool(int v) : _value(v) {}
        explicit operator bool() const { return _value != 0; }
        int raw() const { return _value; }
    private:
        int _value;
    };

    void demo_fixed() {
        GoodBool a(5), b(3);
        // std::cout << (a + b);        // 编译错误，正是我们想要的

        // 但在"需要 bool 的上下文"中仍然自动转换（这叫 contextual conversion）
        if (a) std::cout << "      explicit operator bool 在 if 中仍然自动生效\n";
        if (a && b) std::cout << "      && 和 || 也算 bool 上下文\n";
        while (false) { }
        std::cout << "      !a = " << !a << "（! 也是 bool 上下文）\n";
        std::cout << "      这是 C++11 对 explicit 转换运算符的特殊规定:\n";
        std::cout << "        if / while / for / !  / && / || / ?: 中会自动转换\n";
        std::cout << "        其他场合必须显式 static_cast<bool>(x)\n";
        std::cout << "      所有智能指针和流都用这个模式（见 02-05、07-07）\n";
    }
}

// ---------- 5. 两个方向的转换同时存在 -> 二义性 ----------
namespace ambiguity {
    class B;

    class A {
    public:
        A() = default;
        A(const B&);                       // B -> A（转换构造函数）
        int tag() const { return 1; }
    };

    class B {
    public:
        B() = default;
        operator A() const;                // B -> A（转换运算符）
        int tag() const { return 2; }
    };

    A::A(const B&) {}
    B::operator A() const { return A{}; }

    void takes_a(const A&) {}

    void demo() {
        B b;
        // takes_a(b);      // 编译错误：二义性！
        //   error: conversion from 'B' to 'const A&' is ambiguous
        //   候选 1: A::A(const B&)
        //   候选 2: B::operator A()
        takes_a(static_cast<A>(b));          // 显式指定一种

        std::cout << "      同一对类型之间【只提供一个方向的一种转换】\n";
        std::cout << "      同时有 A(const B&) 和 B::operator A() 会二义性\n";
        std::cout << "      规则: 要么由目标类提供构造函数，要么由源类提供转换运算符\n";
    }
}

// ---------- 6. 隐式转换最多走一步 ----------
namespace one_step {
    class Celsius {
    public:
        explicit Celsius(double v) : _v(v) {}
        double value() const { return _v; }
    private:
        double _v;
    };

    class Fahrenheit {
    public:
        Fahrenheit(const Celsius& c) : _v(c.value() * 9 / 5 + 32) {}   // Celsius -> Fahrenheit
        double value() const { return _v; }
    private:
        double _v;
    };

    void takes_fahrenheit(const Fahrenheit& f) {
        std::cout << "        收到 " << f.value() << " 华氏度\n";
    }

    void demo() {
        Celsius c(100.0);
        takes_fahrenheit(c);                  // 一步转换：Celsius -> Fahrenheit，ok

        // takes_fahrenheit(100.0);           // 编译错误！需要两步:
        //   double -> Celsius（但 Celsius 的构造函数是 explicit，这步本身就不行）
        //   Celsius -> Fahrenheit
        takes_fahrenheit(Fahrenheit(Celsius(100.0)));   // 显式写出每一步

        std::cout << "      【用户定义的隐式转换最多只走一步】\n";
        std::cout << "        内置转换可以和它组合（如 int -> double -> MyClass）\n";
        std::cout << "        但不能连续两次用户定义的转换\n";
        std::cout << "      这个限制是有意的 —— 否则转换路径会难以预测\n";
    }
}

// ---------- 7. 实践：什么时候该提供转换 ----------
namespace practice {
    // 正当用途一：数值类型的包装（强类型单位）
    class Bytes {
    public:
        explicit constexpr Bytes(std::size_t n) : _n(n) {}
        constexpr std::size_t count() const { return _n; }
        // 不提供隐式转换到 size_t —— 否则就失去了强类型的意义
    private:
        std::size_t _n;
    };

    constexpr Bytes operator""_KB(unsigned long long n) { return Bytes(n * 1024); }

    // 正当用途二：和标准类型的互操作（有意的隐式转换）
    class MyString {
    public:
        MyString(const char* s) : _data(s) {}          // 故意不加 explicit
        MyString(std::string s) : _data(std::move(s)) {}

        // 提供显式转换，不提供隐式
        explicit operator std::string() const { return _data; }
        const std::string& str() const { return _data; }        // 更推荐：具名函数
    private:
        std::string _data;
    };

    void takes_mystring(const MyString& s) {
        std::cout << "        收到 \"" << s.str() << "\"\n";
    }

    void demo() {
        constexpr auto buf = 4_KB;
        std::cout << "      强类型单位: 4_KB = " << buf.count() << " 字节\n";
        std::cout << "        不提供隐式转换，所以不会和裸 size_t 混用\n";

        takes_mystring("字面量");                  // 依赖隐式转换，这是设计意图
        std::cout << "      MyString 故意允许 const char* 隐式转换（像 std::string）\n";

        MyString ms("hello");
        std::string s = static_cast<std::string>(ms);     // 显式
        std::cout << "      转出用显式转换或具名函数: " << s << "\n";

        std::cout << "      实践清单:\n";
        std::cout << "        1. 单参数构造函数默认加 explicit\n";
        std::cout << "        2. 转换运算符默认加 explicit（operator bool 尤其）\n";
        std::cout << "        3. 优先提供【具名函数】而不是转换运算符\n";
        std::cout << "           （string 有 c_str()，不是 operator const char*）\n";
        std::cout << "        4. 同一对类型只提供一个方向的一种转换\n";
        std::cout << "        5. 例外: 确实想要\"像内置类型一样用\"时才允许隐式转换\n";
    }
}

// ---------- 8. 标准库的做法（值得模仿）----------
namespace stdlib_examples {
    void demo() {
        std::cout << "      std::string:\n";
        std::cout << "        允许 const char* -> string 隐式转换（方便）\n";
        std::cout << "        但转出用 c_str() / data()，【不是】operator const char*\n";
        std::cout << "        理由: 隐式转出会导致悬垂指针难以察觉\n";

        std::cout << "      智能指针:\n";
        std::cout << "        explicit operator bool() —— 能用于 if，不能参与算术\n";
        std::cout << "        unique_ptr 的构造函数是 explicit —— 防止裸指针隐式接管\n";

        std::cout << "      std::string_view:\n";
        std::cout << "        允许 string -> string_view 隐式转换（方便传参）\n";
        std::cout << "        但 string_view -> string 是 explicit（避免意外分配）\n";

        // 验证
        std::cout << "      实测验证:\n";
        std::cout << "        is_convertible<const char*, string> = "
                  << std::is_convertible_v<const char*, std::string> << "\n";
        std::cout << "        is_convertible<string, const char*> = "
                  << std::is_convertible_v<std::string, const char*> << "\n";
        std::cout << "        is_convertible<string, string_view> = "
                  << std::is_convertible_v<std::string, std::string_view> << "\n";
        std::cout << "        is_convertible<string_view, string> = "
                  << std::is_convertible_v<std::string_view, std::string>
                  << "  <- explicit，所以是 0\n";
    }
}

int main() {
    std::cout << "--- 1. 转换构造函数 ---\n";        to_class::demo();
    std::cout << "--- 2. explicit 构造函数 ---\n";   explicit_ctor::demo();
    std::cout << "--- 3. 类型转换运算符 ---\n";      from_class::demo();
    std::cout << "--- 4. 隐式转换的危险 ---\n";      implicit_danger::demo();
    std::cout << "--- 5. explicit operator bool ---\n"; implicit_danger::demo_fixed();
    std::cout << "--- 6. 二义性 ---\n";              ambiguity::demo();
    std::cout << "--- 7. 只走一步 ---\n";            one_step::demo();
    std::cout << "--- 8. 实践建议 ---\n";            practice::demo();
    std::cout << "--- 9. 标准库的做法 ---\n";        stdlib_examples::demo();
}
