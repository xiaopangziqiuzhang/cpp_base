// std=c++17
// 对应章节: blog/part1_basics/01-06_bool_nullptr_enum.md
// 主题: bool、NULL 的问题与 nullptr、enum 的问题与 enum class
//
// 注意：本文件【故意】保留一个 GCC 警告，它正是第 5 节要演示的问题：
//   warning: comparison between 'enum Color' and 'enum Fruit' [-Wenum-compare]
// 传统 enum 允许跨类型比较，而 enum class 不允许。

#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>

// ---------- 1. bool 类型 ----------
void bool_demo() {
    bool b1 = -100;    // 任何非零值 -> true
    bool b2 = 100;
    bool b3 = 0;       // 零 -> false
    bool b4 = 1;

    int x = true;      // true -> 1
    int y = false;     // false -> 0

    std::cout << "bool(-100)=" << b1 << " bool(100)=" << b2
              << " bool(0)=" << b3 << " bool(1)=" << b4 << "\n";
    std::cout << "int(true)=" << x << " int(false)=" << y << "\n";
    std::cout << "sizeof(bool) = " << sizeof(bool) << "\n";

    // std::boolalpha 让输出变成 true/false
    std::cout << std::boolalpha << "boolalpha: " << b2 << " " << b3
              << std::noboolalpha << "\n";

    // 指针也能隐式转 bool，这是惯用的空检查写法
    int* p = nullptr;
    if (!p) std::cout << "空指针转 bool 得到 false\n";
}

// ---------- 2. NULL 的问题一：重载决议走错分支 ----------
void overloaded(int x)    { std::cout << "  overloaded(int): " << x << "\n"; }
void overloaded(char* p)  { std::cout << "  overloaded(char*): " << (p ? p : "null") << "\n"; }

void null_problem_overload() {
    // overloaded(NULL);
    //   GCC 下 NULL 展开为 __null，类型是 long。
    //   long -> int 和 long -> char* 都算"转换"，同级，于是二义性编译错误：
    //     error: call of overloaded 'overloaded(NULL)' is ambiguous
    //   MSVC 下 NULL 就是 0，会静默选中 int 版本——同样不是你想要的。
    //   两种实现，两种错法，这正是问题所在。
    std::cout << "传 NULL:    编译错误（GCC: ambiguous）/ 静默选 int（MSVC）\n";

    std::cout << "传 0:       "; overloaded(0);        // 静默选中 int 版本
    std::cout << "传 nullptr: "; overloaded(nullptr);  // 正确选中指针版本
}

// ---------- 3. NULL 的问题二：模板里彻底失效 ----------
template <class F, class P>
void call_with_ptr(F f, P ptr) {
    f(ptr);
}

void takes_pointer(int* p) { std::cout << "  takes_pointer: " << (p ? "非空" : "空") << "\n"; }

void null_problem_template() {
    // call_with_ptr(takes_pointer, NULL);  // 编译错误：P 推导为 long，不能转 int*
    call_with_ptr(takes_pointer, nullptr);  // nullptr 的类型是 std::nullptr_t，正确
    std::cout << "  NULL 在模板中会被推导成整型，nullptr 不会\n";
}

// ---------- 4. nullptr 的类型 ----------
void nullptr_type() {
    // 实测 GCC: NULL 是整型，且恰好是 long
    std::cout << "NULL 是整型吗: " << std::is_integral<decltype(NULL)>::value << "\n";
    std::cout << "NULL 就是 long 吗: " << std::is_same<decltype(NULL), long>::value << "\n";
    std::cout << "NULL 是指针吗: " << std::is_pointer<decltype(NULL)>::value << "\n";
    std::cout << "nullptr 的类型是 nullptr_t 吗: "
              << std::is_same<decltype(nullptr), std::nullptr_t>::value << "\n";

    // nullptr 可以转成任意指针类型，但不能转成整型
    int* p1 = nullptr;
    double* p2 = nullptr;
    void (*pf)() = nullptr;
    // int n = nullptr;     // 编译错误
    std::cout << "nullptr 可赋给各种指针: " << (p1 == nullptr) << (p2 == nullptr)
              << (pf == nullptr) << "\n";
}

// ---------- 5. 传统 enum 的三个问题 ----------
namespace traditional {
    enum Color { kRed, kGreen, kBlue };
    enum Fruit { kApple, kBanana /*, kRed */ };  // 取消注释会重定义 kRed

    void problems() {
        Color c = kRed;

        // 问题一：隐式转成整型，各种无意义的比较都能通过
        int n = c;                      // 不需要任何转换
        bool weird = (kRed == kApple);  // 两个不同枚举比较，编译通过！
        std::cout << "  传统 enum 隐式转 int: " << n << "\n";
        std::cout << "  kRed == kApple 竟然能编译，结果=" << weird << "\n";

        // 问题二：枚举值泄漏到外层作用域（所以上面要写 kRed/kApple 而不能重名）
        std::cout << "  枚举值直接在外层可见: kGreen=" << kGreen << "\n";

        // 问题三：底层类型由实现决定，无法指定
        std::cout << "  sizeof(Color) = " << sizeof(Color) << "（标准未规定具体值）\n";
    }
}

// ---------- 6. enum class 解决了这三个问题 [C++11] ----------
namespace modern {
    enum class Color { Red, Green, Blue };          // 强作用域
    enum class Fruit { Apple, Banana, Red };        // Red 可以重名，互不干扰

    // 可以指定底层类型
    enum class Status : std::uint8_t { Ok = 0, Error = 1, Timeout = 2 };

    void benefits() {
        Color c = Color::Red;           // 必须加作用域限定

        // int n = c;                   // 编译错误：不能隐式转整型
        int n = static_cast<int>(c);    // 要转就显式转
        std::cout << "  enum class 需显式转换: " << n << "\n";

        // bool bad = (Color::Red == Fruit::Red);   // 编译错误：类型不同，不可比较
        std::cout << "  Color::Red 与 Fruit::Red 不可比较（类型安全）\n";

        std::cout << "  sizeof(Status) = " << sizeof(Status) << "（显式指定为 uint8_t）\n";

        // 同类型之间的比较是允许的
        std::cout << "  Color::Red == Color::Red: " << (Color::Red == Color::Red) << "\n";
    }

    // 给 enum class 加输出支持的常见写法
    const char* to_string(Status s) {
        switch (s) {
            case Status::Ok:      return "Ok";
            case Status::Error:   return "Error";
            case Status::Timeout: return "Timeout";
        }
        return "Unknown";
    }
}

// ---------- 7. 传统 enum 也能指定底层类型 [C++11] ----------
enum LegacyWithType : short { kSmall = 1, kBig = 2 };

void enum_underlying() {
    std::cout << "sizeof(LegacyWithType) = " << sizeof(LegacyWithType) << "\n";
    std::cout << "underlying_type 是 short 吗: "
              << std::is_same<std::underlying_type_t<LegacyWithType>, short>::value << "\n";
}

int main() {
    std::cout << "--- 1. bool ---\n";                bool_demo();
    std::cout << "--- 2. NULL 重载问题 ---\n";       null_problem_overload();
    std::cout << "--- 3. NULL 模板问题 ---\n";       null_problem_template();
    std::cout << "--- 4. nullptr 类型 ---\n";        nullptr_type();
    std::cout << "--- 5. 传统 enum 的问题 ---\n";    traditional::problems();
    std::cout << "--- 6. enum class [C++11] ---\n";  modern::benefits();
    std::cout << "    to_string(Timeout) = " << modern::to_string(modern::Status::Timeout) << "\n";
    std::cout << "--- 7. 指定底层类型 ---\n";        enum_underlying();
}
