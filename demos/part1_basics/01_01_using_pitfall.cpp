// std=c++17
// 对应章节: blog/part1_basics/01-01_namespace.md
// 主题: using namespace std 会带来什么具体问题

#include <algorithm>
#include <iostream>
#include <vector>

// ---------- 坑 1：与标准库同名导致歧义 ----------
namespace bad_example {
    // 假设项目里有自己的 count 函数
    int count(const std::vector<int>& v) { return static_cast<int>(v.size()); }
}

void ambiguity_demo() {
    using namespace std;          // 引入了 std::count
    using namespace bad_example;  // 引入了 bad_example::count

    std::vector<int> v{1, 2, 2, 3};

    // 下面这行如果写成 count(v)，两个候选都可行 -> 二义性编译错误
    // int n = count(v);  // 编译错误: reference to 'count' is ambiguous

    // 必须显式限定
    int n = bad_example::count(v);
    int m = static_cast<int>(std::count(v.begin(), v.end(), 2));
    std::cout << "bad_example::count = " << n << ", std::count(2) = " << m << "\n";
}

// ---------- 坑 2：标准库新版本引入新名字，老代码突然编不过 ----------
namespace legacy {
    // C++17 之前，自己实现一个 clamp 很常见
    template <class T>
    const T& clamp(const T& v, const T& lo, const T& hi) {
        return v < lo ? lo : (hi < v ? hi : v);
    }
}

void shadowing_risk() {
    using namespace legacy;
    // 如果这里也 using namespace std;（C++17 起 std::clamp 存在）
    // 那么 clamp(5, 1, 3) 就会变成二义性调用 —— 代码没改，升级标准后就崩了
    std::cout << "legacy::clamp(5,1,3) = " << legacy::clamp(5, 1, 3) << "\n";
    std::cout << "std::clamp(5,1,3)    = " << std::clamp(5, 1, 3) << "\n";
}

// ---------- 坑 3：重载决议被意外改变 ----------
namespace mymath {
    // 自己的 abs，只接受 int
    int abs(int x) { return x < 0 ? -x : x; }
}

void overload_hijack() {
    using namespace mymath;

    // 只有 mymath::abs 可见时，-3.7 会被隐式转成 int，精度丢失
    std::cout << "mymath::abs(-3.7) = " << mymath::abs(static_cast<int>(-3.7)) << "  <- 截断成 3\n";

    // 正确做法：用标准库的浮点版本
    std::cout << "std::abs(-3.7)    = " << std::abs(-3.7) << "\n";
}

// ---------- 推荐做法：局部 using 声明 ----------
void recommended() {
    std::vector<int> v{5, 3, 1, 4};

    // 只引入确实要用的名字，且限制在函数内
    using std::sort;
    sort(v.begin(), v.end());

    std::cout << "sorted:";
    for (int x : v) std::cout << " " << x;
    std::cout << "\n";
}

int main() {
    std::cout << "--- ambiguity ---\n";      ambiguity_demo();
    std::cout << "--- shadowing risk ---\n"; shadowing_risk();
    std::cout << "--- overload hijack ---\n";overload_hijack();
    std::cout << "--- recommended ---\n";    recommended();
}
