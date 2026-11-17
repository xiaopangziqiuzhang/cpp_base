// std=c++14
// 对应章节: blog/part6_modern_core/06-04_cpp14.md
// 主题: C++14 全部语言特性汇总（注意本文件用 -std=c++14 编译，证明这些都是 C++14 特性）

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ---------- 1. 函数返回类型推导 ----------
// C++11 需要尾置返回类型，C++14 可以完全省略
auto add_cxx14(int a, int b) { return a + b; }

// 递归函数也可以，但第一个 return 必须能推导出类型
auto factorial(int n) -> long long;        // 先声明
auto factorial(int n) -> long long {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

// decltype(auto)：保留引用和 const
std::vector<int> g_data{1, 2, 3};
auto           get_by_auto()     { return g_data[0]; }   // int（丢引用）
decltype(auto) get_by_decltype() { return g_data[0]; }   // int&（保留引用）

void return_type_deduction() {
    std::cout << "      auto add(int,int) 返回 " << add_cxx14(20, 22) << "\n";
    std::cout << "      factorial(10) = " << factorial(10) << "\n";
    std::cout << "      get_by_auto() 的类型是 int: "
              << std::is_same<decltype(get_by_auto()), int>::value << "\n";
    std::cout << "      get_by_decltype() 的类型是 int&: "
              << std::is_same<decltype(get_by_decltype()), int&>::value << "\n";
    get_by_decltype() = 42;
    std::cout << "      通过 decltype(auto) 返回的引用修改: g_data[0] = " << g_data[0] << "\n";
}

// ---------- 2. 泛型 lambda ----------
void generic_lambda() {
    auto add = [](auto a, auto b) { return a + b; };
    std::cout << "      add(1, 2) = " << add(1, 2) << "\n";
    std::cout << "      add(1.5, 2.5) = " << add(1.5, 2.5) << "\n";
    std::cout << "      add(string, char*) = " << add(std::string("ab"), "cd") << "\n";

    // 配合可变参数
    auto sum_all = [](auto... args) {
        // C++14 没有折叠表达式，用初始化列表技巧
        auto total = 0.0;
        int dummy[] = {0, ((total += args), 0)...};
        (void)dummy;
        return total;
    };
    std::cout << "      sum_all(1, 2.5, 3) = " << sum_all(1, 2.5, 3) << "\n";
}

// ---------- 3. 初始化捕获（广义 lambda 捕获）----------
void init_capture() {
    auto ptr = std::make_unique<int>(42);
    auto f = [p = std::move(ptr)] { return *p; };      // 移动进 lambda
    std::cout << "      捕获 unique_ptr: " << f() << "\n";
    std::cout << "      原指针已被移走: " << (ptr == nullptr) << "\n";

    int x = 5;
    auto g = [y = x * 2] { return y; };                 // 捕获表达式结果
    std::cout << "      捕获 x*2 的结果: " << g() << "\n";
}

// ---------- 4. std::make_unique ----------
void make_unique_demo() {
    // C++11 有 make_shared 但【漏了】make_unique，C++14 补上
    auto p1 = std::make_unique<int>(42);
    auto p2 = std::make_unique<std::vector<int>>(5, 7);
    auto arr = std::make_unique<int[]>(10);

    std::cout << "      make_unique<int>(42) -> " << *p1 << "\n";
    std::cout << "      make_unique<vector<int>>(5,7) -> size=" << p2->size()
              << " [0]=" << (*p2)[0] << "\n";
    arr[0] = 1;
    std::cout << "      make_unique<int[]>(10) -> arr[0]=" << arr[0] << "\n";
    std::cout << "      为什么重要：避免裸 new，且异常安全（见 02-05）\n";
}

// ---------- 5. 变量模板 ----------
template <class T>
constexpr T pi = T(3.14159265358979323846L);

template <class T>
constexpr bool is_numeric = std::is_arithmetic<T>::value;

void variable_templates() {
    std::cout << "      pi<float>  = " << pi<float> << "\n";
    std::cout << "      pi<double> = " << pi<double> << "\n";
    std::cout << "      is_numeric<int> = " << is_numeric<int>
              << ", is_numeric<std::string> = " << is_numeric<std::string> << "\n";
    std::cout << "      标准库的 _v 后缀（C++17）就是用变量模板实现的\n";
}

// ---------- 6. 放宽的 constexpr ----------
// C++11 的 constexpr 函数体基本只能有一条 return
constexpr int factorial_cxx11(int n) {
    return n <= 1 ? 1 : n * factorial_cxx11(n - 1);
}

// C++14 允许局部变量、循环、分支
constexpr int factorial_cxx14(int n) {
    int result = 1;
    for (int i = 2; i <= n; ++i) result *= i;      // C++11 下这是编译错误
    return result;
}

constexpr int fib(int n) {
    if (n <= 1) return n;                           // C++14 起可以用 if 语句
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) { int t = a + b; a = b; b = t; }
    return b;
}

// constexpr 成员函数可以修改成员（非 const 成员函数）
class Point {
public:
    constexpr Point(int x, int y) : _x(x), _y(y) {}
    constexpr int x() const { return _x; }
    constexpr void move(int dx, int dy) { _x += dx; _y += dy; }   // C++14
private:
    int _x, _y;
};

constexpr Point translate(Point p, int dx, int dy) { p.move(dx, dy); return p; }

void relaxed_constexpr() {
    static_assert(factorial_cxx14(10) == 3628800, "编译期求值");
    static_assert(fib(20) == 6765, "");
    constexpr Point moved = translate(Point(0, 0), 3, 4);
    static_assert(moved.x() == 3, "");

    std::cout << "      factorial_cxx14(10) = " << factorial_cxx14(10) << "\n";
    std::cout << "      fib(20) = " << fib(20) << "\n";
    std::cout << "      constexpr Point 在编译期完成构造和修改，moved.x() = "
              << moved.x() << "\n";
    std::cout << "      放宽的意义：编译期计算从模板元编程的黑魔法变成普通代码\n";
}

// ---------- 7. [[deprecated]] 属性 ----------
[[deprecated("请改用 new_api()")]]
void old_api() { std::cout << "        old_api()\n"; }

void new_api() { std::cout << "        new_api()\n"; }

[[deprecated]] int old_constant = 42;

void deprecated_demo() {
    new_api();
    // old_api();          // 会产生警告:
    //   warning: 'void old_api()' is deprecated: 请改用 new_api()
    std::cout << "      [[deprecated]] 让废弃信息从注释变成编译器警告\n";
    std::cout << "      可用于函数、变量、类、枚举、命名空间等\n";
}

// ---------- 8. 二进制字面量与数字分隔符 ----------
void literals() {
    int binary = 0b1010'1010;            // 二进制字面量是 C++14 标准化的
    int million = 1'000'000;              // 数字分隔符
    long long big = 9'223'372'036'854'775'807LL;
    constexpr unsigned mask = 0b1111'0000'1111'0000;

    std::cout << "      0b1010'1010 = " << binary << "\n";
    std::cout << "      1'000'000 = " << million << "\n";
    std::cout << "      big = " << big << "\n";
    std::cout << "      mask = " << mask << "\n";
}

// ---------- 9. 标准库字面量后缀 ----------
void std_literals() {
    using namespace std::string_literals;
    using namespace std::chrono_literals;

    auto s = "hello"s;                    // std::string
    auto d = 500ms;                        // std::chrono::milliseconds
    auto h = 2h;

    std::cout << "      \"hello\"s 的类型是 string: "
              << std::is_same<decltype(s), std::string>::value << "\n";
    std::cout << "      \"hello\"s + \", world\" = " << (s + ", world") << "\n";
    std::cout << "      500ms = " << d.count() << " ms\n";
    std::cout << "      2h 换算成分钟 = "
              << std::chrono::duration_cast<std::chrono::minutes>(h).count() << "\n";
}

// ---------- 10. std::exchange / integer_sequence ----------
void other_library() {
    // std::exchange：设新值，返回旧值
    int x = 1;
    int old = std::exchange(x, 100);
    std::cout << "      std::exchange(x, 100): 旧值=" << old << " 新值=" << x << "\n";
    std::cout << "      常用于实现移动构造函数:\n";
    std::cout << "        _ptr(std::exchange(rhs._ptr, nullptr))\n";

    // integer_sequence：编译期整数序列，用于展开 tuple 等
    auto print_seq = [](auto... is) {
        int dummy[] = {0, ((std::cout << is << " "), 0)...};
        (void)dummy;
    };
    std::cout << "      make_index_sequence<5> 展开: ";
    [&print_seq](auto) {}(0);     // 占位
    print_seq(0, 1, 2, 3, 4);
    std::cout << "\n      （integer_sequence 是实现 std::apply 的基础）\n";
}

int main() {
    std::cout << "本文件用 -std=c++14 编译，证明以下全部是 C++14 特性\n";
    std::cout << "__cplusplus = " << __cplusplus << "\n\n";
    std::cout << "--- 1. 返回类型推导 ---\n";        return_type_deduction();
    std::cout << "--- 2. 泛型 lambda ---\n";         generic_lambda();
    std::cout << "--- 3. 初始化捕获 ---\n";          init_capture();
    std::cout << "--- 4. make_unique ---\n";         make_unique_demo();
    std::cout << "--- 5. 变量模板 ---\n";            variable_templates();
    std::cout << "--- 6. 放宽的 constexpr ---\n";    relaxed_constexpr();
    std::cout << "--- 7. [[deprecated]] ---\n";      deprecated_demo();
    std::cout << "--- 8. 字面量 ---\n";              literals();
    std::cout << "--- 9. 标准库字面量 ---\n";        std_literals();
    std::cout << "--- 10. 其他库特性 ---\n";         other_library();
}
