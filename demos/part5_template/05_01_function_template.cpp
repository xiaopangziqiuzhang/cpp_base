// std=c++17
// 对应章节: blog/part5_template/05-01_function_template.md
// 主题: 函数模板、实例化、推导规则、与重载的关系、非类型参数

#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

// ---------- 1. 模板解决的问题 ----------
namespace motivation {
    // 做法一：为每种类型写一个函数
    int    add_int(int a, int b)       { return a + b; }
    double add_double(double a, double b) { return a + b; }

    // 做法二：重载（好一些，但实现还是重复的）
    int    add(int a, int b)       { return a + b; }
    double add(double a, double b) { return a + b; }

    // 做法三：带参数的宏（无类型检查，有副作用）
    #define ADD(a, b) ((a) + (b))

    // 做法四：模板 —— 类型参数化
    template <class T>
    T add_template(T a, T b) { return a + b; }

    void demo() {
        std::cout << "      重载: add(1,2)=" << add(1, 2)
                  << " add(1.5,2.5)=" << add(1.5, 2.5) << "\n";
        std::cout << "      模板: add_template(1,2)=" << add_template(1, 2)
                  << " add_template(1.5,2.5)=" << add_template(1.5, 2.5) << "\n";
        std::cout << "      模板还能用于当初没想到的类型:\n";
        std::cout << "        string: " << add_template(std::string("a"), std::string("b")) << "\n";
    }
}

// ---------- 2. 模板的定义形式与两个关键字 ----------
// template <class T> 和 template <typename T> 完全等价
template <class T>
T max_by_class(T a, T b) { return a > b ? a : b; }

template <typename T>
T max_by_typename(T a, T b) { return a > b ? a : b; }

void keywords() {
    std::cout << "      class 和 typename 在模板参数列表中等价\n";
    std::cout << "      max_by_class(3,7)=" << max_by_class(3, 7)
              << " max_by_typename(3.5,1.5)=" << max_by_typename(3.5, 1.5) << "\n";
    std::cout << "      历史原因：早期编译器只支持 class；typename 更能体现\"类型\"的含义\n";
}

// ---------- 3. 隐式实例化 vs 显式实例化 ----------
template <class T>
void show_type(T /*v*/) {
    std::cout << "        T 是 int 吗: " << std::is_same<T, int>::value
              << ", 是 double 吗: " << std::is_same<T, double>::value << "\n";
}

void instantiation() {
    int i = 1;
    double d = 1.5;

    std::cout << "      隐式实例化（从实参推导）:\n";
    show_type(i);              // T = int
    show_type(d);              // T = double

    std::cout << "      显式实例化（指定模板实参）:\n";
    show_type<double>(i);      // T = double，i 被转换成 double
    show_type<int>(d);         // T = int，d 被截断

    std::cout << "      显式指定可以绕过推导，也可以处理无法推导的情况\n";
}

// ---------- 4. 推导的限制：多个参数必须一致 ----------
template <class T>
T strict_add(T a, T b) { return a + b; }

template <class T1, class T2>
auto flexible_add(T1 a, T2 b) { return a + b; }    // [C++14] 返回类型推导

void deduction_limits() {
    // strict_add(1, 2.5);          // 编译错误：T 既要是 int 又要是 double
    std::cout << "      strict_add(1, 2.5) 编译错误：deduced conflicting types for 'T'\n";
    std::cout << "      三种解法:\n";
    std::cout << "        1. 显式指定: strict_add<double>(1, 2.5) = "
              << strict_add<double>(1, 2.5) << "\n";
    std::cout << "        2. 用两个模板参数: flexible_add(1, 2.5) = "
              << flexible_add(1, 2.5) << "\n";
    std::cout << "        3. 手工转换: strict_add(1.0, 2.5) = "
              << strict_add(1.0, 2.5) << "\n";
}

// ---------- 5. 推导不做隐式转换 ----------
template <class T>
void takes_vector(const std::vector<T>& v) {
    std::cout << "        收到 vector，size=" << v.size() << "\n";
}

void no_implicit_conversion() {
    std::vector<int> v{1, 2, 3};
    takes_vector(v);           // ok

    // takes_vector({1, 2, 3});   // 编译错误：无法从 initializer_list 推导 T
    takes_vector<int>({1, 2, 3});  // 显式指定后就可以了
    std::cout << "      模板实参推导【不做】隐式转换，只做精确匹配（外加少量调整）\n";
}

// ---------- 6. 模板与普通函数的重载关系 ----------
void which_one(int)    { std::cout << "        普通函数 which_one(int)\n"; }

template <class T>
void which_one(T)      { std::cout << "        模板 which_one(T)\n"; }

template <class T>
void which_one(T*)     { std::cout << "        模板 which_one(T*)（更特化）\n"; }

void overload_rules() {
    int i = 1;
    double d = 1.5;
    int* p = &i;

    std::cout << "      传 int:\n";    which_one(i);     // 普通函数优先
    std::cout << "      传 double:\n"; which_one(d);     // 只有模板能精确匹配
    std::cout << "      传 int*:\n";   which_one(p);     // T* 版本更特化
    std::cout << "      规则：精确匹配的普通函数 > 更特化的模板 > 普通模板\n";
    std::cout << "      强制调用模板版本: "; which_one<>(i);
}

// ---------- 7. 非类型模板参数 ----------
template <class T, int kMultiplier>
T multiply(T x) { return x * kMultiplier; }

// 非类型参数可以有默认值
template <class T, std::size_t kSize = 8>
class FixedArray {
public:
    static constexpr std::size_t size() { return kSize; }
    T& operator[](std::size_t i) { return _data[i]; }
private:
    T _data[kSize]{};
};

void non_type_params() {
    std::cout << "      multiply<int, 10>(5) = " << multiply<int, 10>(5) << "\n";
    std::cout << "      multiply<double, 3>(1.5) = " << multiply<double, 3>(1.5) << "\n";

    FixedArray<int> a;           // 用默认大小 8
    FixedArray<int, 16> b;
    std::cout << "      FixedArray<int>::size() = " << a.size()
              << " FixedArray<int,16>::size() = " << b.size() << "\n";

    // 非类型参数必须是编译期常量
    // int n = 10;
    // multiply<int, n>(5);      // 编译错误：n 不是常量表达式
    constexpr int kN = 10;
    std::cout << "      multiply<int, kN>(5) = " << multiply<int, kN>(5)
              << "（必须是编译期常量）\n";
}

// ---------- 8. auto 非类型参数 [C++17] ----------
template <auto kValue>
void print_value() {
    std::cout << "        值 = " << kValue
              << ", 类型是 int 吗: " << std::is_same<decltype(kValue), int>::value
              << ", 是 char 吗: " << std::is_same<decltype(kValue), char>::value << "\n";
}

void auto_non_type() {
    std::cout << "      C++17 的 auto 非类型参数，类型自动推导:\n";
    print_value<42>();          // int
    print_value<'x'>();         // char
    print_value<true>();        // bool

    std::cout << "      C++17 之前必须写 template<class T, T kValue>，调用时要写两个实参\n";
}

// ---------- 9. 模板参数的默认值 ----------
template <class T, class Compare = std::less<T>>
bool is_ordered(const T& a, const T& b, Compare cmp = Compare{}) {
    return cmp(a, b);
}

void default_template_args() {
    std::cout << "      默认用 less:    is_ordered(1, 2) = " << is_ordered(1, 2) << "\n";
    std::cout << "      指定 greater:   is_ordered<int, std::greater<int>>(1, 2) = "
              << is_ordered<int, std::greater<int>>(1, 2) << "\n";
}

int main() {
    std::cout << "--- 1. 模板解决的问题 ---\n";       motivation::demo();
    std::cout << "--- 2. class vs typename ---\n";    keywords();
    std::cout << "--- 3. 实例化 ---\n";               instantiation();
    std::cout << "--- 4. 推导的限制 ---\n";           deduction_limits();
    std::cout << "--- 5. 不做隐式转换 ---\n";         no_implicit_conversion();
    std::cout << "--- 6. 与重载的关系 ---\n";         overload_rules();
    std::cout << "--- 7. 非类型参数 ---\n";           non_type_params();
    std::cout << "--- 8. auto 非类型参数 [C++17] ---\n"; auto_non_type();
    std::cout << "--- 9. 默认模板参数 ---\n";         default_template_args();
}
