// std=c++17
// 对应章节: blog/part1_basics/01-07_inline.md
// 主题: inline 函数 vs 宏，inline 的现代语义

#include <iostream>
#include <string>

// ---------- 1. 宏的三个问题 ----------
#define MACRO_MAX(a, b) ((a) > (b) ? (a) : (b))
#define MACRO_SQUARE(x) ((x) * (x))

inline int inline_max(int x, int y) { return x > y ? x : y; }
inline int inline_square(int x)     { return x * x; }

void macro_problems() {
    // 问题一：参数多次求值
    int i = 5, j = 3;
    int r1 = MACRO_MAX(i++, j);
    std::cout << "  MACRO_MAX(i++, j): 结果=" << r1 << " i=" << i << "（i 被自增两次）\n";

    i = 5;
    int r2 = inline_max(i++, j);
    std::cout << "  inline_max(i++, j): 结果=" << r2 << " i=" << i << "（只求值一次）\n";

    // 问题二：没有类型检查
    std::cout << "  MACRO_SQUARE(2.5) = " << MACRO_SQUARE(2.5)
              << "（宏不检查类型，按 double 算了）\n";
    std::cout << "  inline_square(2.5) = " << inline_square(2.5)
              << "（函数做了类型转换，截断为 int）\n";

    // 问题三：优先级（即使加了括号也只能解决一部分）
#define BAD_MAX(a, b) (a) > (b) ? (a) : (b)
    int r3 = BAD_MAX(i, j) + 2;   // 展开: (i)>(j)?(i):(j)+2  —— 不是期望的结果
    int r4 = MACRO_MAX(i, j) + 2; // 加了外层括号才正确
    std::cout << "  BAD_MAX(i,j)+2 = " << r3 << "（优先级错误）"
              << "  MACRO_MAX(i,j)+2 = " << r4 << "（正确）\n";
#undef BAD_MAX
}

// ---------- 2. 宏无法操作类的私有成员，inline 可以 ----------
class Counter {
public:
    explicit Counter(int v) : _value(v) {}
    // 成员函数可以访问私有数据；宏做不到这件事
    inline int doubled() const { return _value * 2; }
    int value() const { return _value; }   // 类内定义的成员函数隐式是 inline
private:
    int _value;
};

void class_access() {
    Counter c(21);
    std::cout << "  c.doubled() = " << c.doubled() << "（访问了私有成员 _value）\n";
}

// ---------- 3. inline 关键字必须和定义体在一起 ----------
// 下面这种写法不会让 foo 成为内联函数：
//   inline void foo(int, int);     // 声明处写 inline 无效
//   void foo(int, int) { ... }     // 定义处没写
// 正确写法：
inline void bar(int x, int y) { std::cout << "  bar(" << x << "," << y << ")\n"; }

// ---------- 4. inline 是建议，不是命令 ----------
// 这个函数体很长且有循环，编译器很可能拒绝内联
inline long heavy_function(int n) {
    long sum = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < 10; ++j) {
            sum += static_cast<long>(i) * j;
        }
    }
    return sum;
}

// 反之，没写 inline 的小函数，编译器在 -O2 下通常会自动内联
static int small_add(int a, int b) { return a + b; }

void inline_is_a_hint() {
    std::cout << "  heavy_function(100) = " << heavy_function(100)
              << "（写了 inline，但编译器可能拒绝内联）\n";
    std::cout << "  small_add(1,2) = " << small_add(1, 2)
              << "（没写 inline，-O2 下通常会被内联）\n";
}

// ---------- 5. inline 的真正语义：允许多个翻译单元有相同定义 ----------
// 这是 inline 在现代 C++ 里的核心作用（而不是"展开代码"）。
// 如果去掉 inline 并把这个头文件包含到两个 .cpp 里，链接时会报
//   multiple definition of 'shared_helper'
inline const char* shared_helper() { return "可以安全地放在头文件里"; }

// [C++17] inline 变量：同样的道理，用于变量
inline constexpr int kMagic = 42;

void linkage_semantics() {
    std::cout << "  shared_helper() = " << shared_helper() << "\n";
    std::cout << "  kMagic = " << kMagic << "（inline 变量 [C++17]）\n";

    // inline 函数内的 static 局部变量，在所有翻译单元中是同一个实体
    std::cout << "  inline 函数的 static 局部变量全程序唯一\n";
}

// ---------- 6. assert 为什么必须是宏 ----------
// assert 依赖 __FILE__ / __LINE__ 以及 NDEBUG，函数无法做到：
//   - 函数拿不到调用点的文件名和行号
//   - 函数无法在 Release 版本里"完全消失"
#include <cassert>
void why_assert_is_macro() {
    int x = 1;
    assert(x == 1);      // 宏展开后能拿到本行的 __LINE__
    std::cout << "  assert 需要 __FILE__/__LINE__ 和 NDEBUG，只能是宏\n";
    std::cout << "  当前编译" << (
#ifdef NDEBUG
        "定义了 NDEBUG，assert 被完全移除"
#else
        "未定义 NDEBUG，assert 生效"
#endif
    ) << "\n";
}

int main() {
    std::cout << "--- 1. 宏的问题 ---\n";          macro_problems();
    std::cout << "--- 2. 访问私有成员 ---\n";      class_access();
    std::cout << "--- 3. 正确的 inline 写法 ---\n"; bar(1, 2);
    std::cout << "--- 4. inline 只是建议 ---\n";   inline_is_a_hint();
    std::cout << "--- 5. inline 的链接语义 ---\n"; linkage_semantics();
    std::cout << "--- 6. assert 为何是宏 ---\n";   why_assert_is_macro();
}
