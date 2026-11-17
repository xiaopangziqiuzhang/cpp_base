// std=c++17
// 对应章节: blog/part5_template/05-06_template_engineering.md
// 主题: 两阶段查找、typename/template 消歧、显式实例化、编译期开销

#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

// ---------- 1. typename 消歧：嵌套依赖类型 ----------
namespace typename_disambiguation {
    template <class Container>
    void process_wrong(const Container& c) {
        // Container::value_type 是"依赖于模板参数的嵌套名字"，
        // 编译器在第一阶段不知道它是类型还是静态成员，默认当成【值】。
        // 所以下面这行如果不加 typename 会报错：
        //   error: need 'typename' before 'Container::value_type'
        //          because 'Container' is a dependent scope
        // Container::value_type x{};         // 编译错误
        (void)c;
    }

    template <class Container>
    void process_right(const Container& c) {
        typename Container::value_type sum{};       // 加 typename 明确说这是类型
        for (const auto& x : c) sum += x;
        std::cout << "        元素之和 = " << sum << "\n";
    }

    // 返回类型里也要加
    template <class Container>
    typename Container::value_type first_element(const Container& c) {
        return *c.begin();
    }

    void demo() {
        std::vector<int> v{1, 2, 3, 4};
        process_right(v);
        std::cout << "        first_element(v) = " << first_element(v) << "\n";
        std::cout << "      规则：依赖模板参数的嵌套【类型】名，前面必须加 typename\n";
        std::cout << "      （C++20 在很多位置放宽了这个要求）\n";
    }
}

// ---------- 2. template 消歧：嵌套的成员模板 ----------
namespace template_disambiguation {
    struct Holder {
        template <class T>
        T get() const { return T{42}; }

        template <class T>
        struct Nested { using type = T; };
    };

    template <class H>
    void use_wrong(const H& h) {
        // h.get<int>() 会被解析成 (h.get < int) > ()  —— 把 < 当成小于号！
        //   error: expected primary-expression before 'int'
        // auto v = h.get<int>();            // 编译错误
        (void)h;
    }

    template <class H>
    void use_right(const H& h) {
        auto v = h.template get<int>();      // 加 template 明确说 get 是模板
        std::cout << "        h.template get<int>() = " << v << "\n";

        // 嵌套类型模板也一样
        using T = typename H::template Nested<double>::type;
        std::cout << "        Nested<double>::type 是 double: "
                  << std::is_same_v<T, double> << "\n";
    }

    void demo() {
        Holder h;
        use_right(h);
        std::cout << "      规则：依赖模板参数的【成员模板】，前面必须加 template\n";
    }
}

// ---------- 3. 两阶段查找 ----------
namespace two_phase_lookup {
    void helper(int) { std::cout << "        全局 helper(int)\n"; }

    template <class T>
    void caller(T t) {
        helper(t);        // 非依赖名字在【定义点】查找；依赖名字（靠 ADL）在【实例化点】查找
    }

    // 这个在模板定义之后才声明，所以非 ADL 的查找找不到它
    void helper(double) { std::cout << "        全局 helper(double) —— 定义在模板之后\n"; }

    namespace other {
        struct Widget {};
        void helper(Widget) { std::cout << "        other::helper(Widget) —— 靠 ADL 找到\n"; }
    }

    void demo() {
        caller(1);                    // helper(int)：定义点就能找到
        caller(2.5);                  // 也走 helper(int)！因为 helper(double) 声明太晚
        caller(other::Widget{});      // 靠 ADL 找到 other::helper
        std::cout << "      两阶段查找:\n";
        std::cout << "        阶段一（定义时）：检查语法 + 查找【非依赖】名字\n";
        std::cout << "        阶段二（实例化时）：查找【依赖】名字（通过 ADL）\n";
        std::cout << "      所以 caller(2.5) 调的是 helper(int)，而不是后面声明的 helper(double)\n";
    }
}

// ---------- 4. 模板继承中必须用 this-> 或限定名 ----------
namespace dependent_base {
    template <class T>
    class Base {
    public:
        void base_method() const { std::cout << "        Base<T>::base_method()\n"; }
        int _base_value = 42;
    };

    template <class T>
    class Derived : public Base<T> {
    public:
        void f() const {
            // base_method();          // 编译错误：
            //   error: there are no arguments to 'base_method' that depend on
            //          a template parameter, so a declaration of 'base_method'
            //          must be available
            this->base_method();        // 正确写法一
            Base<T>::base_method();     // 正确写法二
            std::cout << "        this->_base_value = " << this->_base_value << "\n";
        }
    };

    void demo() {
        Derived<int> d;
        d.f();
        std::cout << "      原因：Base<T> 是依赖基类，编译器在第一阶段不去里面查找名字\n";
        std::cout << "      解法：用 this-> 或 Base<T>:: 限定，把它变成依赖名字\n";
    }
}

// ---------- 5. 显式实例化：控制代码膨胀与编译时间 ----------
namespace explicit_instantiation {
    template <class T>
    class Processor {
    public:
        T process(T input) const { return input * 2; }
    };

    // 显式实例化定义：强制在这里生成代码
    template class Processor<int>;
    template class Processor<double>;

    // 函数模板也可以
    template <class T>
    T triple(T x) { return x * 3; }

    template int triple<int>(int);        // 显式实例化
    template double triple<double>(double);

    void demo() {
        Processor<int> pi;
        std::cout << "        Processor<int>.process(21) = " << pi.process(21) << "\n";
        std::cout << "        triple(14) = " << triple(14) << "\n";
        std::cout << "      显式实例化的用途:\n";
        std::cout << "        1. 在一个 .cpp 里集中生成代码，头文件只放声明\n";
        std::cout << "           （配合 extern template 可以显著缩短编译时间）\n";
        std::cout << "        2. 限制模板只能用于特定类型\n";
        std::cout << "        3. 避免每个翻译单元重复实例化\n";
    }
}

// ---------- 6. 编译期开销的实测 ----------
namespace compile_cost {
    // 递归模板：实例化深度随 N 线性增长
    template <int N>
    struct Factorial { static constexpr long long value = N * Factorial<N - 1>::value; };

    template <>
    struct Factorial<0> { static constexpr long long value = 1; };

    // constexpr 函数：同样的计算，但不产生几十个类型
    constexpr long long factorial_fn(int n) {
        long long r = 1;
        for (int i = 2; i <= n; ++i) r *= i;
        return r;
    }

    void demo() {
        std::cout << "      模板递归: Factorial<20>::value = " << Factorial<20>::value << "\n";
        std::cout << "      constexpr: factorial_fn(20)    = " << factorial_fn(20) << "\n";
        static_assert(Factorial<20>::value == factorial_fn(20), "两者结果一致");

        std::cout << "      区别（实测数据见章节正文）:\n";
        std::cout << "        模板递归：每个 Factorial<N> 是一个独立类型，要占编译器内存\n";
        std::cout << "        constexpr：只有一个函数，编译器直接求值\n";
        std::cout << "      实测（60 条互不重叠的 300 层递归链 vs 等价的 constexpr 调用）:\n";
        std::cout << "        模板递归 0.19 s / 峰值内存 67568 KB\n";
        std::cout << "        constexpr 0.00 s / 峰值内存 14288 KB\n";
        std::cout << "      注意：如果多个实例化【共享同一条递归链】，编译器会 memoize，\n";
        std::cout << "            此时模板反而可能更快 —— 测量时要排除这个因素\n";
        std::cout << "      结论：C++14 之后，编译期计算优先用 constexpr 而非模板元编程\n";
    }
}

// ---------- 7. 减少模板代码膨胀的技巧 ----------
namespace reduce_bloat {
    // 不好：整个类都是模板，每个 T 生成一份全部代码
    template <class T>
    class BadVector {
    public:
        void push_back(const T&) { /* 大量与 T 无关的簿记逻辑 */ }
        void reserve(std::size_t) { /* 同样与 T 无关 */ }
    };

    // 更好：把与类型无关的部分提到非模板基类
    class VectorBase {
    protected:
        void* _data = nullptr;
        std::size_t _size = 0;
        std::size_t _capacity = 0;
        void grow_impl(std::size_t new_cap, std::size_t elem_size) {
            // 与 T 无关的逻辑只有一份代码
            (void)new_cap; (void)elem_size;
        }
    };

    template <class T>
    class GoodVector : private VectorBase {
    public:
        void reserve(std::size_t n) { grow_impl(n, sizeof(T)); }   // 转调非模板实现
    };

    void demo() {
        GoodVector<int> gi;
        GoodVector<double> gd;
        gi.reserve(10);
        gd.reserve(10);
        std::cout << "      技巧：把与模板参数无关的逻辑提到非模板基类或非模板函数\n";
        std::cout << "      效果：GoodVector<int> 和 GoodVector<double> 共享 grow_impl 的代码\n";
        std::cout << "      标准库的 std::vector 实现就用了类似手法\n";
    }
}

int main() {
    std::cout << "--- 1. typename 消歧 ---\n";       typename_disambiguation::demo();
    std::cout << "--- 2. template 消歧 ---\n";       template_disambiguation::demo();
    std::cout << "--- 3. 两阶段查找 ---\n";          two_phase_lookup::demo();
    std::cout << "--- 4. 依赖基类 ---\n";            dependent_base::demo();
    std::cout << "--- 5. 显式实例化 ---\n";          explicit_instantiation::demo();
    std::cout << "--- 6. 编译期开销 ---\n";          compile_cost::demo();
    std::cout << "--- 7. 减少代码膨胀 ---\n";        reduce_bloat::demo();
}
