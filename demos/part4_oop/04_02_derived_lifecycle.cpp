// std=c++17
// 对应章节: blog/part4_oop/04-02_derived_lifecycle.md
// 主题: 派生类构造/析构顺序、多基继承、基类与派生类的转换、复制控制
//
// 注意：本文件【故意】保留一个 GCC 警告，它正是第 6 节要演示的 bug：
//   warning: base class 'class BaseCopy' should be explicitly initialized
//            in the copy constructor [-Wextra]
// 派生类的拷贝构造函数忘记调用基类的拷贝构造，导致基类部分走默认构造。

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ---------- 1. 单继承的构造/析构顺序 ----------
class Member {
public:
    explicit Member(const char* tag) : _tag(tag) {
        std::cout << "        + 成员 " << _tag << "\n";
    }
    ~Member() { std::cout << "        - 成员 " << _tag << "\n"; }
private:
    std::string _tag;
};

class Base {
public:
    Base() : _m("Base::_m") { std::cout << "      [Base 构造函数体]\n"; }
    ~Base() { std::cout << "      [Base 析构函数体]\n"; }
private:
    Member _m;
};

class Derived : public Base {
public:
    Derived() : _m1("Derived::_m1"), _m2("Derived::_m2") {
        std::cout << "      [Derived 构造函数体]\n";
    }
    ~Derived() { std::cout << "      [Derived 析构函数体]\n"; }
private:
    Member _m1;
    Member _m2;
};

void single_inheritance_order() {
    std::cout << "    创建 Derived 对象:\n";
    Derived d;
    std::cout << "    ---- 即将销毁 ----\n";
}

// ---------- 2. 派生类必须在初始化列表中构造基类 ----------
class BaseWithArgs {
public:
    explicit BaseWithArgs(int v) : _v(v) {
        std::cout << "      BaseWithArgs(" << v << ")\n";
    }
    // 没有默认构造函数
    int value() const { return _v; }
private:
    int _v;
};

class DerivedCorrect : public BaseWithArgs {
public:
    explicit DerivedCorrect(int v) : BaseWithArgs(v) {}   // 必须显式构造基类
};

// class DerivedWrong : public BaseWithArgs {
// public:
//     DerivedWrong(int v) { /* 编译错误：no matching function for BaseWithArgs() */ }
// };

void must_init_base() {
    DerivedCorrect d(42);
    std::cout << "      基类没有默认构造函数时，派生类必须显式构造它\n";
    std::cout << "      d.value() = " << d.value() << "\n";
}

// ---------- 3. 多基继承 ----------
class Flyable {
public:
    Flyable() { std::cout << "        + Flyable\n"; }
    virtual ~Flyable() { std::cout << "        - Flyable\n"; }
    virtual void fly() const { std::cout << "        Flyable::fly()\n"; }
};

class Swimmable {
public:
    Swimmable() { std::cout << "        + Swimmable\n"; }
    virtual ~Swimmable() { std::cout << "        - Swimmable\n"; }
    virtual void swim() const { std::cout << "        Swimmable::swim()\n"; }
};

class Duck : public Flyable, public Swimmable {     // 注意基类的顺序
public:
    Duck() { std::cout << "        + Duck\n"; }
    ~Duck() override { std::cout << "        - Duck\n"; }
    void quack() const { std::cout << "        Duck::quack()\n"; }
};

void multiple_inheritance() {
    std::cout << "    构造顺序按【派生列表中的顺序】:\n";
    {
        Duck d;
        d.fly();
        d.swim();
        d.quack();
        std::cout << "    ---- 即将销毁（逆序）----\n";
    }
    std::cout << "      基类构造顺序 = 派生列表顺序，析构顺序相反\n";
}

// ---------- 4. 基类与派生类之间的转换 ----------
class Shape {
public:
    virtual ~Shape() = default;
    virtual void draw() const { std::cout << "        Shape::draw()\n"; }
};

class Circle : public Shape {
public:
    void draw() const override { std::cout << "        Circle::draw()\n"; }
    void circle_only() const { std::cout << "        Circle 专有方法\n"; }
private:
    double _radius = 1.0;
};

void conversions() {
    Circle c;

    // 向上转换（派生 -> 基类）：隐式、安全
    Shape* ps = &c;              // 指针
    Shape& rs = c;               // 引用
    ps->draw();                  // 虚函数 -> 调用 Circle::draw()
    rs.draw();

    // 向下转换（基类 -> 派生）：需要显式，且要检查
    if (Circle* pc = dynamic_cast<Circle*>(ps)) {
        pc->circle_only();
    }

    std::cout << "      向上转换隐式且安全；向下转换需要 dynamic_cast（见 01-04）\n";
}

// ---------- 5. 对象切片：按值传递基类会丢掉派生部分 ----------
void by_value(Shape s)        { std::cout << "        按值传基类: "; s.draw(); }
void by_reference(const Shape& s) { std::cout << "        按引用传基类: "; s.draw(); }

void object_slicing() {
    Circle c;
    by_value(c);         // 发生切片！只拷贝了 Shape 部分，虚函数退化为 Shape::draw()
    by_reference(c);     // 正确：多态生效

    // 容器里存基类对象也会切片
    std::vector<Shape> shapes;
    shapes.push_back(c);           // 切片
    std::cout << "        容器存基类对象: ";
    shapes[0].draw();

    // 正确做法：存指针
    std::vector<std::unique_ptr<Shape>> ptrs;
    ptrs.push_back(std::make_unique<Circle>());
    std::cout << "        容器存智能指针: ";
    ptrs[0]->draw();

    std::cout << "      切片（object slicing）是按值传递多态对象的后果\n";
}

// ---------- 6. 派生类的复制控制 ----------
class BaseCopy {
public:
    BaseCopy() = default;
    BaseCopy(const BaseCopy& rhs) : _b(rhs._b) {
        std::cout << "        BaseCopy 拷贝构造\n";
    }
    BaseCopy& operator=(const BaseCopy& rhs) {
        std::cout << "        BaseCopy 拷贝赋值\n";
        _b = rhs._b;
        return *this;
    }
    virtual ~BaseCopy() = default;
protected:
    int _b = 1;
};

class DerivedCopy : public BaseCopy {
public:
    DerivedCopy() = default;

    // 派生类的拷贝构造必须【显式调用】基类的拷贝构造，
    // 否则基类部分会用默认构造函数初始化
    DerivedCopy(const DerivedCopy& rhs)
        : BaseCopy(rhs)              // 关键：不写就调 BaseCopy()
        , _d(rhs._d) {
        std::cout << "        DerivedCopy 拷贝构造\n";
    }

    DerivedCopy& operator=(const DerivedCopy& rhs) {
        if (this != &rhs) {
            BaseCopy::operator=(rhs);    // 关键：显式调用基类的赋值
            _d = rhs._d;
        }
        std::cout << "        DerivedCopy 拷贝赋值\n";
        return *this;
    }

private:
    int _d = 2;
};

// 忘记调用基类版本的错误示范
class DerivedCopyWrong : public BaseCopy {
public:
    DerivedCopyWrong() = default;
    DerivedCopyWrong(const DerivedCopyWrong& rhs) : _d(rhs._d) {
        // 没写 BaseCopy(rhs) -> 基类部分走默认构造，数据丢失
        std::cout << "        DerivedCopyWrong 拷贝构造（基类部分丢了！）\n";
    }
private:
    int _d = 2;
};

void derived_copy_control() {
    std::cout << "    正确的派生类拷贝:\n";
    DerivedCopy a;
    DerivedCopy b = a;
    DerivedCopy c;
    c = a;

    std::cout << "    忘记调用基类拷贝构造:\n";
    DerivedCopyWrong w1;
    DerivedCopyWrong w2 = w1;
    std::cout << "      注意上面【没有】BaseCopy 拷贝构造的输出\n";
}

int main() {
    std::cout << "--- 1. 单继承顺序 ---\n";        single_inheritance_order();
    std::cout << "--- 2. 必须构造基类 ---\n";      must_init_base();
    std::cout << "--- 3. 多基继承 ---\n";          multiple_inheritance();
    std::cout << "--- 4. 基类派生类转换 ---\n";    conversions();
    std::cout << "--- 5. 对象切片 ---\n";          object_slicing();
    std::cout << "--- 6. 派生类复制控制 ---\n";    derived_copy_control();
}
