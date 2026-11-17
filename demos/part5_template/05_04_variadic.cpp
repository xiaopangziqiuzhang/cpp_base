// std=c++17
// 对应章节: blog/part5_template/05-04_variadic.md
// 主题: 可变模板参数、参数包展开的四种方式、折叠表达式

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// ---------- 1. 可变参数的历史方案与问题 ----------
#include <cstdarg>
namespace old_way {
    // C 的 varargs：没有类型信息，必须手工传个数或哨兵
    int sum_c(int count, ...) {
        va_list args;
        va_start(args, count);
        int total = 0;
        for (int i = 0; i < count; ++i) total += va_arg(args, int);
        va_end(args);
        return total;
    }

    void demo() {
        std::cout << "      C 的 varargs: sum_c(3, 1,2,3) = " << sum_c(3, 1, 2, 3) << "\n";
        std::cout << "      问题: 1) 无类型检查 2) 要手工传个数 3) 不能传类对象\n";
        std::cout << "             4) printf(\"%d\", 1.5) 这类错误只能靠编译器特判\n";
    }
}

// ---------- 2. 参数包的声明与 sizeof... ----------
template <class... Args>              // Args 是模板参数包
void count_args(Args... args) {        // args 是函数参数包
    std::cout << "        sizeof...(Args) = " << sizeof...(Args)
              << ", sizeof...(args) = " << sizeof...(args) << "\n";
}

void parameter_pack() {
    count_args();
    count_args(3.14159);
    count_args('a', "hello");
    count_args(1, 1.2, "hello,world", std::string("str"));
    std::cout << "      声明参数包：省略号在名字【左边】 class... Args\n";
    std::cout << "      使用参数包：省略号在名字【右边】 args...（表示展开）\n";
}

// ---------- 3. 展开方式一：递归 ----------
namespace recursive {
    // 递归终止函数（必须有）
    void print() { std::cout << "\n"; }

    // 递归展开：取出第一个，剩下的递归处理
    template <class T, class... Args>
    void print(T first, Args... rest) {
        std::cout << first;
        if (sizeof...(rest) > 0) std::cout << ", ";
        print(rest...);          // 展开剩余参数包
    }

    void demo() {
        std::cout << "        ";  print(1, 2.5, "three", 'x');
        std::cout << "        ";  print(42);
        std::cout << "        ";  print();
        std::cout << "      优点：直观，是标准做法\n";
        std::cout << "      缺点：需要一个重载的递归终止函数\n";
    }
}

// ---------- 4. 展开方式二：逗号表达式 + 初始化列表 ----------
namespace comma_trick {
    template <class T>
    void print_one(const T& t) { std::cout << t << " "; }

    template <class... Args>
    void print_all(Args... args) {
        // (print_one(args), 0) 的值是 0，但副作用是打印
        // 展开成 {(print_one(a1),0), (print_one(a2),0), ...}
        int dummy[] = {(print_one(args), 0)...};
        (void)dummy;
        std::cout << "\n";
    }

    // 参数包为空时上面的数组会是零长度（非法），所以要补一个元素
    template <class... Args>
    void print_all_safe(Args... args) {
        int dummy[] = {0, (print_one(args), 0)...};
        (void)dummy;
        std::cout << "\n";
    }

    void demo() {
        std::cout << "        ";  print_all(1, 'a', 12.12, "hello");
        std::cout << "        ";  print_all_safe();
        std::cout << "      不需要递归终止函数，但要用逗号表达式这个技巧\n";
        std::cout << "      注意空包时数组长度为 0 的问题，要补一个 0\n";
    }
}

// ---------- 5. 展开方式三：折叠表达式 [C++17] ----------
namespace fold {
    // 一元右折叠: (args op ...)  ->  a1 op (a2 op (a3 op a4))
    template <class... Args>
    auto sum_right(Args... args) { return (args + ...); }

    // 一元左折叠: (... op args)  ->  ((a1 op a2) op a3) op a4
    template <class... Args>
    auto sum_left(Args... args) { return (... + args); }

    // 二元折叠（带初值），可以处理空包
    template <class... Args>
    auto sum_with_init(Args... args) { return (0 + ... + args); }

    // 用逗号运算符折叠来做"对每个参数执行某操作"
    template <class... Args>
    void print_all(Args... args) {
        ((std::cout << args << " "), ...);     // 逗号折叠
        std::cout << "\n";
    }

    // 逻辑运算的折叠
    template <class... Args>
    bool all_true(Args... args) { return (args && ...); }

    template <class... Args>
    bool any_true(Args... args) { return (args || ...); }

    // 实用例子：把所有参数拼成字符串
    template <class... Args>
    std::string concat(const Args&... args) {
        std::ostringstream oss;
        (oss << ... << args);                   // 二元左折叠
        return oss.str();
    }

    void demo() {
        std::cout << "      sum_right(1,2,3,4) = " << sum_right(1, 2, 3, 4) << "\n";
        std::cout << "      sum_left(1,2,3,4)  = " << sum_left(1, 2, 3, 4) << "\n";
        std::cout << "      sum_with_init()    = " << sum_with_init()
                  << "（空包也能工作）\n";
        std::cout << "      print_all: ";  print_all(1, 'a', 2.5, "str");
        std::cout << "      all_true(true,true,false) = " << all_true(true, true, false) << "\n";
        std::cout << "      any_true(false,false,true) = " << any_true(false, false, true) << "\n";
        std::cout << "      concat(\"a\", 1, \"-\", 2.5) = " << concat("a", 1, "-", 2.5) << "\n";
        std::cout << "      一行代码替代了递归 + 终止函数\n";
    }
}

// ---------- 6. 包扩展表达式的各种形式 ----------
namespace expansion_forms {
    template <class T>
    T square(T x) { return x * x; }

    template <class T>
    void show(const T& t) { std::cout << t << " "; }

    template <class... Args>
    void forms(Args... args) {
        // 形式一：f(args...)  —— 整个包作为实参列表
        std::cout << "        f(args...)        -> 参数个数 " << sizeof...(args) << "\n";

        // 形式二：f(args)...  —— 对每个参数分别调用
        std::cout << "        f(args)...        -> ";
        ((show(args)), ...);
        std::cout << "\n";

        // 形式三：对包里每个元素做变换后再传递
        std::cout << "        f(square(args))...-> ";
        ((show(square(args))), ...);
        std::cout << "\n";
    }

    void demo() {
        forms(1, 2, 3);
        std::cout << "      包扩展 exp... 相当于把省略号左侧的表达式整体展开\n";
    }
}

// ---------- 7. 完美转发 + 可变参数：现代 C++ 的标准组合 ----------
namespace forwarding {
    class Widget {
    public:
        Widget() { std::cout << "        Widget()\n"; }
        Widget(int a) { std::cout << "        Widget(int " << a << ")\n"; }
        Widget(int a, const std::string& s) {
            std::cout << "        Widget(int " << a << ", string " << s << ")\n";
        }
    };

    // 这就是 make_unique 的实现方式
    template <class T, class... Args>
    std::unique_ptr<T> my_make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    // 通用的"调用任意函数并计时"的包装器
    template <class F, class... Args>
    auto invoke_and_report(F&& f, Args&&... args) {
        std::cout << "        调用一个带 " << sizeof...(args) << " 个参数的函数\n";
        return std::forward<F>(f)(std::forward<Args>(args)...);
    }

    void demo() {
        auto p1 = my_make_unique<Widget>();
        auto p2 = my_make_unique<Widget>(42);
        auto p3 = my_make_unique<Widget>(42, "hello");

        int r = invoke_and_report([](int a, int b) { return a + b; }, 3, 4);
        std::cout << "        结果 = " << r << "\n";
        std::cout << "      Args&&... + std::forward<Args>(args)... 是固定搭配\n";
    }
}

// ---------- 8. 可变参数在类模板中：tuple 的原理 ----------
namespace variadic_class {
    // 简化版 tuple：靠递归继承存储任意个成员
    template <class... Types>
    class MiniTuple;

    template <>
    class MiniTuple<> {};                       // 递归终止：空 tuple

    template <class Head, class... Tail>
    class MiniTuple<Head, Tail...> : private MiniTuple<Tail...> {
    public:
        MiniTuple(Head h, Tail... t) : MiniTuple<Tail...>(t...), _head(std::move(h)) {}
        Head& head() { return _head; }
        MiniTuple<Tail...>& tail() { return *this; }
    private:
        Head _head;
    };

    void demo() {
        MiniTuple<int, double, std::string> t(1, 2.5, "three");
        std::cout << "        head = " << t.head() << "\n";
        std::cout << "        tail().head() = " << t.tail().head() << "\n";
        std::cout << "        tail().tail().head() = " << t.tail().tail().head() << "\n";
        std::cout << "      std::tuple 就是这个思路（实现更复杂，用的是多重继承）\n";

        // 标准库的 tuple
        std::tuple<int, double, std::string> st(1, 2.5, "three");
        std::cout << "        std::get<2>(st) = " << std::get<2>(st) << "\n";
    }
}

int main() {
    std::cout << "--- 1. 历史方案 ---\n";              old_way::demo();
    std::cout << "--- 2. 参数包与 sizeof... ---\n";    parameter_pack();
    std::cout << "--- 3. 递归展开 ---\n";              recursive::demo();
    std::cout << "--- 4. 逗号表达式展开 ---\n";        comma_trick::demo();
    std::cout << "--- 5. 折叠表达式 [C++17] ---\n";    fold::demo();
    std::cout << "--- 6. 包扩展形式 ---\n";            expansion_forms::demo();
    std::cout << "--- 7. 完美转发组合 ---\n";          forwarding::demo();
    std::cout << "--- 8. 可变参数类模板 ---\n";        variadic_class::demo();
}
