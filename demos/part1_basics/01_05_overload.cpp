// std=c++17
// 对应章节: blog/part1_basics/01-05_overload.md
// 主题: 函数重载、name mangling、重载决议、默认参数

#include <iostream>
#include <string>

// ---------- 1. 重载：同名不同参 ----------
void print(int x)                { std::cout << "  print(int): " << x << "\n"; }
void print(double x)             { std::cout << "  print(double): " << x << "\n"; }
void print(const char* x)        { std::cout << "  print(const char*): " << x << "\n"; }
void print(const std::string& x) { std::cout << "  print(const string&): " << x << "\n"; }
void print(int x, int y)         { std::cout << "  print(int,int): " << x << "," << y << "\n"; }

void overload_basics() {
    print(42);
    print(3.14);
    print("c string");
    print(std::string("std::string"));
    print(1, 2);
}

// ---------- 2. 什么不构成重载 ----------
int  f_ret(int x) { return x; }
// double f_ret(int x) { return x; }   // 编译错误：仅返回类型不同，不是重载

void f_cv(int x)        { std::cout << "  f_cv(int)\n"; (void)x; }
// void f_cv(const int x) {}          // 编译错误：顶层 const 不参与区分

void f_ref(int& x)       { std::cout << "  f_ref(int&)\n"; (void)x; }
void f_ref(const int& x) { std::cout << "  f_ref(const int&)\n"; (void)x; }  // 这个可以

void not_overload() {
    int a = 1;
    const int b = 2;
    f_ref(a);     // int&
    f_ref(b);     // const int&
    f_ref(3);     // const int&（右值只能绑 const 引用）
    f_cv(a);
    std::cout << "  f_ret(5) = " << f_ret(5) << "\n";
}

// ---------- 3. 重载决议的优先级 ----------
void resolve(int)    { std::cout << "  精确匹配 int\n"; }
void resolve(double) { std::cout << "  精确匹配 double\n"; }

void resolution_demo() {
    resolve(1);        // int：精确匹配
    resolve(1.0);      // double：精确匹配
    resolve('a');      // int：char -> int 是整型提升，优先于 char -> double
    resolve(1.0f);     // double：float -> double 是浮点提升
    resolve(true);     // int：bool -> int 提升
    // resolve(1L);    // 二义性：long -> int 和 long -> double 都是"转换"，同级
}

// ---------- 4. name mangling：重载在链接层面如何区分 ----------
// 下面这些函数编译后符号名不同，可以用 nm 命令观察：
//   g++ -c 01_05_overload.cpp && nm -C 01_05_overload.o | grep mangle_demo
void mangle_demo(int)          {}
void mangle_demo(double)       {}
void mangle_demo(int, char*)   {}

// extern "C" 关闭 mangling，因此不能重载
extern "C" void c_linkage(int) {}
// extern "C" void c_linkage(double) {}   // 编译错误：C 链接的函数不能重载

// ---------- 5. 默认参数 ----------
void func1(int x = 0, int y = 0) {
    std::cout << "  func1: x=" << x << " y=" << y << "\n";
}

// 默认参数必须从右往左连续
// void bad1(int a = 1, int b, int c = 0, int d);   // 编译错误
void ok1(int a, int b, int c = 0, int d = 0) {
    std::cout << "  ok1: " << a << "," << b << "," << c << "," << d << "\n";
}

void default_args() {
    func1();         // 用两个默认值
    func1(1);        // 只给 x
    func1(1, 2);     // 都给
    ok1(1, 2);
    ok1(1, 2, 3, 4);
}

// ---------- 6. 默认参数的陷阱：声明处 vs 定义处 ----------
void declared_with_default(int x = 100);       // 默认值写在声明里（推荐）
void declared_with_default(int x) {            // 定义里不能重复写默认值
    std::cout << "  declared_with_default: " << x << "\n";
}

// ---------- 7. 默认参数替代简单重载 ----------
// 这三个重载
void old_style();
void old_style(int x);
void old_style(int x, int y);
// 可以合成一个
void new_style(int x = 0, int y = 0) {
    std::cout << "  new_style: " << x << "," << y << "\n";
}

// ---------- 8. 默认参数 + 重载 = 二义性 ----------
void risky(int) { std::cout << "  risky(int)\n"; }
void risky(int, int = 0) { std::cout << "  risky(int,int=0)\n"; }

void ambiguity_demo() {
    // risky(1);        // 编译错误：两个候选都能接受一个 int 实参
    risky(1, 2);        // 这个没问题，只有第二个能匹配
}

// ---------- 9. 默认参数的求值时机 ----------
int counter = 0;
int next_id() { return ++counter; }

void with_default_call(int id = next_id()) {    // 每次调用时才求值
    std::cout << "  id = " << id << "\n";
}

void eval_timing() {
    with_default_call();      // next_id() 被调用
    with_default_call();      // 又被调用一次
    with_default_call(100);   // 不调用 next_id()
    std::cout << "  counter = " << counter << "\n";
}

int main() {
    std::cout << "--- 1. 重载基础 ---\n";      overload_basics();
    std::cout << "--- 2. 什么不是重载 ---\n";  not_overload();
    std::cout << "--- 3. 重载决议 ---\n";      resolution_demo();
    std::cout << "--- 5. 默认参数 ---\n";      default_args();
    std::cout << "--- 6. 声明处默认值 ---\n";  declared_with_default();
    std::cout << "--- 7. 替代重载 ---\n";      new_style(); new_style(1); new_style(1, 2);
    std::cout << "--- 8. 二义性 ---\n";        ambiguity_demo();
    std::cout << "--- 9. 求值时机 ---\n";      eval_timing();
    mangle_demo(1); mangle_demo(1.0); mangle_demo(1, nullptr); c_linkage(0);
}
