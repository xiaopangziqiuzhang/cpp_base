// std=c++17
// 对应章节: blog/part1_basics/01-04_cast.md
// 主题: 四种 C++ 风格转换 + C 风格转换的危险

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <typeinfo>

// ---------- 1. static_cast：最常用，编译期检查的"正常"转换 ----------
void static_cast_demo() {
    int iNumber = 100;
    float fNumber = static_cast<float>(iNumber);
    std::cout << "int -> float: " << fNumber << "\n";

    // 整型之间，显式表达"我知道会截断"
    double d = 3.99;
    int truncated = static_cast<int>(d);
    std::cout << "double 3.99 -> int: " << truncated << "\n";

    // void* -> 具体类型指针
    void* pvoid = std::malloc(sizeof(int));
    int* pInt = static_cast<int*>(pvoid);
    *pInt = 42;
    std::cout << "void* -> int*: " << *pInt << "\n";
    std::free(pvoid);

    // 但不能在无关的指针类型间转换
    int x = 1;
    int* px = &x;
    // float* pf = static_cast<float*>(px);  // 编译错误：invalid static_cast
    (void)px;

    // 任何类型 -> void（抑制"未使用"警告的标准手法）
    static_cast<void>(iNumber);
}

// ---------- 2. 类层次中的 static_cast：上行安全，下行不安全 ----------
class Base {
public:
    virtual ~Base() = default;
    virtual const char* who() const { return "Base"; }
};
class Derived : public Base {
public:
    const char* who() const override { return "Derived"; }
    void derived_only() const { std::cout << "  只有 Derived 才有的方法\n"; }
};
class Other : public Base {
public:
    const char* who() const override { return "Other"; }
};

void static_cast_hierarchy() {
    Derived d;
    Base* pb = static_cast<Base*>(&d);          // 上行转换：永远安全
    std::cout << "上行转换 who() = " << pb->who() << "\n";

    Derived* pd = static_cast<Derived*>(pb);    // 下行转换：这次恰好是对的
    pd->derived_only();

    // 危险：对象实际是 Other，但 static_cast 不做检查，直接当 Derived 用
    Other o;
    Base* pbo = &o;
    Derived* wrong = static_cast<Derived*>(pbo);  // 编译通过，运行时是未定义行为
    std::cout << "static_cast 把 Other* 当成 Derived*，编译器没有阻止："
              << static_cast<const void*>(wrong) << "\n";
    // wrong->derived_only();   // 真调用就是未定义行为，不要这么做
}

// ---------- 3. dynamic_cast：带运行期检查的下行转换 ----------
void dynamic_cast_demo() {
    Derived d;
    Other o;
    Base* targets[] = { &d, &o };

    for (Base* pb : targets) {
        std::cout << "实际类型 " << pb->who() << ": ";
        if (Derived* pd = dynamic_cast<Derived*>(pb)) {   // 失败返回 nullptr
            std::cout << "dynamic_cast 成功 -> ";
            pd->derived_only();
        } else {
            std::cout << "dynamic_cast 返回 nullptr（安全地失败了）\n";
        }
    }

    // 引用版本失败时抛 std::bad_cast，而不是返回空
    try {
        Base& rb = o;
        Derived& rd = dynamic_cast<Derived&>(rb);
        (void)rd;
    } catch (const std::bad_cast& e) {
        std::cout << "引用形式失败抛异常: " << e.what() << "\n";
    }

    // typeid：另一种运行期类型查询
    Base* pb = &d;
    std::cout << "typeid(*pb).name() = " << typeid(*pb).name() << "\n";
}

// ---------- 4. const_cast：增删 const 属性 ----------
void legacy_c_api(char* buf) { std::cout << "  legacy API 收到: " << buf << "\n"; }

void const_cast_demo() {
    const int number = 100;
    // int* bad = &number;                 // 编译错误
    int* p = const_cast<int*>(&number);    // 编译通过
    std::cout << "const_cast 之后指针有效，*p = " << *p << "\n";
    // *p = 200;   // 未定义行为！number 本身是 const 对象，改它是 UB

    // 正当用途：对象本身不是 const，只是手里的指针/引用带了 const
    int real = 10;
    const int* cp = &real;
    int* writable = const_cast<int*>(cp);
    *writable = 20;                        // 合法：real 本身不是 const
    std::cout << "对非 const 对象去 const 后修改: real = " << real << "\n";

    // 正当用途：对接没有 const 正确性的老 C 接口
    std::string s = "hello";
    legacy_c_api(const_cast<char*>(s.c_str()));
}

// ---------- 5. reinterpret_cast：重新解释比特位 ----------
void reinterpret_cast_demo() {
    // 典型用途：指针与整数之间转换
    int x = 42;
    int* px = &x;
    auto addr = reinterpret_cast<std::uintptr_t>(px);
    int* back = reinterpret_cast<int*>(addr);
    std::cout << "指针 -> 整数 -> 指针，*back = " << *back
              << "  (往返是安全的)\n";

    // 典型用途：按字节查看对象表示
    float f = 1.0f;
    auto* bytes = reinterpret_cast<unsigned char*>(&f);
    std::cout << "float 1.0f 的字节: ";
    for (std::size_t i = 0; i < sizeof(f); ++i) {
        std::cout << std::hex << static_cast<int>(bytes[i]) << " ";
    }
    std::cout << std::dec << "\n";

    // 危险：把 int 的比特当 float 读，是未定义行为（严格别名规则）
    // float* pf = reinterpret_cast<float*>(&x);
    // std::cout << *pf;    // UB，不要这么做；要做类型双关请用 memcpy 或 C++20 bit_cast
}

// ---------- 6. C 风格转换的问题：一个语法干了四件事 ----------
void c_style_problem() {
    const int ci = 10;

    // 下面四行语法完全一样，干的事情危险程度天差地别
    double d1 = (double)ci;                  // 相当于 static_cast
    int* p1 = (int*)&ci;                     // 相当于 const_cast —— 悄悄去掉了 const
    Derived dd; Base* pb = &dd;
    Derived* p2 = (Derived*)pb;              // 相当于 static_cast 下行，无检查
    auto addr = (std::uintptr_t)pb;          // 相当于 reinterpret_cast

    std::cout << "C 风格转换一律写作 (T)x，看不出到底做了什么\n";
    std::cout << "d1=" << d1 << " *p1=" << *p1
              << " p2!=null:" << (p2 != nullptr) << " addr!=0:" << (addr != 0) << "\n";
}

int main() {
    std::cout << "--- 1. static_cast ---\n";       static_cast_demo();
    std::cout << "--- 2. 类层次转换 ---\n";        static_cast_hierarchy();
    std::cout << "--- 3. dynamic_cast ---\n";      dynamic_cast_demo();
    std::cout << "--- 4. const_cast ---\n";        const_cast_demo();
    std::cout << "--- 5. reinterpret_cast ---\n";  reinterpret_cast_demo();
    std::cout << "--- 6. C 风格的问题 ---\n";      c_style_problem();
}
