// std=c++17
// 对应章节: blog/part6_modern_core/06-01_lambda.md
// 主题: lambda 的闭包本质、捕获方式、泛型 lambda、初始化捕获、constexpr lambda

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

// ---------- 1. 没有 lambda 时：函数对象的样板代码 ----------
namespace before_lambda {
    // C++98：要用 STL 算法做自定义比较，必须写一个类
    class GreaterThan {
    public:
        explicit GreaterThan(int threshold) : _threshold(threshold) {}
        bool operator()(int value) const { return value > _threshold; }
    private:
        int _threshold;
    };

    void demo() {
        std::vector<int> v{1, 5, 3, 8, 2, 9};

        // C++98 写法
        int count98 = static_cast<int>(
            std::count_if(v.begin(), v.end(), GreaterThan(3)));

        // C++11 写法
        int threshold = 3;
        int count11 = static_cast<int>(
            std::count_if(v.begin(), v.end(),
                          [threshold](int x) { return x > threshold; }));

        std::cout << "      C++98（需要一个类）: " << count98 << "\n";
        std::cout << "      C++11（就地写）:     " << count11 << "\n";
        std::cout << "      关键差别：逻辑写在使用的地方，不用跳到别处看一个类\n";
    }
}

// ---------- 2. lambda 的完整语法 ----------
void full_syntax() {
    // [捕获列表](参数列表) 修饰符 -> 返回类型 { 函数体 }
    auto f1 = [] { return 42; };                              // 最简形式
    auto f2 = [](int x) { return x * 2; };                    // 带参数
    auto f3 = [](int x) -> double { return x / 2.0; };        // 显式返回类型
    auto f4 = []() noexcept { return 1; };                    // noexcept
    auto f5 = [](int x) mutable { return ++x; };              // mutable

    std::cout << "      f1() = " << f1() << "\n";
    std::cout << "      f2(21) = " << f2(21) << "\n";
    std::cout << "      f3(5) = " << f3(5) << "\n";
    std::cout << "      f4() = " << f4() << " f5(1) = " << f5(1) << "\n";
    std::cout << "      语法: [捕获](参数) 修饰符 -> 返回类型 { 体 }\n";
}

// ---------- 3. lambda 的本质：编译器生成的类 ----------
void closure_essence() {
    int x = 10;
    std::string s = "hello";

    auto lam = [x, s](int y) { return x + y + static_cast<int>(s.size()); };

    // 编译器大致生成这样一个类：
    class EquivalentClass {
    public:
        EquivalentClass(int x, std::string s) : _x(x), _s(std::move(s)) {}
        int operator()(int y) const { return _x + y + static_cast<int>(_s.size()); }
    private:
        int _x;
        std::string _s;
    };
    EquivalentClass eq(x, s);

    std::cout << "      lambda 结果:      " << lam(5) << "\n";
    std::cout << "      等价类的结果:     " << eq(5) << "\n";
    // 注意：不能写 sizeof([]{...})，在 C++17 下 lambda 不能出现在未求值上下文:
    //   error: lambda-expression in unevaluated context only available with '-std=c++20'
    // 所以先给它起个名字
    auto empty_lam = [] { return 1; };
    std::cout << "      sizeof(lambda)  = " << sizeof(lam)
              << "（捕获的 int + string）\n";
    std::cout << "      sizeof(无捕获)  = " << sizeof(empty_lam)
              << "（空类，1 字节）\n";
    std::cout << "      每个 lambda 都是【独立的、匿名的类型】:\n";
    auto a = [] {};
    auto b = [] {};
    std::cout << "        两个相同的空 lambda 类型相同吗: "
              << std::is_same_v<decltype(a), decltype(b)> << "\n";
}

// ---------- 4. 捕获方式 ----------
void capture_modes() {
    int a = 1, b = 2, c = 3;

    auto by_value = [a, b] { return a + b; };            // 按值捕获
    auto by_ref   = [&a, &b] { a = 100; return a + b; }; // 按引用捕获
    auto all_value = [=] { return a + b + c; };          // 全部按值
    auto all_ref   = [&] { a += 1; return a + b + c; };  // 全部按引用
    auto mixed     = [=, &a] { a += 1; return a + b; };  // 混合：默认按值，a 按引用
    auto mixed2    = [&, b] { a += 1; return a + b; };   // 混合：默认按引用，b 按值

    std::cout << "      按值捕获: " << by_value() << "\n";
    std::cout << "      按引用捕获: " << by_ref() << "（a 被改成 " << a << "）\n";
    a = 1;
    std::cout << "      全部按值 [=]: " << all_value() << "\n";
    std::cout << "      全部按引用 [&]: " << all_ref() << "（a 变成 " << a << "）\n";
    a = 1;
    std::cout << "      混合 [=,&a]: " << mixed() << "（a 变成 " << a << "）\n";
    a = 1;
    std::cout << "      混合 [&,b]: " << mixed2() << "（a 变成 " << a << "）\n";
}

// ---------- 5. 按值捕获是在【创建 lambda 时】拷贝 ----------
void capture_timing() {
    int x = 1;
    auto by_value = [x] { return x; };      // 这一刻拷贝了 x=1
    auto by_ref   = [&x] { return x; };

    x = 100;                                 // 之后修改 x

    std::cout << "      修改 x 为 100 之后:\n";
    std::cout << "        按值捕获的 lambda 返回: " << by_value() << "（还是创建时的值）\n";
    std::cout << "        按引用捕获的 lambda 返回: " << by_ref() << "（跟着变了）\n";
}

// ---------- 6. mutable：修改按值捕获的副本 ----------
void mutable_demo() {
    int counter = 0;

    // 不加 mutable 时，operator() 是 const 的，不能改捕获的副本
    // auto bad = [counter] { return ++counter; };     // 编译错误

    auto gen = [counter]() mutable { return ++counter; };   // 有了自己的状态
    std::cout << "      调用三次: " << gen() << " " << gen() << " " << gen() << "\n";
    std::cout << "      外部的 counter 仍然是: " << counter << "\n";
    std::cout << "      mutable 让 lambda 拥有可变的内部状态（像个小对象）\n";
}

// ---------- 7. 悬垂引用：捕获的最大陷阱 ----------
std::function<int()> make_dangling() {
    int local = 42;
    // return [&local] { return local; };      // 危险！local 在函数返回后销毁
    return [local] { return local; };           // 安全：按值捕获
}

void dangling_trap() {
    auto f = make_dangling();
    std::cout << "      按值捕获的 lambda 在函数外仍然安全: " << f() << "\n";
    std::cout << "      规则：lambda 的生命周期超出当前作用域时，必须按值捕获\n";
    std::cout << "      特别注意: [&] 会捕获所有用到的局部变量，很容易出这个问题\n";
    std::cout << "      [=] 捕获 this 时也有同样问题（捕获的是指针！）\n";
}

// ---------- 8. 捕获 this 的坑 ----------
class Widget {
public:
    explicit Widget(int v) : _value(v) {}

    // [=] 和 [this] 捕获的都是 this 【指针】，不是对象
    std::function<int()> get_bad() {
        return [this] { return _value; };      // 若 Widget 先销毁，这里就悬垂
    }

    // C++17: 按值捕获整个 *this
    std::function<int()> get_good() {
        return [*this] { return _value; };     // 拷贝整个对象，安全
    }

private:
    int _value;
};

void this_capture() {
    std::function<int()> bad, good;
    {
        Widget w(42);
        bad = w.get_bad();
        good = w.get_good();
        std::cout << "      对象存活时: bad=" << bad() << " good=" << good() << "\n";
    }
    // 此时 w 已销毁
    std::cout << "      对象销毁后: good=" << good() << "（安全，捕获了副本）\n";
    std::cout << "      bad() 此时是未定义行为（已注释，不执行）\n";
    std::cout << "      [C++17] [*this] 按值捕获整个对象\n";
}

// ---------- 9. 泛型 lambda [C++14] ----------
void generic_lambda() {
    // C++11：参数类型必须写死
    auto add_int = [](int a, int b) { return a + b; };

    // C++14：参数用 auto，相当于模板
    auto add_any = [](auto a, auto b) { return a + b; };

    std::cout << "      add_any(1, 2) = " << add_any(1, 2) << "\n";
    std::cout << "      add_any(1.5, 2.5) = " << add_any(1.5, 2.5) << "\n";
    std::cout << "      add_any(string, char*) = "
              << add_any(std::string("ab"), "cd") << "\n";

    // 配合完美转发
    auto forwarder = [](auto&& f, auto&&... args) {
        return std::forward<decltype(f)>(f)(std::forward<decltype(args)>(args)...);
    };
    std::cout << "      通用转发器: " << forwarder(add_int, 20, 22) << "\n";
    std::cout << "      泛型 lambda 等价于一个带模板 operator() 的类\n";
}

// ---------- 10. 初始化捕获 [C++14] ----------
void init_capture() {
    // 用途一：捕获移动语义的对象
    auto ptr = std::make_unique<int>(42);
    // auto bad = [ptr] { return *ptr; };        // 编译错误：unique_ptr 不可拷贝
    auto good = [p = std::move(ptr)] { return *p; };    // [C++14] 移动进 lambda
    std::cout << "      捕获 unique_ptr: " << good() << "\n";
    std::cout << "      原 ptr 是否为空: " << (ptr == nullptr) << "\n";

    // 用途二：捕获表达式的结果（而不是变量）
    int x = 5;
    auto f = [y = x * 2] { return y; };
    std::cout << "      捕获表达式结果 x*2: " << f() << "\n";

    // 用途三：给捕获的变量改名或改类型
    auto g = [&r = x] { return ++r; };      // 捕获引用但改名
    g();
    std::cout << "      通过引用捕获修改 x: " << x << "\n";
}

// ---------- 11. constexpr lambda [C++17] ----------
void constexpr_lambda() {
    // C++17 起 lambda 的 operator() 在满足条件时隐式是 constexpr
    auto square = [](int x) { return x * x; };
    static_assert(square(5) == 25, "编译期求值");

    // 也可以显式标记
    auto cube = [](int x) constexpr { return x * x * x; };
    static_assert(cube(3) == 27, "");

    constexpr int arr_size = square(3);
    int arr[arr_size]{};

    std::cout << "      square(5) 在编译期求值 = " << square(5) << "\n";
    std::cout << "      用作数组大小 square(3) = " << sizeof(arr) / sizeof(arr[0]) << "\n";
}

// ---------- 12. 实际用途：STL 算法 ----------
void with_algorithms() {
    std::vector<int> v{5, 2, 8, 1, 9, 3};

    std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
    std::cout << "      降序排序: ";
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";

    auto sum = std::accumulate(v.begin(), v.end(), 0);
    auto even_count = std::count_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    std::cout << "      和 = " << sum << ", 偶数个数 = " << even_count << "\n";

    // 累积状态的 lambda
    int running = 0;
    std::for_each(v.begin(), v.end(), [&running](int x) { running += x; });
    std::cout << "      for_each 累加 = " << running << "\n";

    // 立即调用的 lambda（IIFE）：用于复杂初始化
    const auto config = [] {
        std::vector<std::string> result;
        result.push_back("a");
        result.push_back("b");
        return result;
    }();      // 注意末尾的 ()
    std::cout << "      立即调用的 lambda 初始化 const 对象，size=" << config.size() << "\n";
}

int main() {
    std::cout << "--- 1. 没有 lambda 的时代 ---\n";    before_lambda::demo();
    std::cout << "--- 2. 完整语法 ---\n";              full_syntax();
    std::cout << "--- 3. 闭包的本质 ---\n";            closure_essence();
    std::cout << "--- 4. 捕获方式 ---\n";              capture_modes();
    std::cout << "--- 5. 捕获时机 ---\n";              capture_timing();
    std::cout << "--- 6. mutable ---\n";               mutable_demo();
    std::cout << "--- 7. 悬垂引用陷阱 ---\n";          dangling_trap();
    std::cout << "--- 8. 捕获 this ---\n";             this_capture();
    std::cout << "--- 9. 泛型 lambda [C++14] ---\n";   generic_lambda();
    std::cout << "--- 10. 初始化捕获 [C++14] ---\n";   init_capture();
    std::cout << "--- 11. constexpr lambda [C++17] ---\n"; constexpr_lambda();
    std::cout << "--- 12. 配合 STL 算法 ---\n";        with_algorithms();
}
