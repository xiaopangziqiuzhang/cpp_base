// std=c++17
// 对应章节: blog/part1_basics/01-02_const_constexpr.md
// 主题: const 修饰变量/指针/成员函数/对象，以及 const 与宏的差别

#include <algorithm>   // std::max
#include <iostream>
#include <string>
#include <type_traits>

// ---------- 1. const 修饰变量 ----------
void const_variable() {
    const int number1 = 10;   // 两种写法等价
    int const number2 = 20;
    // const int val;         // 编译错误：常量必须初始化
    // number1 = 11;          // 编译错误：assignment of read-only variable

    std::cout << "number1=" << number1 << " number2=" << number2 << "\n";
}

// ---------- 2. const vs 宏：类型检查的差别 ----------
#define MACRO_NUMBER 1024
const int kConstNumber = 1024;

#define MACRO_MAX(a, b) ((a) > (b) ? (a) : (b))

void const_vs_macro() {
    // 宏没有类型，const 有
    std::cout << "sizeof(MACRO_NUMBER) = " << sizeof(MACRO_NUMBER)
              << ", sizeof(kConstNumber) = " << sizeof(kConstNumber) << "\n";
    static_assert(std::is_same<decltype(kConstNumber), const int>::value, "");

    // 宏的副作用：参数被多次求值
    int i = 5, j = 3;
    int r1 = MACRO_MAX(i++, j);     // i 被求值两次！
    std::cout << "MACRO_MAX(i++, j) -> r1=" << r1 << ", i 变成了 " << i
              << "（期望 6，实际被加了两次）\n";

    i = 5;
    int r2 = std::max(i++, j);      // 函数：参数只求值一次
    std::cout << "std::max(i++, j) -> r2=" << r2 << ", i = " << i << "\n";
}

// ---------- 3. const 修饰指针：看 const 在 * 的哪一侧 ----------
void const_pointer() {
    int number1 = 10, number2 = 20;

    const int* p1 = &number1;       // 常量指针：不能改所指内容
    // *p1 = 100;                   // 编译错误
    p1 = &number2;                  // ok：可以改指向

    int const* p2 = &number1;       // 与 p1 完全等价

    int* const p3 = &number1;       // 指针常量：不能改指向
    *p3 = 100;                      // ok：可以改所指内容
    // p3 = &number2;               // 编译错误

    const int* const p4 = &number1; // 两者都不能改

    std::cout << "number1=" << number1 << " *p1=" << *p1
              << " *p3=" << *p3 << " *p4=" << *p4 << "\n";
    (void)p2;
}

// ---------- 4. const 修饰成员函数与对象 ----------
class Computer {
public:
    Computer(const std::string& brand, double price)
        : _brand(brand), _price(price) {}

    // const 成员函数：承诺不修改对象状态
    void print() const {
        std::cout << "品牌:" << _brand << " 价格:" << _price << "\n";
        // _price = 0;        // 编译错误：const 成员函数内不能改成员
        // setPrice(0);       // 编译错误：不能调用非 const 成员函数
    }

    void setPrice(double price) { _price = price; }

    // const 与非 const 可以构成重载
    const std::string& brand() const { std::cout << "[const 版本] "; return _brand; }
    std::string&       brand()       { std::cout << "[非const版本] "; return _brand; }

private:
    std::string _brand;
    double _price;
};

void const_member() {
    Computer pc("Huawei MateBook14", 5699);
    pc.print();
    pc.setPrice(5999);
    pc.print();

    // const 对象只能调用 const 成员函数
    const Computer cpc("ThinkPad X1", 9999);
    cpc.print();
    // cpc.setPrice(1);   // 编译错误：passing 'const Computer' as 'this'

    std::cout << "非 const 对象调用 brand(): " << pc.brand() << "\n";
    std::cout << "const 对象调用 brand():   " << cpc.brand() << "\n";
}

// ---------- 5. const 引用延长临时对象的生命周期 ----------
std::string make_string() { return "temporary"; }

void const_ref_lifetime() {
    // 临时对象本应在表达式结束时销毁，绑定到 const 引用后寿命延长到引用的作用域
    const std::string& ref = make_string();
    std::cout << "ref 仍然有效: " << ref << "\n";

    // std::string& bad = make_string();  // 编译错误：非 const 左值引用不能绑右值
}

// ---------- 6. mutable：const 成员函数中的例外 ----------
class Cache {
public:
    int get() const {
        ++_access_count;    // mutable 成员可以在 const 成员函数中修改
        return _value;
    }
    int access_count() const { return _access_count; }
private:
    int _value = 42;
    mutable int _access_count = 0;
};

void mutable_demo() {
    const Cache c;
    c.get(); c.get(); c.get();
    std::cout << "const 对象被访问了 " << c.access_count() << " 次\n";
}

int main() {
    std::cout << "--- 1. const 变量 ---\n";      const_variable();
    std::cout << "--- 2. const vs 宏 ---\n";     const_vs_macro();
    std::cout << "--- 3. const 指针 ---\n";      const_pointer();
    std::cout << "--- 4. const 成员 ---\n";      const_member();
    std::cout << "--- 5. const 引用寿命 ---\n";  const_ref_lifetime();
    std::cout << "--- 6. mutable ---\n";         mutable_demo();
}
