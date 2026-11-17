// std=c++14
// 对应章节: blog/part1_basics/01-10_auto_decltype.md
// 主题: auto 的推导规则与它解决的问题

#include <iostream>
#include <map>
#include <string>
#include <vector>

// ---------- 1. auto 解决的第一个问题：冗长的类型名 ----------
void verbose_type_name() {
    std::map<std::string, std::vector<int>> scores{
        {"alice", {90, 85}},
        {"bob", {70, 95}},
    };

    // C++98 写法：类型名比逻辑还长
    for (std::map<std::string, std::vector<int>>::const_iterator it = scores.begin();
         it != scores.end(); ++it) {
        std::cout << it->first << " has " << it->second.size() << " scores\n";
    }

    // C++11 写法
    for (auto it = scores.begin(); it != scores.end(); ++it) {
        std::cout << it->first << " -> " << it->second[0] << "\n";
    }
}

// ---------- 2. auto 的推导规则：会丢弃引用和顶层 const ----------
void deduction_rules() {
    int x = 10;
    const int cx = x;
    const int& rx = x;

    auto a1 = cx;  // int        —— 顶层 const 被丢弃
    auto a2 = rx;  // int        —— 引用被丢弃，const 也跟着丢了
    auto& a3 = cx; // const int& —— 加了 & 就保留 const
    auto&& a4 = x; // int&       —— 万能引用，绑定左值

    a1 = 100; // 合法：a1 是独立的 int
    a2 = 200; // 合法
    // a3 = 300; // 编译错误：a3 是 const int&
    a4 = 400;     // 合法：改的是 x

    std::cout << "a1=" << a1 << " a2=" << a2 << " a3=" << a3 << " x=" << x << "\n";
}

// ---------- 3. auto 与指针 ----------
void with_pointers() {
    int v = 42;
    int* p = &v;

    auto p1 = p;   // int*
    auto* p2 = p;  // int*，等价但意图更清晰
    const auto* p3 = p; // const int*

    std::cout << *p1 << " " << *p2 << " " << *p3 << "\n";
}

// ---------- 4. 陷阱一：auto 与 initializer_list ----------
void trap_braced_init() {
    auto n1 = 1;     // int
    auto n2{1};      // int —— 见下方说明（N3922）
    auto n3 = {1};   // std::initializer_list<int>，带 = 号就是列表
    auto n4 = {1, 2, 3};

    // 注意 n2：标准文本上 C++11/14 原本规定它是 initializer_list<int>，
    // 由决议 N3922 改成 int。GCC/Clang 把该决议追溯应用到了 -std=c++11，
    // 所以本机 c++11/14/17 三种模式下 n2 都是 int（已实测验证）。
    // 结论：直接初始化(n2)取元素类型，拷贝初始化(n3)取 initializer_list。

    std::cout << "n1=" << n1 << " n2=" << n2
              << " n3.size=" << n3.size()
              << " n4.size=" << n4.size() << "\n";
}

// ---------- 5. 陷阱二：vector<bool> 的代理对象 ----------
void trap_proxy_object() {
    std::vector<bool> flags{true, false, true};

    // operator[] 返回的不是 bool&，而是一个代理对象
    auto proxy = flags[0];     // std::vector<bool>::reference，不是 bool！
    bool real = flags[0];      // 这才是 bool

    flags[0] = false;
    // proxy 仍然绑定在那个 bit 上，所以跟着变了
    std::cout << "proxy=" << proxy << " real=" << real << "\n";
}

// ---------- 6. 陷阱三：auto 丢引用导致的意外拷贝 ----------
struct Heavy {
    std::vector<int> data = std::vector<int>(1000, 7);
    int id = 0;
};

void trap_unintended_copy() {
    std::vector<Heavy> items(3);

    // 每次迭代都拷贝一个 Heavy（含 1000 个 int）
    for (auto item : items) {
        item.id = 1; // 改的是副本，原对象没变
    }
    std::cout << "after copy-loop, items[0].id = " << items[0].id << "\n";

    // 正确：用引用
    for (auto& item : items) {
        item.id = 1;
    }
    std::cout << "after ref-loop,  items[0].id = " << items[0].id << "\n";

    // 只读遍历：const auto&
    int sum = 0;
    for (const auto& item : items) {
        sum += item.id;
    }
    std::cout << "sum of id = " << sum << "\n";
}

int main() {
    std::cout << "--- verbose_type_name ---\n";       verbose_type_name();
    std::cout << "--- deduction_rules ---\n";         deduction_rules();
    std::cout << "--- with_pointers ---\n";           with_pointers();
    std::cout << "--- trap_braced_init ---\n";        trap_braced_init();
    std::cout << "--- trap_proxy_object ---\n";       trap_proxy_object();
    std::cout << "--- trap_unintended_copy ---\n";    trap_unintended_copy();
}
