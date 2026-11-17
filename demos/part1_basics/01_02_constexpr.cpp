// std=c++17
// 对应章节: blog/part1_basics/01-02_const_constexpr.md
// 主题: constexpr 从 C++11 到 C++17 的演进

#include <array>
#include <iostream>
#include <string_view>

// ---------- 1. C++11 的 constexpr 函数：只能有一条 return ----------
constexpr int factorial_cxx11(int n) {
    return n <= 1 ? 1 : n * factorial_cxx11(n - 1);   // 只能用三目递归
}

// ---------- 2. C++14 放宽：允许局部变量、循环、分支 ----------
constexpr int factorial_cxx14(int n) {
    int result = 1;
    for (int i = 2; i <= n; ++i) {   // C++11 下这是编译错误
        result *= i;
    }
    return result;
}

constexpr int fib(int n) {
    if (n <= 1) return n;            // C++14 起可以用 if 语句
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        int t = a + b;
        a = b;
        b = t;
    }
    return b;
}

// ---------- 3. constexpr 的核心价值：编译期就算完 ----------
void compile_time_eval() {
    // 这些值在编译期算出，运行时是立即数
    constexpr int f5 = factorial_cxx11(5);
    constexpr int f10 = factorial_cxx14(10);
    constexpr int f20 = fib(20);

    // static_assert 能通过，证明是编译期常量
    static_assert(f5 == 120, "");
    static_assert(f10 == 3628800, "");
    static_assert(f20 == 6765, "");

    std::cout << "5! = " << f5 << ", 10! = " << f10 << ", fib(20) = " << f20 << "\n";

    // constexpr 函数也能在运行期调用（传入运行期值时）
    int n = 6;
    std::cout << "运行期调用 factorial(6) = " << factorial_cxx14(n) << "\n";
}

// ---------- 4. constexpr 用作数组大小、模板参数 ----------
constexpr std::size_t kBufferSize = 16;

void as_constant_expression() {
    int buf[kBufferSize]{};              // const int 也行，但 constexpr 意图更明确
    std::array<double, kBufferSize> arr{};
    std::cout << "buf 元素个数 = " << sizeof(buf) / sizeof(buf[0])
              << ", arr.size() = " << arr.size() << "\n";
}

// ---------- 5. const 与 constexpr 的区别 ----------
int runtime_value() { return 42; }

void const_vs_constexpr() {
    const int a = runtime_value();     // ok：const 只要求初始化后不变
    // constexpr int b = runtime_value();  // 编译错误：必须是编译期常量

    const int c = 10;                  // 这个恰好也是编译期常量
    constexpr int d = 10;

    int arr1[c]{};                     // ok
    int arr2[d]{};                     // ok
    // int arr3[a]{};                  // 编译错误：a 不是常量表达式

    std::cout << "a=" << a << " c=" << c << " d=" << d
              << " sizeof(arr1)=" << sizeof(arr1) << " sizeof(arr2)=" << sizeof(arr2) << "\n";
}

// ---------- 6. constexpr 构造函数与字面类型 ----------
class Point {
public:
    constexpr Point(int x, int y) : _x(x), _y(y) {}
    constexpr int x() const { return _x; }
    constexpr int y() const { return _y; }
    // C++14 起 constexpr 成员函数可以修改成员（非 const 成员函数）
    constexpr void move(int dx, int dy) { _x += dx; _y += dy; }
private:
    int _x, _y;
};

constexpr Point translate(Point p, int dx, int dy) {
    p.move(dx, dy);
    return p;
}

void constexpr_class() {
    constexpr Point origin(0, 0);
    constexpr Point moved = translate(origin, 3, 4);

    static_assert(moved.x() == 3 && moved.y() == 4, "编译期完成了对象构造和计算");
    std::cout << "moved = (" << moved.x() << ", " << moved.y() << ")\n";
}

// ---------- 7. if constexpr [C++17]：编译期分支 ----------
template <class T>
auto describe(T value) {
    if constexpr (std::is_integral_v<T>) {
        // 只有 T 是整型时，这个分支才会被编译
        return value * 2;
    } else if constexpr (std::is_floating_point_v<T>) {
        return value / 2.0;
    } else {
        return std::string_view("unknown type");
    }
}

void if_constexpr_demo() {
    std::cout << "describe(21)    = " << describe(21) << "\n";
    std::cout << "describe(3.0)   = " << describe(3.0) << "\n";
    std::cout << "describe(\"hi\") = " << describe("hi") << "\n";
}

// ---------- 8. inline 变量 [C++17]：头文件里定义全局常量 ----------
// C++17 之前：头文件里写 const 对象会每个 TU 一份副本；
// 想共享就得 extern 声明 + 某个 .cpp 里定义。
inline constexpr double kPi = 3.14159265358979323846;   // 可以直接放头文件

void inline_variable_demo() {
    std::cout << "kPi = " << kPi << "\n";
}

int main() {
    std::cout << "--- 1-3. 编译期求值 ---\n";       compile_time_eval();
    std::cout << "--- 4. 作常量表达式 ---\n";       as_constant_expression();
    std::cout << "--- 5. const vs constexpr ---\n"; const_vs_constexpr();
    std::cout << "--- 6. constexpr 类 ---\n";       constexpr_class();
    std::cout << "--- 7. if constexpr [C++17] ---\n"; if_constexpr_demo();
    std::cout << "--- 8. inline 变量 [C++17] ---\n";  inline_variable_demo();
}
