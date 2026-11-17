// std=c++17
// 对应章节: blog/part1_basics/01-09_literals.md
// 主题: 原始字符串、用户自定义字面量、数字分隔符、属性

#include <chrono>
#include <complex>
#include <iostream>
#include <string>

// ---------- 1. 原始字符串字面量 [C++11] ----------
void raw_string() {
    // 传统写法：每个反斜杠都要转义
    const char* path_old = "C:\\Users\\name\\Documents\\file.txt";
    // 原始字符串：R"(...)"，内部所有字符原样保留
    const char* path_new = R"(C:\Users\name\Documents\file.txt)";

    std::cout << "  传统: " << path_old << "\n";
    std::cout << "  原始: " << path_new << "\n";

    // 正则表达式受益最大：匹配一个反斜杠的正则
    const char* re_old = "\\\\d+";      // 四个反斜杠才能表示正则里的 \\d
    const char* re_new = R"(\\d+)";
    std::cout << "  正则(传统): " << re_old << "   正则(原始): " << re_new << "\n";

    // 内容里含 )" 时，用自定义分隔符 R"delim(...)delim"
    const char* tricky = R"json({"key": "value with )\" inside"})json";
    std::cout << "  自定义分隔符: " << tricky << "\n";

    // 原始字符串可以跨行，换行被原样保留
    const char* multi = R"(第一行
第二行
第三行)";
    std::cout << "  跨行:\n" << multi << "\n";
}

// ---------- 2. 数字分隔符 [C++14] 与各种进制字面量 ----------
void digit_separator() {
    int million = 1'000'000;              // [C++14] 单引号做分隔符，编译器忽略
    long long big = 9'223'372'036'854'775'807LL;
    int binary = 0b1010'1010;             // [C++11] 二进制字面量
    int hex = 0xFF'FF;
    int octal = 0755;                     // 老式八进制

    std::cout << "  1'000'000 = " << million << "\n";
    std::cout << "  0b1010'1010 = " << binary << "\n";
    std::cout << "  0xFF'FF = " << hex << "  0755 = " << octal << "\n";
    std::cout << "  big = " << big << "\n";

    // 实用场景：掩码和大常量一眼能看清位数
    constexpr unsigned kMask = 0b1111'0000'1111'0000;
    std::cout << "  kMask = " << kMask << " (0b" << "1111000011110000" << ")\n";
}

// ---------- 3. 标准库提供的字面量后缀 ----------
void standard_literals() {
    using namespace std::string_literals;        // s 后缀
    using namespace std::chrono_literals;        // h/min/s/ms/us/ns 后缀

    // 不加 s 后缀时，"abc" 是 const char*，无法直接用 string 的成员函数
    auto cstr = "hello";                         // const char*
    auto sstr = "hello"s;                        // std::string [C++14]
    std::cout << "  \"hello\" 的类型是 string 吗: "
              << std::is_same<decltype(sstr), std::string>::value << "\n";
    std::cout << "  相加: " << ("hello"s + ", world") << "\n";
    (void)cstr;

    // chrono 字面量让时间单位写在代码里 [C++14]
    auto timeout = 500ms;
    auto hour = 1h;
    std::cout << "  500ms = " << timeout.count() << " ms\n";
    std::cout << "  1h 换算成分钟 = "
              << std::chrono::duration_cast<std::chrono::minutes>(hour).count() << "\n";

    // 复数字面量 [C++14]
    using namespace std::complex_literals;
    auto z = 1.0 + 2.0i;
    std::cout << "  复数 1.0+2.0i 的模 = " << std::abs(z) << "\n";
}

// ---------- 4. 用户自定义字面量 [C++11] ----------
// 形式一：数值字面量 —— 参数必须是 unsigned long long / long double / const char*
constexpr long double operator""_km(long double v) { return v * 1000.0L; }   // 转成米
constexpr long double operator""_m(long double v)  { return v; }
constexpr long double operator""_cm(long double v) { return v / 100.0L; }

// 形式二：整型字面量
constexpr unsigned long long operator""_KB(unsigned long long v) { return v * 1024; }
constexpr unsigned long long operator""_MB(unsigned long long v) { return v * 1024 * 1024; }

// 形式三：字符串字面量
std::string operator""_upper(const char* s, std::size_t len) {
    std::string result(s, len);
    for (char& c : result) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    return result;
}

// 带单位的强类型，避免单位混用的 bug
class Temperature {
public:
    constexpr explicit Temperature(double celsius) : _celsius(celsius) {}
    constexpr double celsius() const { return _celsius; }
    constexpr double fahrenheit() const { return _celsius * 9.0 / 5.0 + 32.0; }
private:
    double _celsius;
};
constexpr Temperature operator""_degC(long double v) {
    return Temperature(static_cast<double>(v));
}

void user_defined_literals() {
    // 单位换算在编译期完成
    constexpr long double distance = 1.5_km + 300.0_m + 50.0_cm;
    std::cout << "  1.5_km + 300.0_m + 50.0_cm = " << static_cast<double>(distance) << " 米\n";

    constexpr auto buffer_size = 4_KB;
    constexpr auto file_limit = 16_MB;
    std::cout << "  4_KB = " << buffer_size << " 字节, 16_MB = " << file_limit << " 字节\n";

    std::cout << "  \"hello\"_upper = " << "hello"_upper << "\n";

    constexpr auto body_temp = 36.5_degC;
    std::cout << "  36.5_degC = " << body_temp.fahrenheit() << " 华氏度\n";

    // static_assert 证明这些都是编译期常量
    static_assert(4_KB == 4096, "");
    static_assert(body_temp.celsius() == 36.5, "");
}

// ---------- 5. 属性 [[...]] ----------
// [[noreturn]]：告诉编译器这个函数不会返回
[[noreturn]] void fail(const std::string& msg) {
    std::cout << "  [[noreturn]] fail(): " << msg << "\n";
    std::exit(0);     // 必须真的不返回，否则是未定义行为
}

// [[nodiscard]] [C++17]：返回值被忽略时产生警告
[[nodiscard]] int must_check() { return 42; }

// [[nodiscard]] 用在类型上：所有返回该类型的函数都受保护
// 注意位置：属性必须跟在 struct/class 关键字之后，写在前面会被忽略
// （GCC 提示: attribute for 'struct ErrorCode' must follow the 'struct' keyword）
struct [[nodiscard]] ErrorCode { int value; };
ErrorCode do_work() { return ErrorCode{0}; }

// [[deprecated]] [C++14]：标记废弃接口
[[deprecated("请改用 new_api()")]] void old_api() {}
void new_api() { std::cout << "  new_api() 被调用\n"; }

// [[maybe_unused]] [C++17]：抑制"未使用"警告
void maybe_unused_demo([[maybe_unused]] int debug_only_param) {
    std::cout << "  [[maybe_unused]] 抑制了未使用参数的警告\n";
}

int fallthrough_demo(int x) {
    int result = 0;
    switch (x) {
        case 1:
            result += 1;
            [[fallthrough]];      // [C++17] 显式声明"这里故意不 break"
        case 2:
            result += 10;
            break;
        default:
            result = -1;
    }
    return result;
}

void attributes_demo() {
    int value = must_check();        // 用了返回值，没有警告
    std::cout << "  must_check() = " << value << "\n";
    // must_check();                 // 会产生 -Wunused-result 警告

    ErrorCode ec = do_work();        // 类型上的 [[nodiscard]] 同样生效
    std::cout << "  do_work().value = " << ec.value << "\n";
    // do_work();                    // 忽略返回值会告警

    new_api();
    // old_api();                    // 会产生 -Wdeprecated-declarations 警告

    maybe_unused_demo(1);
    std::cout << "  fallthrough_demo(1) = " << fallthrough_demo(1)
              << " （1 落穿到 case 2，所以是 11）\n";
    std::cout << "  fallthrough_demo(2) = " << fallthrough_demo(2) << "\n";
}

int main() {
    std::cout << "--- 1. 原始字符串 [C++11] ---\n";      raw_string();
    std::cout << "--- 2. 数字分隔符 [C++14] ---\n";      digit_separator();
    std::cout << "--- 3. 标准库字面量 ---\n";            standard_literals();
    std::cout << "--- 4. 自定义字面量 [C++11] ---\n";    user_defined_literals();
    std::cout << "--- 5. 属性 ---\n";                    attributes_demo();
}
