// std=c++17
// 对应章节: blog/part2_memory/02-04_perfect_forwarding.md
// 主题: 万能引用、引用折叠、std::forward、emplace 原理

#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ---------- 一个会报告自己是怎么被构造的类 ----------
class Probe {
public:
    Probe()                       { std::cout << "      Probe()\n"; }
    Probe(int)                    { std::cout << "      Probe(int)\n"; }
    Probe(int, const char*)       { std::cout << "      Probe(int, const char*)\n"; }
    Probe(const Probe&)           { std::cout << "      Probe(const Probe&)  拷贝\n"; }
    Probe(Probe&&) noexcept       { std::cout << "      Probe(Probe&&)       移动\n"; }
};

void take(Probe&)        { std::cout << "      take(Probe&)        非const左值\n"; }
void take(const Probe&)  { std::cout << "      take(const Probe&)  const左值\n"; }
void take(Probe&&)       { std::cout << "      take(Probe&&)       右值\n"; }

// ---------- 1. 问题：如何写一个"原样传递参数"的包装函数 ----------
// 尝试一：按值传参 —— 多一次拷贝，而且丢失左值/右值信息
void wrapper_by_value(Probe p) { take(p); }          // p 是左值，永远调 take(Probe&)

// 尝试二：按 const 引用 —— 不拷贝，但永远调 const 版本，丢失可写性
void wrapper_by_cref(const Probe& p) { take(p); }

// 尝试三：写三个重载 —— 一个参数要三个重载，两个参数就要九个
void wrapper_overload(Probe& p)       { take(p); }
void wrapper_overload(const Probe& p) { take(p); }
void wrapper_overload(Probe&& p)      { take(std::move(p)); }

void the_problem() {
    Probe lv;
    const Probe clv;

    std::cout << "    按值传参（注意多了一次拷贝，且总是非const左值）:\n";
    wrapper_by_value(lv);

    std::cout << "    按 const& 传参（总是 const 左值）:\n";
    wrapper_by_cref(lv);

    std::cout << "    手写三个重载（正确，但组合爆炸）:\n";
    wrapper_overload(lv);
    wrapper_overload(clv);
    wrapper_overload(Probe{});
}

// ---------- 2. 万能引用 + std::forward = 完美转发 ----------
template <class T>
void wrapper_forward(T&& p) {        // T&& 在模板里是"万能引用"
    take(std::forward<T>(p));        // 原样传递值类别
}

void perfect_forwarding() {
    Probe lv;
    const Probe clv;

    std::cout << "    转发非const左值:\n"; wrapper_forward(lv);
    std::cout << "    转发const左值:\n";   wrapper_forward(clv);
    std::cout << "    转发右值:\n";        wrapper_forward(Probe{});
}

// ---------- 3. 引用折叠：T 被推导成什么 ----------
template <class T>
void show_deduction(T&& /*arg*/) {
    std::cout << "      T 是左值引用吗: " << std::is_lvalue_reference<T>::value
              << ", T&& 是左值引用吗: " << std::is_lvalue_reference<T&&>::value << "\n";
}

void reference_collapsing() {
    int x = 0;
    std::cout << "    传左值 x:\n";  show_deduction(x);        // T = int&,  T&& = int&
    std::cout << "    传右值 10:\n"; show_deduction(10);       // T = int,   T&& = int&&

    // 折叠规则：& & -> &,  & && -> &,  && & -> &,  && && -> &&
    // 只有"右值引用的右值引用"才折叠成右值引用，其余都是左值引用
    //
    // 注意：引用折叠只发生在"通过 typedef/别名或模板参数间接产生"的场合。
    // 直接写 int& & 是语法错误：
    //   error: cannot declare reference to 'int&',
    //          which is not a typedef or a template type argument
    using LRef = int&;
    using RRef = int&&;
    std::cout << "    折叠规则验证（必须经由别名才能表达）:\n";
    std::cout << "      (int&)&   -> 左值引用: "
              << std::is_same<LRef&, int&>::value << "\n";
    std::cout << "      (int&)&& -> 左值引用: "
              << std::is_same<LRef&&, int&>::value << "\n";
    std::cout << "      (int&&)& -> 左值引用: "
              << std::is_same<RRef&, int&>::value << "\n";
    std::cout << "      (int&&)&&-> 右值引用: "
              << std::is_same<RRef&&, int&&>::value << "\n";
}

// ---------- 4. 万能引用 vs 右值引用：差别在于类型是否被推导 ----------
template <class T>
void universal(T&& t) { (void)t; }        // 万能引用：T 由推导得来

void rvalue_only(Probe&& p) { (void)p; }  // 右值引用：类型已确定

template <class T>
void not_universal(const T&& t) { (void)t; }   // 有 const 就不是万能引用

template <class T>
void not_universal2(std::vector<T>&& v) { (void)v; }  // 不是 T&& 的形式，也不是

void universal_vs_rvalue() {
    Probe lv;
    universal(lv);          // ok：万能引用能接左值
    universal(Probe{});     // ok：也能接右值
    // rvalue_only(lv);     // 编译错误：右值引用不能绑左值
    rvalue_only(Probe{});   // ok
    std::cout << "    万能引用能接左值和右值；真正的右值引用只能接右值\n";
}

// ---------- 5. std::move vs std::forward ----------
template <class T>
void always_moves(T&& t) { take(std::move(t)); }      // 不管传进来什么，一律当右值

template <class T>
void forwards(T&& t) { take(std::forward<T>(t)); }    // 原样传递

void move_vs_forward() {
    Probe lv;
    std::cout << "    std::move 不管来源，总是转成右值（可能误偷调用方的对象）:\n";
    always_moves(lv);       // lv 是左值，却被当成右值 -> take(Probe&&)
    std::cout << "    std::forward 保留原来的值类别:\n";
    forwards(lv);           // -> take(Probe&)
}

// ---------- 6. 完美转发的实际应用：emplace / make_unique ----------
// 简化版 make_unique，展示它如何把任意参数转发给构造函数
template <class T, class... Args>
std::unique_ptr<T> my_make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// 简化版容器的 emplace_back
template <class T>
class MiniVector {
public:
    template <class... Args>
    void emplace_back(Args&&... args) {
        // 在已分配的内存上直接构造，参数完美转发给 T 的构造函数
        _items.push_back(T(std::forward<Args>(args)...));
    }
    std::size_t size() const { return _items.size(); }
private:
    std::vector<T> _items;
};

void applications() {
    std::cout << "    my_make_unique<Probe>(1, \"x\") 转发两个参数:\n";
    auto p = my_make_unique<Probe>(1, "x");

    std::cout << "    emplace_back(1) 直接在容器内构造:\n";
    MiniVector<Probe> v;
    v.emplace_back(1);

    std::cout << "    标准库 emplace_back 对比 push_back:\n";
    std::vector<Probe> sv;
    sv.reserve(4);
    std::cout << "      push_back(Probe{}):\n";
    sv.push_back(Probe{});
    std::cout << "      emplace_back(1):\n";
    sv.emplace_back(1);
}

// ---------- 7. 转发时的常见错误 ----------
template <class T>
void forward_twice(T&& t) {
    take(std::forward<T>(t));
    // take(std::forward<T>(t));   // 错误：同一个参数转发两次，
                                   // 第一次已经把资源移走了
}

void common_mistakes() {
    std::cout << "    不要对同一个参数转发两次（第一次就可能把资源移走）\n";
    Probe p;
    forward_twice(std::move(p));

    std::cout << "    万能引用在重载决议中优先级极高，容易吃掉所有调用:\n";
    std::cout << "      template<class T> void f(T&&)  会比  void f(int)  更匹配 short\n";
}

int main() {
    std::cout << "--- 1. 问题 ---\n";                  the_problem();
    std::cout << "--- 2. 完美转发 ---\n";              perfect_forwarding();
    std::cout << "--- 3. 引用折叠 ---\n";              reference_collapsing();
    std::cout << "--- 4. 万能引用 vs 右值引用 ---\n";  universal_vs_rvalue();
    std::cout << "--- 5. move vs forward ---\n";       move_vs_forward();
    std::cout << "--- 6. 实际应用 ---\n";              applications();
    std::cout << "--- 7. 常见错误 ---\n";              common_mistakes();
}
