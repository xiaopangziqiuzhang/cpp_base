// std=c++17
// 对应章节: blog/part6_modern_core/06-03_using.md
// 主题: typedef vs using、别名模板、_t/_v 惯例

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

// ---------- 1. typedef 与 using 的基本等价 ----------
namespace basic_equivalence {
    typedef unsigned long ulong_old;
    using ulong_new = unsigned long;

    typedef std::vector<std::string> StringVec_old;
    using StringVec_new = std::vector<std::string>;

    void demo() {
        std::cout << "      两者完全等价: "
                  << std::is_same_v<ulong_old, ulong_new> << "\n";
        std::cout << "      std::is_same_v<StringVec_old, StringVec_new> = "
                  << std::is_same_v<StringVec_old, StringVec_new> << "\n";
        std::cout << "      using 的写法方向更自然：【新名字 = 旧类型】\n";
    }
}

// ---------- 2. 函数指针：using 的可读性优势明显 ----------
namespace function_pointers {
    // typedef 的语法：名字埋在中间
    typedef int (*FuncPtr_old)(double, const std::string&);

    // using 的语法：名字在最左边
    using FuncPtr_new = int (*)(double, const std::string&);

    // 更复杂的例子：返回函数指针的函数指针
    typedef int (*(*Complex_old)(char))(double);
    using Complex_new = int (*(*)(char))(double);

    int sample(double d, const std::string& s) {
        return static_cast<int>(d) + static_cast<int>(s.size());
    }

    void demo() {
        FuncPtr_new fp = sample;
        std::cout << "      通过别名调用函数指针: " << fp(3.9, "ab") << "\n";
        std::cout << "      typedef: typedef int (*FuncPtr)(double, const string&);\n";
        std::cout << "      using:   using FuncPtr = int (*)(double, const string&);\n";
        std::cout << "      函数指针、数组、成员指针这些场合，using 的可读性优势很明显\n";
        std::cout << "      两者仍然等价: " << std::is_same_v<FuncPtr_old, FuncPtr_new> << "\n";
    }
}

// ---------- 3. using 的决定性优势：别名模板 ----------
namespace alias_templates {
    // typedef 不能有模板参数！只能用一个结构体包一层
    template <class T>
    struct MyAlloc_old {
        typedef std::vector<T, std::allocator<T>> type;      // 要写 ::type
    };

    // using 可以直接模板化
    template <class T>
    using MyAlloc_new = std::vector<T, std::allocator<T>>;    // 直接用

    // 实用例子
    template <class K, class V>
    using Dict = std::map<K, V, std::less<K>>;

    template <class T>
    using Ptr = std::shared_ptr<T>;

    template <class T>
    using Callback = std::function<void(const T&)>;

    void demo() {
        // 旧写法：每次都要写 typename ...::type
        typename MyAlloc_old<int>::type v_old{1, 2, 3};
        // 新写法
        MyAlloc_new<int> v_new{1, 2, 3};

        std::cout << "      旧写法: typename MyAlloc_old<int>::type（要 typename + ::type）\n";
        std::cout << "      新写法: MyAlloc_new<int>\n";
        std::cout << "      两者类型相同: "
                  << std::is_same_v<typename MyAlloc_old<int>::type, MyAlloc_new<int>> << "\n";

        Dict<std::string, int> scores{{"a", 1}, {"b", 2}};
        Ptr<int> p = std::make_shared<int>(42);
        Callback<int> cb = [](const int& x) { std::cout << "        收到 " << x << "\n"; };

        std::cout << "      Dict<string,int> size = " << scores.size() << "\n";
        std::cout << "      Ptr<int> 指向 " << *p << "\n";
        std::cout << "      Callback<int>: "; cb(99);
        std::cout << "      sizeof(v_old)=" << sizeof(v_old)
                  << " sizeof(v_new)=" << sizeof(v_new) << "\n";
    }
}

// ---------- 4. 为什么别名模板不需要 typename ----------
namespace no_typename_needed {
    template <class T>
    struct Wrapper_old { typedef T* type; };

    template <class T>
    using Wrapper_new = T*;

    // 在模板里使用
    template <class T>
    void use_old() {
        typename Wrapper_old<T>::type p = nullptr;    // 必须写 typename（见 05-06）
        (void)p;
    }

    template <class T>
    void use_new() {
        Wrapper_new<T> p = nullptr;                    // 不需要 typename！
        (void)p;
    }

    void demo() {
        use_old<int>();
        use_new<int>();
        std::cout << "      别名模板【保证】是类型，所以不需要 typename 消歧\n";
        std::cout << "      而 struct 里的 ::type 可能被特化成别的东西，所以必须写 typename\n";
        std::cout << "      这正是标准库提供 _t 后缀的原因\n";
    }
}

// ---------- 5. 标准库的 _t / _v 惯例 ----------
namespace std_conventions {
    void demo() {
        std::cout << "      C++11 原始形式（struct + ::type / ::value）:\n";
        std::cout << "        typename std::remove_reference<int&>::type\n";
        std::cout << "        std::is_same<int, int>::value\n";

        std::cout << "      C++14 的 _t 别名模板:\n";
        std::cout << "        std::remove_reference_t<int&>  -> 不需要 typename\n";
        std::cout << "        等价性: "
                  << std::is_same_v<typename std::remove_reference<int&>::type,
                                    std::remove_reference_t<int&>> << "\n";

        std::cout << "      C++17 的 _v 变量模板:\n";
        std::cout << "        std::is_same_v<int,int> = " << std::is_same_v<int, int> << "\n";
        std::cout << "        等价性: "
                  << (std::is_same<int, int>::value == std::is_same_v<int, int>) << "\n";

        // 对比一段真实代码的可读性
        std::cout << "      可读性对比（同一个条件）:\n";
        std::cout << "        C++11: typename std::enable_if<std::is_integral<T>::value, T>::type\n";
        std::cout << "        C++14: std::enable_if_t<std::is_integral<T>::value, T>\n";
        std::cout << "        C++17: std::enable_if_t<std::is_integral_v<T>, T>\n";
    }
}

// ---------- 6. 自己定义 _t / _v 后缀 ----------
namespace own_conventions {
    // 一个自定义的 trait
    template <class T>
    struct is_string_like {
        static constexpr bool value =
            std::is_same_v<T, std::string> || std::is_same_v<T, const char*>;
    };

    // 提供 _v 版本
    template <class T>
    constexpr bool is_string_like_v = is_string_like<T>::value;

    // 一个类型变换 trait
    template <class T>
    struct add_const_ref { using type = const T&; };

    // 提供 _t 版本
    template <class T>
    using add_const_ref_t = typename add_const_ref<T>::type;

    void demo() {
        std::cout << "      is_string_like_v<std::string> = "
                  << is_string_like_v<std::string> << "\n";
        std::cout << "      is_string_like_v<int> = " << is_string_like_v<int> << "\n";
        std::cout << "      add_const_ref_t<int> 是 const int& 吗: "
                  << std::is_same_v<add_const_ref_t<int>, const int&> << "\n";
        std::cout << "      自定义 trait 时也应该提供 _t / _v 版本，保持一致的风格\n";
    }
}

// ---------- 7. using 的其他用途（与类型别名无关）----------
namespace other_uses {
    namespace deep { namespace nested { void func() { std::cout << "        deep::nested::func()\n"; } } }

    class Base {
    public:
        void f(int) { std::cout << "        Base::f(int)\n"; }
        void f(double) { std::cout << "        Base::f(double)\n"; }
    protected:
        int _value = 42;
    };

    class Derived : public Base {
    public:
        using Base::f;          // 引入基类的全部重载（见 04-01）
        using Base::_value;     // 改变访问权限
        void f(const char*) { std::cout << "        Derived::f(const char*)\n"; }
    };

    class WithInheritedCtor : public std::string {
    public:
        using std::string::string;    // 继承构造函数（见 03-02）
    };

    void demo() {
        std::cout << "      用途一：using 声明引入名字\n";
        using deep::nested::func;
        func();

        std::cout << "      用途二：引入基类重载 + 改访问权限\n";
        Derived d;
        d.f(1); d.f(2.5); d.f("text");
        std::cout << "        d._value = " << d._value << "（原本是 protected）\n";

        std::cout << "      用途三：继承构造函数\n";
        WithInheritedCtor s("hello");
        std::cout << "        用 std::string 的构造函数: " << s << "\n";

        std::cout << "      所以 using 有四种用途：类型别名、别名模板、using 声明、继承构造\n";
    }
}

int main() {
    std::cout << "--- 1. 基本等价 ---\n";              basic_equivalence::demo();
    std::cout << "--- 2. 函数指针 ---\n";              function_pointers::demo();
    std::cout << "--- 3. 别名模板 ---\n";              alias_templates::demo();
    std::cout << "--- 4. 不需要 typename ---\n";       no_typename_needed::demo();
    std::cout << "--- 5. 标准库的 _t/_v ---\n";        std_conventions::demo();
    std::cout << "--- 6. 自定义 _t/_v ---\n";          own_conventions::demo();
    std::cout << "--- 7. using 的其他用途 ---\n";      other_uses::demo();
}
