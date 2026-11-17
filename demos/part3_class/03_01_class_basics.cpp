// std=c++17
// 对应章节: blog/part3_class/03-01_class_basics.md
// 主题: 类的定义、class vs struct、this 指针、成员函数的本质

#include <cstring>
#include <iostream>
#include <string>

// ---------- 1. 类的基本定义 ----------
class Computer {
public:
    // 公有：对外提供的接口
    void setBrand(const char* brand) { std::strncpy(_brand, brand, sizeof(_brand) - 1); }
    void setPrice(double price) { _price = price; }
    void print() const {
        std::cout << "      品牌:" << _brand << " 价格:" << _price << "\n";
    }
protected:
    // 保护：开放给子类
    int _internal_id = 0;
private:
    // 私有：只能类内访问，封装性由此体现
    char _brand[32]{};
    double _price = 0;
};

void basic_definition() {
    Computer pc;
    pc.setBrand("Huawei MateBook14");
    pc.setPrice(5699);
    pc.print();
    // pc._price = 0;     // 编译错误：private
}

// ---------- 2. 成员函数可以在类外定义 ----------
class Point {
public:
    void set(int x, int y);       // 类内声明
    void print() const;
private:
    int _ix = 0;
    int _iy = 0;
};

void Point::set(int x, int y) {   // 类外定义，要用 :: 限定
    _ix = x;
    _iy = y;
}
void Point::print() const {
    std::cout << "      (" << _ix << "," << _iy << ")\n";
}

// ---------- 3. class vs struct：唯一区别是默认访问权限 ----------
struct AsStruct {
    void method() {}        // 默认 public
    int field = 0;          // 默认 public
};

class AsClass {
    void method() {}        // 默认 private
    int field = 0;          // 默认 private
public:
    void accessible() { std::cout << "      class 里显式 public 的成员\n"; }
};

void class_vs_struct() {
    AsStruct s;
    s.field = 1;            // ok：struct 默认 public
    s.method();
    std::cout << "      struct 默认 public，s.field = " << s.field << "\n";

    AsClass c;
    // c.field = 1;         // 编译错误：class 默认 private
    c.accessible();

    // 继承时的默认权限也不同
    std::cout << "      继承时: struct 默认 public 继承，class 默认 private 继承\n";
}

// ---------- 4. this 指针：成员函数如何区分不同对象 ----------
class Counter {
public:
    explicit Counter(int v) : _value(v) {}

    void show() const {
        // 编译器实际传入的是 const Counter* const this
        std::cout << "      this=" << static_cast<const void*>(this)
                  << " _value=" << _value
                  << "（等价于 this->_value）\n";
    }

    // 返回 *this 实现链式调用
    Counter& add(int n) { _value += n; return *this; }
    Counter& mul(int n) { _value *= n; return *this; }

    int value() const { return _value; }
private:
    int _value;
};

void this_pointer() {
    Counter a(1), b(100);
    a.show();
    b.show();
    std::cout << "      两个对象共用同一份成员函数代码，靠 this 区分\n";

    // 链式调用
    Counter c(1);
    c.add(2).mul(10).add(5);
    std::cout << "      链式调用 (1+2)*10+5 = " << c.value() << "\n";
}

// ---------- 5. 成员函数的本质：不占对象空间 ----------
class Empty {};
class OnlyData { int _a; double _b; };
class DataAndMethods {
public:
    void f1() {}
    void f2() {}
    void f3() {}
    static int s_counter;       // 静态成员也不占对象空间
private:
    int _a;
    double _b;
};
int DataAndMethods::s_counter = 0;

void object_layout() {
    std::cout << "      sizeof(Empty)          = " << sizeof(Empty)
              << "（空类也占 1 字节，保证不同对象地址不同）\n";
    std::cout << "      sizeof(OnlyData)       = " << sizeof(OnlyData) << "\n";
    std::cout << "      sizeof(DataAndMethods) = " << sizeof(DataAndMethods)
              << "（加了 3 个成员函数和 1 个静态成员，大小不变）\n";

    DataAndMethods o1, o2;
    std::cout << "      两个对象地址不同: " << (&o1 != &o2) << "\n";
}

// ---------- 6. 通过空指针调用成员函数会发生什么 ----------
class NullPointCall {
public:
    static void test1() {
        std::cout << "      test1(): 静态成员函数，不用 this -> 正常\n";
        std::cout << "        _static = " << _static << "\n";
    }
    void test2() {
        std::cout << "      test2(): 非静态但不访问成员 -> 实践中能跑，但是未定义行为\n";
    }
    void test3(int v) {
        std::cout << "      test3(" << v << "): 只用参数，不访问成员 -> 同上\n";
    }
    void test4() {
        std::cout << "      test4(): 访问 _member 会解引用空的 this -> 崩溃\n";
        // std::cout << _member;   // 真的执行会 segfault
    }
    virtual void test5() {
        std::cout << "      test5(): 虚函数要查虚表，必须解引用 this -> 崩溃\n";
    }
    virtual ~NullPointCall() = default;
private:
    static int _static;
    int _member = 0;
};
int NullPointCall::_static = 42;

void null_pointer_call() {
    NullPointCall* p = nullptr;

    NullPointCall::test1();     // 推荐写法：通过类名调用静态成员函数
    p->test1();                 // 也能工作：静态函数不需要 this

    p->test2();                 // 实践中不崩，但标准上是未定义行为
    p->test3(10);

    std::cout << "      （test4/test5 若真的调用会崩溃，已注释）\n";
    // p->test4();              // 解引用空 this -> segfault
    // p->test5();              // 查虚表 -> segfault
}

// ---------- 7. 类内定义的成员函数隐式是 inline ----------
class InlineDemo {
public:
    int get() const { return _v; }      // 隐式 inline
    int compute() const;                 // 类外定义，不是 inline
private:
    int _v = 1;
};
int InlineDemo::compute() const { return _v * 2; }

inline int InlineDemo_explicit(const InlineDemo& d) { return d.get(); }

void inline_members() {
    InlineDemo d;
    std::cout << "      类内定义的 get() 隐式 inline，返回 " << d.get() << "\n";
    std::cout << "      类外定义的 compute() 不是 inline，返回 " << d.compute() << "\n";
    std::cout << "      （见 01-07：inline 主要是链接属性）\n";
    (void)InlineDemo_explicit(d);
}

int main() {
    std::cout << "--- 1. 类的定义 ---\n";          basic_definition();
    std::cout << "--- 2. 类外定义成员函数 ---\n";
    { Point pt; pt.set(1, 2); pt.print(); }
    std::cout << "--- 3. class vs struct ---\n";   class_vs_struct();
    std::cout << "--- 4. this 指针 ---\n";         this_pointer();
    std::cout << "--- 5. 对象布局 ---\n";          object_layout();
    std::cout << "--- 6. 空指针调用成员函数 ---\n"; null_pointer_call();
    std::cout << "--- 7. 隐式 inline ---\n";       inline_members();
}
