// std=c++17
// 对应章节: blog/part5_template/05-03_specialization.md
// 主题: 全特化、偏特化、特化的常见坑

#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

// ---------- 1. 为什么需要特化：通用版本对某些类型不对 ----------
namespace why_specialize {
    // 通用版本
    template <class T>
    T add(T a, T b) { return a + b; }

    // 对 const char* 来说，a + b 是指针相加 —— 根本编不过/没意义
    // 解法一：重载
    const char* add(const char* a, const char* b) {
        char* tmp = new char[std::strlen(a) + std::strlen(b) + 1]();
        std::strcpy(tmp, a);
        std::strcat(tmp, b);
        return tmp;
    }

    void demo() {
        std::cout << "      通用版本: add(1,2) = " << add(1, 2) << "\n";
        const char* r = add("hello,", "world");
        std::cout << "      重载版本: add(\"hello,\",\"world\") = " << r << "\n";
        delete[] r;
    }
}

// ---------- 2. 函数模板的全特化 ----------
namespace full_specialization {
    // 基础模板（必须先有它）
    template <class T>
    void describe(T value) {
        std::cout << "        通用版本, 值 = " << value << "\n";
    }

    // 全特化：template<> + 函数名<特化类型>
    template <>
    void describe<bool>(bool value) {
        std::cout << "        bool 特化版本, 值 = " << (value ? "true" : "false") << "\n";
    }

    template <>
    void describe<const char*>(const char* value) {
        std::cout << "        const char* 特化版本, 长度 = " << std::strlen(value)
                  << ", 值 = " << value << "\n";
    }

    void demo() {
        describe(42);
        describe(3.14);
        describe(true);                        // 走 bool 特化
        describe<const char*>("hello");        // 走 const char* 特化
        std::cout << "      函数模板只能【全特化】，不能偏特化\n";
        std::cout << "      实践中通常用重载替代函数模板特化（原因见正文）\n";
    }
}

// ---------- 3. 类模板的全特化 ----------
namespace class_full_spec {
    // 通用版本
    template <class T>
    class TypeInfo {
    public:
        static std::string name() { return "unknown"; }
        static constexpr bool is_special = false;
    };

    // 全特化
    template <>
    class TypeInfo<int> {
    public:
        static std::string name() { return "int"; }
        static constexpr bool is_special = true;
        static constexpr int min_value = -2147483648;     // 特化可以加新成员
    };

    template <>
    class TypeInfo<std::string> {
    public:
        static std::string name() { return "std::string"; }
        static constexpr bool is_special = true;
    };

    void demo() {
        std::cout << "      TypeInfo<double>::name() = " << TypeInfo<double>::name() << "\n";
        std::cout << "      TypeInfo<int>::name()    = " << TypeInfo<int>::name()
                  << " min_value=" << TypeInfo<int>::min_value << "\n";
        std::cout << "      TypeInfo<string>::name() = " << TypeInfo<std::string>::name() << "\n";
        std::cout << "      特化版本可以有【完全不同的接口】（这里多了 min_value）\n";
    }
}

// ---------- 4. 类模板的偏特化 ----------
namespace partial_spec {
    // 通用版本
    template <class T1, class T2>
    class Pair {
    public:
        static std::string which() { return "通用: Pair<T1, T2>"; }
    };

    // 偏特化一：两个类型相同
    template <class T>
    class Pair<T, T> {
    public:
        static std::string which() { return "偏特化: Pair<T, T>（两类型相同）"; }
    };

    // 偏特化二：第二个是 int
    template <class T>
    class Pair<T, int> {
    public:
        static std::string which() { return "偏特化: Pair<T, int>"; }
    };

    // 偏特化三：都是指针
    template <class T1, class T2>
    class Pair<T1*, T2*> {
    public:
        static std::string which() { return "偏特化: Pair<T1*, T2*>（都是指针）"; }
    };

    // 全特化
    template <>
    class Pair<double, double> {
    public:
        static std::string which() { return "全特化: Pair<double, double>"; }
    };

    void demo() {
        std::cout << "      Pair<char, long>     -> " << Pair<char, long>::which() << "\n";
        std::cout << "      Pair<char, char>     -> " << Pair<char, char>::which() << "\n";
        std::cout << "      Pair<char, int>      -> " << Pair<char, int>::which() << "\n";
        std::cout << "      Pair<int*, char*>    -> " << Pair<int*, char*>::which() << "\n";
        std::cout << "      Pair<double, double> -> " << Pair<double, double>::which() << "\n";
        std::cout << "      规则：全特化 > 更特化的偏特化 > 通用版本\n";
    }
}

// ---------- 5. 偏特化的常见用途：萃取类型信息 ----------
namespace traits {
    // 判断是否是指针
    template <class T>
    struct is_pointer_impl { static constexpr bool value = false; };

    template <class T>
    struct is_pointer_impl<T*> { static constexpr bool value = true; };    // 偏特化

    // 去掉引用
    template <class T> struct remove_ref_impl       { using type = T; };
    template <class T> struct remove_ref_impl<T&>   { using type = T; };
    template <class T> struct remove_ref_impl<T&&>  { using type = T; };

    // 萃取容器的元素类型
    template <class C>
    struct element_type { using type = typename C::value_type; };

    template <class T, std::size_t N>
    struct element_type<T[N]> { using type = T; };      // 数组的偏特化

    void demo() {
        std::cout << "      自己实现的 is_pointer:\n";
        std::cout << "        is_pointer_impl<int>::value  = "
                  << is_pointer_impl<int>::value << "\n";
        std::cout << "        is_pointer_impl<int*>::value = "
                  << is_pointer_impl<int*>::value << "\n";

        std::cout << "      自己实现的 remove_reference:\n";
        std::cout << "        remove_ref_impl<int&>::type 是 int 吗: "
                  << std::is_same<remove_ref_impl<int&>::type, int>::value << "\n";
        std::cout << "        remove_ref_impl<int&&>::type 是 int 吗: "
                  << std::is_same<remove_ref_impl<int&&>::type, int>::value << "\n";

        std::cout << "      萃取元素类型:\n";
        std::cout << "        vector<double> 的元素类型是 double 吗: "
                  << std::is_same<element_type<std::vector<double>>::type, double>::value << "\n";
        std::cout << "        int[5] 的元素类型是 int 吗: "
                  << std::is_same<element_type<int[5]>::type, int>::value << "\n";
        std::cout << "      标准库的 type_traits 全是这么实现的\n";
    }
}

// ---------- 6. 坑一：函数模板"特化"不参与重载决议的常规排序 ----------
namespace pitfall_order {
    template <class T> void f(T)   { std::cout << "        f(T) 通用模板\n"; }
    template <class T> void f(T*)  { std::cout << "        f(T*) 另一个模板\n"; }
    template <> void f<int*>(int*) { std::cout << "        f<int*>(int*) 特化\n"; }
    // 注意：上面的特化是 f(T) 的特化还是 f(T*) 的特化？
    // 答案：是 f(T) 的特化（T = int*），因为特化在声明时先做重载决议选基础模板。
    // 而重载决议会选 f(T*)（更特化），所以传 int* 时走的是 f(T*)，
    // 那个 f<int*> 特化【永远不会被调用】！

    void demo() {
        int i = 1;
        int* p = &i;
        std::cout << "      f(p) 实际调用的是:\n";
        f(p);
        std::cout << "      ^ 走了 f(T*)，而不是我们写的 f<int*> 特化\n";
        std::cout << "      教训：函数模板优先用重载，不要用特化\n";
    }
}

// ---------- 7. 坑二：特化必须在使用之前声明 ----------
namespace pitfall_timing {
    template <class T>
    struct Widget { static std::string which() { return "通用"; } };

    // 如果在这里先实例化了 Widget<int>，后面的特化就是错误的
    // （编译器已经用通用版本生成了 Widget<int>）
    // void early_use() { Widget<int>::which(); }      // 过早使用

    template <>
    struct Widget<int> { static std::string which() { return "int 特化"; } };

    void demo() {
        std::cout << "      Widget<int>::which() = " << Widget<int>::which() << "\n";
        std::cout << "      特化必须在该类型【首次实例化之前】声明，\n";
        std::cout << "      否则编译器已经用通用版本实例化了，特化会报错或被忽略\n";
        std::cout << "      实践：把特化声明和基础模板放在同一个头文件里\n";
    }
}

// ---------- 8. 坑三：特化不继承通用版本的任何东西 ----------
namespace pitfall_no_inherit {
    template <class T>
    class Base {
    public:
        void common() { std::cout << "        通用版本的 common()\n"; }
        void also_common() { std::cout << "        通用版本的 also_common()\n"; }
    };

    template <>
    class Base<int> {
    public:
        void common() { std::cout << "        int 特化的 common()\n"; }
        // 没有 also_common！特化是一个全新的类
    };

    void demo() {
        Base<double> bd;
        bd.common();
        bd.also_common();

        Base<int> bi;
        bi.common();
        // bi.also_common();     // 编译错误：特化版本没有这个成员
        std::cout << "      特化是一个【全新的类】，不继承通用版本的任何成员\n";
        std::cout << "      想复用公共部分，要显式继承一个公共基类\n";
    }
}

int main() {
    std::cout << "--- 1. 为什么需要特化 ---\n";      why_specialize::demo();
    std::cout << "--- 2. 函数模板全特化 ---\n";      full_specialization::demo();
    std::cout << "--- 3. 类模板全特化 ---\n";        class_full_spec::demo();
    std::cout << "--- 4. 类模板偏特化 ---\n";        partial_spec::demo();
    std::cout << "--- 5. 偏特化做类型萃取 ---\n";    traits::demo();
    std::cout << "--- 6. 坑一：特化与重载 ---\n";    pitfall_order::demo();
    std::cout << "--- 7. 坑二：特化的时机 ---\n";    pitfall_timing::demo();
    std::cout << "--- 8. 坑三：特化不继承 ---\n";    pitfall_no_inherit::demo();
}
