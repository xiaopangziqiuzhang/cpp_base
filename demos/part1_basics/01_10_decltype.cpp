// std=c++14
// 对应章节: blog/part1_basics/01-10_auto_decltype.md
// 主题: decltype / 尾置返回类型 / decltype(auto)

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

// ---------- 1. decltype 保留引用和 const，这是它与 auto 的根本差别 ----------
void decltype_vs_auto() {
    int x = 0;
    const int& rx = x;

    auto a = rx;          // int
    decltype(rx) d = x;   // const int&

    static_assert(std::is_same<decltype(a), int>::value, "auto 丢掉了 const&");
    static_assert(std::is_same<decltype(d), const int&>::value, "decltype 保留 const&");

    std::cout << "a=" << a << " d=" << d << "\n";
}

// ---------- 2. decltype 对表达式的推导：带不带括号结果不同 ----------
struct Point {
    int x = 1;
    int y = 2;
};

void decltype_parentheses() {
    Point p;

    decltype(p.x)   v1 = 10; // int      —— 成员访问本身，得到声明类型
    decltype((p.x)) v2 = p.x; // int&    —— 加括号后是左值表达式，得到左值引用

    v2 = 99; // 改到了 p.x

    static_assert(std::is_same<decltype(v1), int>::value, "");
    static_assert(std::is_same<decltype(v2), int&>::value, "");

    std::cout << "p.x=" << p.x << " v1=" << v1 << "\n";
}

// ---------- 3. decltype 解决的核心问题：返回类型依赖参数类型 ----------
// C++98 做不到：返回类型在参数列表之前，那时 t 和 u 还不在作用域里
// template <class T, class U>
// ??? add(T t, U u) { return t + u; }

// C++11：尾置返回类型，把返回类型挪到参数列表之后
template <class T, class U>
auto add_cxx11(T t, U u) -> decltype(t + u) {
    return t + u;
}

// C++14：返回类型可以直接推导
template <class T, class U>
auto add_cxx14(T t, U u) {
    return t + u;
}

void generic_return_type() {
    std::cout << "int+double = " << add_cxx11(1, 2.5) << "\n";
    std::cout << "string+char* = " << add_cxx14(std::string("hello,"), "world") << "\n";

    static_assert(std::is_same<decltype(add_cxx11(1, 2.5)), double>::value, "");
}

// ---------- 4. auto 返回类型会丢引用，decltype(auto) 不会 [C++14] ----------
std::vector<int> g_data{1, 2, 3};

auto           get_by_auto()     { return g_data[0]; } // int，返回副本
decltype(auto) get_by_decltype() { return g_data[0]; } // int&，返回引用

void decltype_auto_demo() {
    static_assert(std::is_same<decltype(get_by_auto()), int>::value, "");
    static_assert(std::is_same<decltype(get_by_decltype()), int&>::value, "");

    get_by_decltype() = 42; // 能写回原容器
    std::cout << "g_data[0]=" << g_data[0] << "\n";
}

// ---------- 5. 完美转发返回值的标准写法 ----------
template <class Container, class Index>
decltype(auto) element_at(Container&& c, Index i) {
    return std::forward<Container>(c)[i];
}

void forwarding_return() {
    std::vector<std::string> v{"a", "b"};
    element_at(v, 0) = "modified"; // 因为返回的是引用，才能赋值
    std::cout << "v[0]=" << v[0] << "\n";
}

int main() {
    std::cout << "--- decltype_vs_auto ---\n";      decltype_vs_auto();
    std::cout << "--- decltype_parentheses ---\n";  decltype_parentheses();
    std::cout << "--- generic_return_type ---\n";   generic_return_type();
    std::cout << "--- decltype_auto_demo ---\n";    decltype_auto_demo();
    std::cout << "--- forwarding_return ---\n";     forwarding_return();
}
