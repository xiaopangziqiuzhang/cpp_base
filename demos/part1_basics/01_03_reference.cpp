// std=c++17
// 对应章节: blog/part1_basics/01-03_reference.md
// 主题: 引用的本质、做参数、做返回值、与指针的对比

#include <iostream>
#include <string>
#include <vector>

// ---------- 1. 引用是别名 ----------
void reference_basics() {
    int a = 1;
    int& ref1 = a;      // ref1 是 a 的别名
    // int& ref2;       // 编译错误：引用必须初始化
    // double& ref3 = a;// 编译错误：类型必须匹配（const double& 可以，绑临时量）

    ref1 = 100;         // 改 ref1 就是改 a
    std::cout << "a=" << a << " ref1=" << ref1 << "\n";
    std::cout << "&a=" << &a << " &ref1=" << &ref1 << "  (地址相同)\n";

    int b = 2;
    ref1 = b;           // 这是赋值（a = b），不是重新绑定！
    std::cout << "ref1 = b 之后: a=" << a << " &ref1=" << &ref1 << " (仍是 a 的地址)\n";
}

// ---------- 2. 引用占不占内存？ ----------
struct WithRef {
    int& r;
    WithRef(int& x) : r(x) {}
};
struct WithPtr {
    int* p;
};

void reference_size() {
    int x = 0;
    int& r = x;

    // sizeof(引用) 返回的是被引用对象的大小，这是语言规定
    std::cout << "sizeof(int)=" << sizeof(int)
              << " sizeof(r)=" << sizeof(r) << "  <- 取的是 int 的大小\n";

    // 但作为类成员时，引用确实占空间（通常等于一个指针）
    std::cout << "sizeof(WithRef)=" << sizeof(WithRef)
              << " sizeof(WithPtr)=" << sizeof(WithPtr) << "  <- 引用成员占一个指针\n";
    (void)r;
}

// ---------- 3. 引用做参数：三种传参方式对比 ----------
class Tracer {
public:
    Tracer(std::string name) : _name(std::move(name)) {}
    Tracer(const Tracer& rhs) : _name(rhs._name) {
        std::cout << "  [拷贝构造] " << _name << "\n";
    }
    const std::string& name() const { return _name; }
private:
    std::string _name;
};

void by_value(Tracer t)        { std::cout << "  by_value: " << t.name() << "\n"; }
void by_cref(const Tracer& t)  { std::cout << "  by_cref:  " << t.name() << "\n"; }
void by_ptr(const Tracer* t)   { std::cout << "  by_ptr:   " << t->name() << "\n"; }

void parameter_passing() {
    Tracer t("obj");
    std::cout << "值传递（注意有拷贝构造）:\n"; by_value(t);
    std::cout << "const 引用传递（无拷贝）:\n"; by_cref(t);
    std::cout << "指针传递（无拷贝，但要解引用）:\n"; by_ptr(&t);
}

// ---------- 4. 交换两个变量：指针版 vs 引用版 ----------
void swap_by_ptr(int* pa, int* pb) {
    if (!pa || !pb) return;        // 指针得检查空
    int tmp = *pa; *pa = *pb; *pb = tmp;
}
void swap_by_ref(int& x, int& y) { // 引用不可能为空，无需检查
    int tmp = x; x = y; y = tmp;
}

void swap_demo() {
    int a = 1, b = 2;
    swap_by_ptr(&a, &b);
    std::cout << "swap_by_ptr: a=" << a << " b=" << b << "\n";
    swap_by_ref(a, b);
    std::cout << "swap_by_ref: a=" << a << " b=" << b << "\n";
}

// ---------- 5. 引用做返回值 ----------
int g_number = 0;

int  return_by_value() { g_number = 100;  return g_number; }  // 返回副本
int& return_by_ref()   { g_number = 1000; return g_number; }  // 返回别名

// 危险示例（已注释，取消注释编译会收到 -Wreturn-local-addr 警告）
// int& return_local_ref() {
//     int local = 1;
//     return local;        // local 在函数返回后销毁 -> 悬垂引用
// }

void return_value_demo() {
    std::cout << "return_by_value() = " << return_by_value() << "\n";
    return_by_ref() = 2000;      // 返回引用，可以作为左值被赋值
    std::cout << "给返回的引用赋值后 g_number = " << g_number << "\n";
}

// ---------- 6. 引用返回的正当用途：链式调用与下标访问 ----------
class Builder {
public:
    Builder& add(const std::string& part) {    // 返回自身引用
        _result += part;
        return *this;
    }
    const std::string& str() const { return _result; }
private:
    std::string _result;
};

class IntArray {
public:
    explicit IntArray(std::size_t n) : _data(n, 0) {}
    int& operator[](std::size_t i)             { return _data[i]; } // 可写
    const int& operator[](std::size_t i) const { return _data[i]; } // 只读
private:
    std::vector<int> _data;
};

void good_usage() {
    Builder b;
    b.add("hello").add(", ").add("world");     // 链式调用依赖引用返回
    std::cout << "Builder: " << b.str() << "\n";

    IntArray arr(3);
    arr[0] = 10;                                // 因为返回 int&，所以能赋值
    arr[1] = 20;
    std::cout << "arr[0]=" << arr[0] << " arr[1]=" << arr[1] << "\n";
}

// ---------- 7. const 引用绑定临时量与隐式转换 ----------
void takes_cref(const std::string& s) { std::cout << "  收到: " << s << "\n"; }

void const_ref_binding() {
    takes_cref("字面量");                  // const char* 隐式转成临时 string
    takes_cref(std::string("临时对象"));

    const int& r = 42;                     // 可以绑定字面量
    std::cout << "const int& r = 42 -> " << r << "\n";

    double d = 3.14;
    const int& ri = d;                     // 绑定到 int 临时量（d 被转换）
    std::cout << "const int& ri = 3.14 -> " << ri << "  (发生了转换，ri 绑的是临时量)\n";
}

int main() {
    std::cout << "--- 1. 基础 ---\n";         reference_basics();
    std::cout << "--- 2. 大小 ---\n";         reference_size();
    std::cout << "--- 3. 做参数 ---\n";       parameter_passing();
    std::cout << "--- 4. swap ---\n";         swap_demo();
    std::cout << "--- 5. 做返回值 ---\n";     return_value_demo();
    std::cout << "--- 6. 正当用途 ---\n";     good_usage();
    std::cout << "--- 7. const 引用绑定 ---\n"; const_ref_binding();
}
