// std=c++17
// 对应章节: blog/part4_oop/04-04_virtual.md
// 主题: 虚函数、动态绑定、虚表虚指针的实测、构造析构中的虚调用

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ---------- 1. 没有虚函数时：静态绑定 ----------
namespace without_virtual {
    class Shape {
    public:
        void draw() const { std::cout << "        Shape::draw()\n"; }
    };
    class Circle : public Shape {
    public:
        void draw() const { std::cout << "        Circle::draw()\n"; }
    };

    void demo() {
        Circle c;
        Shape* p = &c;
        p->draw();          // 调用 Shape::draw()！按【指针类型】决定
        c.draw();           // 调用 Circle::draw()
        std::cout << "      没有 virtual 时，调哪个函数由【指针的静态类型】决定\n";
    }
}

// ---------- 2. 有虚函数时：动态绑定 ----------
namespace with_virtual {
    class Shape {
    public:
        virtual ~Shape() = default;
        virtual void draw() const { std::cout << "        Shape::draw()\n"; }
        virtual double area() const { return 0; }
    };
    class Circle : public Shape {
    public:
        explicit Circle(double r) : _r(r) {}
        void draw() const override { std::cout << "        Circle::draw()\n"; }
        double area() const override { return 3.14159 * _r * _r; }
    private:
        double _r;
    };
    class Square : public Shape {
    public:
        explicit Square(double s) : _s(s) {}
        void draw() const override { std::cout << "        Square::draw()\n"; }
        double area() const override { return _s * _s; }
    private:
        double _s;
    };

    void demo() {
        Circle c(1.0);
        Shape* p = &c;
        p->draw();          // 调用 Circle::draw()！按【对象的实际类型】决定
        std::cout << "      有 virtual 时，调哪个函数由【对象的实际类型】决定\n";

        // 多态的实际价值：统一处理不同类型
        std::vector<std::unique_ptr<Shape>> shapes;
        shapes.push_back(std::make_unique<Circle>(1.0));
        shapes.push_back(std::make_unique<Square>(2.0));
        shapes.push_back(std::make_unique<Circle>(3.0));

        double total = 0;
        for (const auto& s : shapes) {
            s->draw();
            total += s->area();
        }
        std::cout << "      总面积 = " << total << "\n";
        std::cout << "      调用方不需要知道具体类型，这就是多态\n";
    }
}

// ---------- 3. 对象大小：虚表指针的代价 ----------
namespace layout {
    class NoVirtual { int _a; };
    class OneVirtual { int _a; public: virtual ~OneVirtual() = default; };
    class ManyVirtual {
        int _a;
    public:
        virtual ~ManyVirtual() = default;
        virtual void f1() {}
        virtual void f2() {}
        virtual void f3() {}
        virtual void f4() {}
    };

    void demo() {
        std::cout << "      sizeof(NoVirtual)   = " << sizeof(NoVirtual) << "\n";
        std::cout << "      sizeof(OneVirtual)  = " << sizeof(OneVirtual)
                  << "  <- 多了一个虚表指针（8 字节）+ 对齐\n";
        std::cout << "      sizeof(ManyVirtual) = " << sizeof(ManyVirtual)
                  << "  <- 5 个虚函数，仍然只多一个指针\n";
        std::cout << "      虚函数的空间代价是【每个对象一个指针】，与虚函数个数无关\n";
    }
}

// ---------- 4. 实测虚表的存在 ----------
namespace vtable_probe {
    class Base {
    public:
        virtual void f0() { std::cout << "        Base::f0()\n"; }
        virtual void f1() { std::cout << "        Base::f1()\n"; }
        virtual ~Base() = default;
    };
    class Derived : public Base {
    public:
        void f0() override { std::cout << "        Derived::f0()\n"; }
        void f1() override { std::cout << "        Derived::f1()\n"; }
    };

    using FuncPtr = void(*)(void*);

    void demo() {
        Base b;
        Derived d;

        // 对象的头部就是虚表指针（GCC/Clang 的 Itanium ABI 布局）
        void** b_vptr = *reinterpret_cast<void***>(&b);
        void** d_vptr = *reinterpret_cast<void***>(&d);

        std::cout << "      Base 对象的虚表地址:    " << static_cast<void*>(b_vptr) << "\n";
        std::cout << "      Derived 对象的虚表地址: " << static_cast<void*>(d_vptr) << "\n";
        std::cout << "      两个虚表不同: " << (b_vptr != d_vptr) << "\n";

        // 通过虚表直接调用函数（纯演示，依赖具体 ABI，不要在生产代码里这么做）
        std::cout << "      直接通过 Base 的虚表槽位 0 调用:\n";
        reinterpret_cast<FuncPtr>(b_vptr[0])(&b);
        std::cout << "      直接通过 Derived 的虚表槽位 0 调用:\n";
        reinterpret_cast<FuncPtr>(d_vptr[0])(&d);

        std::cout << "      同一个类的所有对象共享一张虚表:\n";
        Derived d2;
        void** d2_vptr = *reinterpret_cast<void***>(&d2);
        std::cout << "        d 和 d2 的虚表相同: " << (d_vptr == d2_vptr) << "\n";
    }
}

// ---------- 5. 哪些函数不能是虚函数 ----------
namespace cannot_be_virtual {
    class Demo {
    public:
        // virtual Demo() {}              // 构造函数：对象还没建好，虚表指针未初始化
        // virtual static void f() {}      // 静态成员函数：没有对象，无法查虚表
        // 友元函数：不是成员函数
        // 普通全局函数：不是成员函数
        // 内联展开的函数：虚调用无法在编译期确定目标（inline 请求会被忽略）

        virtual ~Demo() = default;        // 析构函数【可以】是虚函数，而且通常应该是
    };

    void demo() {
        std::cout << "      不能是虚函数:\n";
        std::cout << "        构造函数   —— 调用时虚表指针还没初始化\n";
        std::cout << "        静态成员函数 —— 没有 this，无法找到虚表\n";
        std::cout << "        友元函数/全局函数 —— 不是成员函数\n";
        std::cout << "      可以但要小心:\n";
        std::cout << "        inline 虚函数 —— 虚调用时 inline 请求被忽略\n";
        std::cout << "      应该是虚函数:\n";
        std::cout << "        析构函数（当类会被继承并通过基类指针删除时）\n";
    }
}

// ---------- 6. 构造/析构函数中调用虚函数 ----------
namespace virtual_in_ctor {
    class Base {
    public:
        Base() {
            std::cout << "        Base() 构造函数里调用 which(): ";
            which();            // 调用的是 Base::which()，不是 Derived::which()
        }
        virtual ~Base() {
            std::cout << "        ~Base() 析构函数里调用 which(): ";
            which();            // 同样是 Base::which()
        }
        virtual void which() const { std::cout << "Base::which()\n"; }
    };

    class Derived : public Base {
    public:
        Derived() {
            std::cout << "        Derived() 构造函数里调用 which(): ";
            which();            // 这里才是 Derived::which()
        }
        ~Derived() override {
            std::cout << "        ~Derived() 析构函数里调用 which(): ";
            which();
        }
        void which() const override { std::cout << "Derived::which()\n"; }
    };

    void demo() {
        std::cout << "    创建 Derived 对象:\n";
        {
            Derived d;
            std::cout << "    构造完成后调用 which(): ";
            d.which();
            std::cout << "    ---- 销毁 ----\n";
        }
        std::cout << "      规则：构造/析构期间，虚函数调用解析到【当前正在构造/析构的那一层】\n";
        std::cout << "      原因：虚表指针随构造过程逐层更新，析构时逐层回退\n";
        std::cout << "      结论：不要在构造/析构函数里调用虚函数\n";
    }
}

// ---------- 7. 虚函数的五种访问方式 ----------
namespace access_ways {
    class Base {
    public:
        virtual ~Base() = default;
        virtual void f() const { std::cout << "Base::f()\n"; }
        void call_in_member() const {
            std::cout << "        在成员函数中调用: ";
            f();                 // 通过 this->f()，是虚调用
        }
    };
    class Derived : public Base {
    public:
        void f() const override { std::cout << "Derived::f()\n"; }
    };

    void demo() {
        Derived d;

        std::cout << "        指针访问: ";   Base* p = &d; p->f();          // 虚调用
        std::cout << "        引用访问: ";   Base& r = d;  r.f();           // 虚调用
        std::cout << "        对象访问: ";   Base b = d;   b.f();           // 非虚！切片了
        d.call_in_member();                                                  // 虚调用
        std::cout << "        限定名访问: "; d.Base::f();                   // 非虚，强制调基类版本

        std::cout << "      只有通过【指针】或【引用】才有多态；对象访问会切片\n";
    }
}

int main() {
    std::cout << "--- 1. 静态绑定 ---\n";          without_virtual::demo();
    std::cout << "--- 2. 动态绑定 ---\n";          with_virtual::demo();
    std::cout << "--- 3. 对象大小 ---\n";          layout::demo();
    std::cout << "--- 4. 实测虚表 ---\n";          vtable_probe::demo();
    std::cout << "--- 5. 不能是虚函数的 ---\n";    cannot_be_virtual::demo();
    std::cout << "--- 6. 构造/析构中的虚调用 ---\n"; virtual_in_ctor::demo();
    std::cout << "--- 7. 五种访问方式 ---\n";      access_ways::demo();
}
