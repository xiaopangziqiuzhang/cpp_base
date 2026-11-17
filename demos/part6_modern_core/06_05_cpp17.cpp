// std=c++17
// 对应章节: blog/part6_modern_core/06-05_cpp17.md
// 主题: C++17 语言特性汇总

#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

// ---------- 1. 结构化绑定 ----------
struct Record { int id; std::string name; double score; };

void structured_bindings() {
    // 来源一：struct / 聚合类型
    Record r{1, "alice", 95.5};
    auto [id, name, score] = r;
    std::cout << "      struct: id=" << id << " name=" << name << " score=" << score << "\n";

    // 来源二：tuple / pair
    auto [a, b] = std::make_pair(1, "two");
    std::cout << "      pair: " << a << ", " << b << "\n";

    // 来源三：数组
    int arr[3]{10, 20, 30};
    auto [x, y, z] = arr;
    std::cout << "      array: " << x << " " << y << " " << z << "\n";

    // 绑定引用可以写回
    auto& [rid, rname, rscore] = r;
    rid = 99;
    std::cout << "      通过引用修改: r.id = " << r.id << "\n";

    // 最常用：遍历 map
    std::map<std::string, int> m{{"x", 1}, {"y", 2}};
    std::cout << "      遍历 map: ";
    for (const auto& [k, v] : m) std::cout << k << "=" << v << " ";
    std::cout << "\n";

    // 接住返回多个值的函数
    auto [iter, inserted] = m.insert({"z", 3});
    std::cout << "      insert 返回: inserted=" << inserted
              << " key=" << iter->first << "\n";
}

// ---------- 2. if / switch 初始化语句 ----------
void if_init() {
    std::map<std::string, int> config{{"timeout", 30}};

    if (auto it = config.find("timeout"); it != config.end()) {
        std::cout << "      找到 timeout = " << it->second << "\n";
    } else {
        std::cout << "      未找到\n";
    }
    // it 在这里不可见

    // 与结构化绑定组合（现代 C++ 最常见的写法之一）
    if (auto [it, ok] = config.insert({"retries", 3}); ok) {
        std::cout << "      插入成功: " << it->first << " = " << it->second << "\n";
    }

    switch (int n = static_cast<int>(config.size()); n) {
        case 2:  std::cout << "      switch: 配置项有 2 个\n"; break;
        default: std::cout << "      switch: 配置项有 " << n << " 个\n";
    }

    std::cout << "      价值：把变量的作用域限制在 if/switch 内\n";
}

// ---------- 3. if constexpr ----------
template <class T>
std::string describe(T value) {
    if constexpr (std::is_integral_v<T>) {
        return "整型: " + std::to_string(value);
    } else if constexpr (std::is_floating_point_v<T>) {
        return "浮点: " + std::to_string(value);
    } else {
        return "其他类型";
    }
}

void if_constexpr_demo() {
    std::cout << "      describe(42)    = " << describe(42) << "\n";
    std::cout << "      describe(3.14)  = " << describe(3.14) << "\n";
    std::cout << "      describe(\"x\")   = " << describe("x") << "\n";
    std::cout << "      不成立的分支【不被实例化】，所以可以写对别的类型非法的代码\n";
}

// ---------- 4. 折叠表达式 ----------
template <class... Args>
auto sum(Args... args) { return (0 + ... + args); }

template <class... Args>
void print_all(Args... args) { ((std::cout << args << " "), ...); }

template <class... Args>
bool all_of(Args... args) { return (args && ...); }

void fold_expressions() {
    std::cout << "      sum(1,2,3,4) = " << sum(1, 2, 3, 4) << "\n";
    std::cout << "      sum() = " << sum() << "（带初值的折叠能处理空包）\n";
    std::cout << "      print_all: "; print_all(1, 'a', 2.5, "str"); std::cout << "\n";
    std::cout << "      all_of(true,true,false) = " << all_of(true, true, false) << "\n";
    std::cout << "      一行替代 C++11 的递归 + 终止函数\n";
}

// ---------- 5. CTAD（类模板实参推导）----------
template <class T>
class Box {
public:
    explicit Box(T v) : _value(std::move(v)) {}
    const T& get() const { return _value; }
private:
    T _value;
};

void ctad() {
    Box b1(42);                          // 推导为 Box<int>
    Box b2(std::string("hello"));         // 推导为 Box<std::string>
    std::cout << "      Box b1(42) -> Box<int>: "
              << std::is_same_v<decltype(b1), Box<int>> << "\n";

    std::vector v{1, 2, 3};               // vector<int>
    std::pair p(1, 2.5);                  // pair<int,double>
    std::array arr{1, 2, 3};              // array<int,3>
    std::cout << "      std::vector v{1,2,3} -> vector<int>: "
              << std::is_same_v<decltype(v), std::vector<int>> << "\n";
    std::cout << "      std::array arr{1,2,3} -> array<int,3>: "
              << std::is_same_v<decltype(arr), std::array<int, 3>> << "\n";

    std::mutex mtx;
    std::lock_guard lk(mtx);              // lock_guard<mutex>，不用写模板参数
    std::cout << "      std::lock_guard lk(mtx) 不用再写 <std::mutex>\n";
    std::cout << "      效果：make_pair / make_tuple 基本可以退休了\n";
    (void)b2; (void)p;
}

// ---------- 6. 内联变量 ----------
// C++17 之前：头文件里定义全局常量很别扭
inline constexpr double kPi = 3.14159265358979323846;
inline int g_counter = 0;        // 非 const 的也行

class WithStatic {
public:
    static inline int instance_count = 0;      // 不需要类外定义！
    static inline const std::string kName = "widget";
    WithStatic() { ++instance_count; }
};

void inline_variables() {
    WithStatic a, b, c;
    std::cout << "      kPi = " << kPi << "\n";
    std::cout << "      WithStatic::instance_count = " << WithStatic::instance_count << "\n";
    std::cout << "      WithStatic::kName = " << WithStatic::kName << "\n";
    std::cout << "      C++17 之前静态成员必须在类外定义一次，现在可以 static inline\n";
    std::cout << "      对 header-only 库意义重大\n";
}

// ---------- 7. 嵌套命名空间简化 ----------
namespace a::b::c {
    void func() { std::cout << "        a::b::c::func()\n"; }
}

void nested_namespace() {
    a::b::c::func();
    std::cout << "      C++98: namespace a { namespace b { namespace c { ... } } }\n";
    std::cout << "      C++17: namespace a::b::c { ... }\n";
}

// ---------- 8. 保证的拷贝消除 ----------
class Tracer {
public:
    Tracer() { std::cout << "        Tracer()\n"; }
    Tracer(const Tracer&) { std::cout << "        拷贝构造\n"; }
    Tracer(Tracer&&) noexcept { std::cout << "        移动构造\n"; }
    ~Tracer() { std::cout << "        ~Tracer()\n"; }
};

Tracer make_tracer() { return Tracer{}; }     // 返回纯右值

// 不可移动也不可拷贝的类型，C++17 下仍能从函数返回
class Immovable {
public:
    Immovable() { std::cout << "        Immovable()\n"; }
    Immovable(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
};
Immovable make_immovable() { return Immovable{}; }

void guaranteed_elision() {
    std::cout << "      从函数返回临时对象:\n";
    Tracer t = make_tracer();
    std::cout << "      ^ C++17 保证没有任何拷贝/移动（只有一次构造）\n";

    std::cout << "      甚至能返回【不可拷贝不可移动】的类型:\n";
    Immovable im = make_immovable();
    std::cout << "      ^ C++14 下这行无法编译\n";
    (void)im;
}

// ---------- 9. __has_include ----------
void has_include() {
#if __has_include(<optional>)
    std::cout << "      __has_include(<optional>) 为真\n";
#endif
#if __has_include(<not_a_real_header>)
    std::cout << "      不会执行到这里\n";
#else
    std::cout << "      __has_include(<not_a_real_header>) 为假\n";
#endif
    std::cout << "      用途：条件包含头文件，做跨标准/跨平台兼容\n";
}

// ---------- 10. 其他语言特性 ----------
// 10a. auto 非类型模板参数
template <auto kValue>
void show_value() {
    std::cout << "        值=" << kValue
              << " 类型是 int: " << std::is_same_v<decltype(kValue), int> << "\n";
}

// 10b. 模板模板参数可以用 typename
template <template <class...> typename Container>
void takes_template() { Container<int> c; (void)c; }

// 10c. 构造函数的聚合初始化支持基类
struct Base { int a; };
struct Derived : Base { int b; };

// 10d. noexcept 成为类型系统的一部分
void may_throw() {}
void never_throws() noexcept {}

void other_features() {
    std::cout << "      auto 非类型模板参数:\n";
    show_value<42>();
    show_value<'x'>();

    takes_template<std::vector>();

    Derived d{{1}, 2};        // C++17 起聚合初始化可以初始化基类
    std::cout << "      聚合初始化基类: d.a=" << d.a << " d.b=" << d.b << "\n";

    std::cout << "      noexcept 进入类型系统: "
              << "void(*)() 和 void(*)() noexcept 是不同类型: "
              << !std::is_same_v<decltype(&may_throw), decltype(&never_throws)> << "\n";

    std::cout << "      其他: [[fallthrough]] [[nodiscard]] [[maybe_unused]]（见 01-09）\n";
    std::cout << "            单参数 static_assert（不需要消息）\n";
    static_assert(sizeof(int) == 4);     // C++17：可以省略消息
}

int main() {
    std::cout << "--- 1. 结构化绑定 ---\n";        structured_bindings();
    std::cout << "--- 2. if/switch 初始化 ---\n";  if_init();
    std::cout << "--- 3. if constexpr ---\n";      if_constexpr_demo();
    std::cout << "--- 4. 折叠表达式 ---\n";        fold_expressions();
    std::cout << "--- 5. CTAD ---\n";              ctad();
    std::cout << "--- 6. 内联变量 ---\n";          inline_variables();
    std::cout << "--- 7. 嵌套命名空间 ---\n";      nested_namespace();
    std::cout << "--- 8. 保证的拷贝消除 ---\n";    guaranteed_elision();
    std::cout << "--- 9. __has_include ---\n";     has_include();
    std::cout << "--- 10. 其他特性 ---\n";         other_features();
}
