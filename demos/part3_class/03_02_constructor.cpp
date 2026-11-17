// std=c++17
// 对应章节: blog/part3_class/03-02_constructor.md
// 主题: 构造函数、初始化列表与初始化顺序、委托构造、继承构造、explicit
//
// 注意：本文件【故意】保留两个 GCC 警告，它们正是第 4 节要演示的 bug：
//   warning: 'OrderTrap::_iy' will be initialized after [-Wreorder]
//   warning: '*this.OrderTrap::_iy' is used uninitialized [-Wuninitialized]
// 真实项目里这两个警告都应当当作错误处理。

#include <iostream>
#include <string>
#include <vector>

// ---------- 1. 构造函数的基本特征 ----------
class Point {
public:
    Point() {                       // 默认构造函数
        std::cout << "      Point()\n";
        _ix = 0;
        _iy = 0;
    }
    Point(int ix, int iy) {         // 重载的构造函数
        std::cout << "      Point(int,int)\n";
        _ix = ix;
        _iy = iy;
    }
    void print() const { std::cout << "      (" << _ix << "," << _iy << ")\n"; }
private:
    int _ix;
    int _iy;
};

void basics() {
    Point p1;        p1.print();
    Point p2(1, 2);  p2.print();
    std::cout << "      构造函数：与类同名、无返回类型、创建对象时自动调用、可重载\n";
}

// ---------- 2. 初始化列表 vs 函数体内赋值 ----------
class Tracer {
public:
    Tracer() { std::cout << "        Tracer()\n"; }
    Tracer(int v) { std::cout << "        Tracer(int " << v << ")\n"; }
    Tracer(const Tracer&) { std::cout << "        Tracer(const Tracer&)\n"; }
    Tracer& operator=(const Tracer&) {
        std::cout << "        Tracer::operator=\n";
        return *this;
    }
};

class InBody {
public:
    InBody() {
        std::cout << "      函数体内赋值（先默认构造，再赋值，两步）:\n";
        _t = Tracer(5);      // 默认构造 + 构造临时对象 + 赋值
    }
private:
    Tracer _t;
};

class InInitList {
public:
    InInitList() : _t(5) {   // 直接构造，一步
        std::cout << "      初始化列表（直接构造，一步）: ^ 上面只有一次构造\n";
    }
private:
    Tracer _t;
};

void init_list_vs_body() {
    { InBody a; }
    { InInitList b; }
}

// ---------- 3. 必须用初始化列表的三种成员 ----------
class MustUseInitList {
public:
    MustUseInitList(int v, int& ref)
        : _const_member(v)       // const 成员：只能在初始化列表
        , _ref_member(ref)       // 引用成员：只能在初始化列表
        , _no_default(v)         // 没有默认构造函数的类成员：只能在初始化列表
    {
        // _const_member = v;    // 编译错误：assignment of read-only member
        // _ref_member = ref;    // 这是赋值，不是绑定
    }
    void print() const {
        std::cout << "      const=" << _const_member
                  << " ref=" << _ref_member << "\n";
    }
private:
    class NoDefault {
    public:
        explicit NoDefault(int v) : _v(v) {}   // 只有带参构造函数
        int _v;
    };
    const int _const_member;
    int& _ref_member;
    NoDefault _no_default;
};

void must_use_init_list() {
    int x = 42;
    MustUseInitList m(10, x);
    m.print();
    std::cout << "      const 成员、引用成员、无默认构造的类成员，必须用初始化列表\n";
}

// ---------- 4. 初始化顺序由声明顺序决定（经典陷阱）----------
class OrderTrap {
public:
    // 书写顺序：先 _iy 后 _ix；实际顺序：先 _ix 后 _iy（按声明顺序）
    // 所以 _ix(_iy) 用到的 _iy 此时还未初始化！
    OrderTrap(int a) : _iy(a), _ix(_iy) {}

    void print() const {
        std::cout << "      _ix=" << _ix << " _iy=" << _iy
                  << "  <- _ix 用了未初始化的 _iy，值是垃圾\n";
    }
private:
    int _ix;    // 声明在前 -> 先初始化
    int _iy;    // 声明在后 -> 后初始化
};

class OrderFixed {
public:
    OrderFixed(int a) : _ix(a), _iy(_ix) {}   // 顺序与声明一致
    void print() const {
        std::cout << "      _ix=" << _ix << " _iy=" << _iy << "  <- 正确\n";
    }
private:
    int _ix;
    int _iy;
};

void init_order() {
    OrderTrap t(100);
    t.print();
    OrderFixed f(100);
    f.print();
    std::cout << "      规则：成员的初始化顺序由【声明顺序】决定，不由初始化列表的书写顺序决定\n";
    std::cout << "      （GCC 的 -Wreorder 会警告书写顺序与声明顺序不一致）\n";
}

// ---------- 5. 类内成员初始值 [C++11] ----------
class WithDefaults {
public:
    WithDefaults() = default;                 // 全部用类内初始值
    WithDefaults(int t) : timeout(t) {}        // 只覆盖 timeout
    void print() const {
        std::cout << "      timeout=" << timeout << " retries=" << retries
                  << " name=" << name << " tags=" << tags.size() << "\n";
    }
private:
    int timeout{30};                           // [C++11] 类内成员初始值
    int retries{3};
    std::string name{"default"};
    std::vector<int> tags{1, 2, 3};
};

void member_initializers() {
    WithDefaults a;    a.print();
    WithDefaults b(60); b.print();
    std::cout << "      新增成员时不会忘记在某个构造函数里初始化它\n";
}

// ---------- 6. 委托构造 [C++11] ----------
class Delegating {
public:
    // 主构造函数：所有真正的初始化逻辑都在这里
    Delegating(int a, int b, const std::string& name)
        : _a(a), _b(b), _name(name) {
        std::cout << "      主构造函数 (" << _a << "," << _b << "," << _name << ")\n";
    }
    // 其他构造函数委托给它
    Delegating(int a, int b) : Delegating(a, b, "unnamed") {
        std::cout << "      委托构造 (int,int)\n";
    }
    Delegating() : Delegating(0, 0, "empty") {
        std::cout << "      委托构造 ()\n";
    }
private:
    int _a;
    int _b;
    std::string _name;
};

void delegating_ctor() {
    std::cout << "    三个参数:\n"; Delegating d1(1, 2, "full");
    std::cout << "    两个参数:\n"; Delegating d2(1, 2);
    std::cout << "    无参数:\n";   Delegating d3;
    std::cout << "      C++11 之前只能把公共逻辑抽到一个 init() 私有函数里\n";
}

// ---------- 7. 继承构造函数 [C++11] ----------
class Base {
public:
    Base() { std::cout << "      Base()\n"; }
    Base(int a) { std::cout << "      Base(int " << a << ")\n"; }
    Base(int a, double b) { std::cout << "      Base(int,double) " << a << "," << b << "\n"; }
};

class DerivedOld : public Base {
public:
    // C++11 之前：每个基类构造函数都要手写一个转发版本
    DerivedOld() : Base() {}
    DerivedOld(int a) : Base(a) {}
    DerivedOld(int a, double b) : Base(a, b) {}
};

class DerivedNew : public Base {
public:
    using Base::Base;      // [C++11] 一行继承基类的全部构造函数
};

void inheriting_ctor() {
    std::cout << "    手写转发:\n"; DerivedOld o(1, 2.5);
    std::cout << "    using Base::Base:\n";
    DerivedNew n1;
    DerivedNew n2(42);
    DerivedNew n3(1, 2.5);
}

// ---------- 8. explicit：禁止隐式转换 ----------
class Implicit {
public:
    Implicit(int v) : _v(v) {}             // 没有 explicit
    int value() const { return _v; }
private:
    int _v;
};

class Explicit {
public:
    explicit Explicit(int v) : _v(v) {}    // 有 explicit
    int value() const { return _v; }
private:
    int _v;
};

void takes_implicit(Implicit o) { std::cout << "      收到 Implicit(" << o.value() << ")\n"; }
void takes_explicit(Explicit o) { std::cout << "      收到 Explicit(" << o.value() << ")\n"; }

void explicit_demo() {
    Implicit a = 42;            // 隐式转换，能编译
    takes_implicit(42);         // 隐式转换，能编译 —— 可能不是你想要的

    // Explicit b = 42;         // 编译错误
    Explicit b(42);             // 必须显式构造
    Explicit c{42};
    // takes_explicit(42);      // 编译错误
    takes_explicit(Explicit(42));

    std::cout << "      Implicit a = 42 能编译（a.value()=" << a.value() << "）\n";
    std::cout << "      Explicit b = 42 编译错误，必须写 Explicit(42)\n";

    // 单参数构造函数几乎都该加 explicit，经典反例是 vector
    std::vector<int> v1(5);     // 5 个元素
    std::cout << "      vector<int> v1(5) 的 explicit 构造函数防止了 vector<int> v = 5\n";
    (void)b; (void)c; (void)v1;
}

int main() {
    std::cout << "--- 1. 构造函数基础 ---\n";          basics();
    std::cout << "--- 2. 初始化列表 vs 赋值 ---\n";    init_list_vs_body();
    std::cout << "--- 3. 必须用初始化列表 ---\n";      must_use_init_list();
    std::cout << "--- 4. 初始化顺序陷阱 ---\n";        init_order();
    std::cout << "--- 5. 类内初始值 [C++11] ---\n";    member_initializers();
    std::cout << "--- 6. 委托构造 [C++11] ---\n";      delegating_ctor();
    std::cout << "--- 7. 继承构造 [C++11] ---\n";      inheriting_ctor();
    std::cout << "--- 8. explicit ---\n";              explicit_demo();
}
