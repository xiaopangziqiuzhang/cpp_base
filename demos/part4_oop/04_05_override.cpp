// std=c++17
// 对应章节: blog/part4_oop/04-05_override.md
// 主题: 重载/隐藏/覆盖的区别、override 和 final

#include <iostream>
#include <string>

// ---------- 1. 三种"同名"的区别 ----------
namespace three_kinds {
    class Base {
    public:
        virtual ~Base() = default;

        // 用于演示覆盖
        virtual void virtual_func() const { std::cout << "        Base::virtual_func()\n"; }

        // 用于演示隐藏
        void normal_func() const { std::cout << "        Base::normal_func()\n"; }

        // 用于演示重载（同一作用域内）
        void overloaded(int) const    { std::cout << "        Base::overloaded(int)\n"; }
        void overloaded(double) const { std::cout << "        Base::overloaded(double)\n"; }
    };

    class Derived : public Base {
    public:
        // 覆盖（override）：虚函数 + 签名相同
        void virtual_func() const override { std::cout << "        Derived::virtual_func()\n"; }

        // 隐藏（hiding）：非虚函数同名，或虚函数签名不同
        void normal_func() const { std::cout << "        Derived::normal_func()\n"; }
    };

    void demo() {
        Derived d;
        Base* p = &d;

        std::cout << "      覆盖（virtual + 签名相同）:\n";
        std::cout << "        通过基类指针: "; p->virtual_func();   // Derived 版本
        std::cout << "        通过对象:     "; d.virtual_func();    // Derived 版本

        std::cout << "      隐藏（非虚函数同名）:\n";
        std::cout << "        通过基类指针: "; p->normal_func();    // Base 版本！
        std::cout << "        通过对象:     "; d.normal_func();     // Derived 版本

        std::cout << "      重载（同一作用域，参数不同）:\n";
        Base b;
        b.overloaded(1);
        b.overloaded(1.5);
    }
}

// ---------- 2. 没有 override 时，写错签名会静默变成"隐藏" ----------
namespace silent_bug {
    class Base {
    public:
        virtual ~Base() = default;
        virtual void f(int x) const { std::cout << "        Base::f(int)\n"; (void)x; }
        virtual void g() const      { std::cout << "        Base::g() const\n"; }
        virtual void h(int x)       { std::cout << "        Base::h(int)\n"; (void)x; }
    };

    class DerivedBuggy : public Base {
    public:
        // 想覆盖，但参数类型写错了 -> 变成一个新的虚函数（隐藏了基类的 f）
        void f(double x) const { std::cout << "        DerivedBuggy::f(double)\n"; (void)x; }

        // 想覆盖，但漏了 const -> 同样是新函数
        void g() { std::cout << "        DerivedBuggy::g() 非const\n"; }

        // 想覆盖，但多写了 const -> 同样是新函数
        void h(int x) const { std::cout << "        DerivedBuggy::h(int) const\n"; (void)x; }
    };

    // 加了 override 之后，上面三个错误全部变成编译错误
    class DerivedChecked : public Base {
    public:
        // void f(double x) const override {}   // error: 'void f(double) const'
        //                                      //   marked 'override', but does not override
        // void g() override {}                 // error: 同上
        // void h(int x) const override {}      // error: 同上
        void f(int x) const override { std::cout << "        DerivedChecked::f(int)\n"; (void)x; }
        void g() const override      { std::cout << "        DerivedChecked::g() const\n"; }
        void h(int x) override       { std::cout << "        DerivedChecked::h(int)\n"; (void)x; }
    };

    void demo() {
        std::cout << "      有 bug 的派生类（三个函数都没覆盖成功）:\n";
        DerivedBuggy buggy;
        Base* p = &buggy;
        std::cout << "        p->f(1):  "; p->f(1);     // 调到了 Base::f！
        std::cout << "        p->g():   "; p->g();      // 调到了 Base::g！
        std::cout << "        p->h(1):  "; p->h(1);     // 调到了 Base::h！
        std::cout << "      ^ 三次都调用了基类版本，覆盖全部失败，而且编译器不报错\n";

        std::cout << "      加了 override 的版本:\n";
        DerivedChecked checked;
        Base* p2 = &checked;
        std::cout << "        p2->f(1): "; p2->f(1);
        std::cout << "        p2->g():  "; p2->g();
        std::cout << "        p2->h(1): "; p2->h(1);
        std::cout << "      ^ 正确覆盖；若签名写错，override 会让编译失败\n";
    }
}

// ---------- 3. override 能检查出的四类错误 ----------
namespace override_checks {
    class Base {
    public:
        virtual ~Base() = default;
        virtual void f() const {}
        void not_virtual() {}
    };

    class Derived : public Base {
    public:
        void f() const override {}          // ok

        // 以下都是编译错误（已注释）：
        // void f(int) const override {}    // 1. 参数列表不同
        // void f() override {}             // 2. const 限定不同
        // void not_virtual() override {}   // 3. 基类的函数不是虚函数
        // void typo() override {}          // 4. 基类根本没有这个函数（拼写错误）
    };

    void demo() {
        std::cout << "      override 能在编译期检查出:\n";
        std::cout << "        1. 参数列表不匹配\n";
        std::cout << "        2. const / 引用限定不匹配\n";
        std::cout << "        3. 基类对应函数不是虚函数\n";
        std::cout << "        4. 函数名拼写错误\n";
        std::cout << "      代价：零（纯编译期检查）\n";
    }
}

// ---------- 4. final：禁止进一步覆盖或继承 ----------
namespace final_demo {
    class Base {
    public:
        virtual ~Base() = default;
        virtual void can_override() { std::cout << "        Base::can_override()\n"; }
        virtual void cannot_override() { std::cout << "        Base::cannot_override()\n"; }
    };

    class Middle : public Base {
    public:
        void can_override() override { std::cout << "        Middle::can_override()\n"; }
        // final：到此为止，派生类不能再覆盖
        void cannot_override() final { std::cout << "        Middle::cannot_override()\n"; }
    };

    class Leaf : public Middle {
    public:
        void can_override() override { std::cout << "        Leaf::can_override()\n"; }
        // void cannot_override() override {}   // 编译错误：
        //   error: virtual function 'virtual void Leaf::cannot_override()'
        //          overriding final function
    };

    // 类也可以是 final：禁止被继承
    class NoInherit final {
    public:
        void f() { std::cout << "        NoInherit::f()\n"; }
    };
    // class Try : public NoInherit {};      // 编译错误：
    //   error: cannot derive from 'final' base 'NoInherit'

    void demo() {
        Leaf leaf;
        Base* p = &leaf;
        std::cout << "      可覆盖的虚函数: "; p->can_override();
        std::cout << "      final 的虚函数: "; p->cannot_override();
        NoInherit ni; ni.f();
        std::cout << "      final 的两种用法: 函数（禁止再覆盖）、类（禁止继承）\n";
    }
}

// ---------- 5. final 可以帮助编译器去虚化 ----------
namespace devirtualization {
    class Base {
    public:
        virtual ~Base() = default;
        virtual int compute(int x) const { return x; }
    };

    class FinalDerived final : public Base {    // 类是 final
    public:
        int compute(int x) const override { return x * 2; }
    };

    // 因为 FinalDerived 是 final，编译器知道这里不可能有更深的派生类，
    // 可以把虚调用优化成直接调用，甚至内联
    int use_final(const FinalDerived& d, int x) { return d.compute(x); }

    void demo() {
        FinalDerived d;
        std::cout << "      use_final(d, 21) = " << use_final(d, 21) << "\n";
        std::cout << "      final 给编译器提供了去虚化（devirtualization）的依据\n";
        std::cout << "      用 -O2 编译时，这类调用可能被内联，开销降为零\n";
    }
}

// ---------- 6. 协变返回类型 ----------
namespace covariant {
    class Base {
    public:
        virtual ~Base() = default;
        virtual Base* clone() const { std::cout << "        Base::clone()\n"; return new Base(*this); }
    };

    class Derived : public Base {
    public:
        // 返回类型可以是派生类的指针/引用（协变），这是覆盖规则的唯一例外
        Derived* clone() const override {
            std::cout << "        Derived::clone() 返回 Derived*\n";
            return new Derived(*this);
        }
    };

    void demo() {
        Derived d;
        Derived* dp = d.clone();         // 静态类型就是 Derived*，不需要转换
        Base* bp = static_cast<Base*>(&d);
        Base* bp2 = bp->clone();          // 通过基类指针调用，静态类型是 Base*
        std::cout << "      协变返回类型：覆盖时返回类型可以更具体\n";
        delete dp;
        delete bp2;
    }
}

int main() {
    std::cout << "--- 1. 三种同名 ---\n";          three_kinds::demo();
    std::cout << "--- 2. 静默的覆盖失败 ---\n";    silent_bug::demo();
    std::cout << "--- 3. override 的检查 ---\n";   override_checks::demo();
    std::cout << "--- 4. final ---\n";             final_demo::demo();
    std::cout << "--- 5. 去虚化 ---\n";            devirtualization::demo();
    std::cout << "--- 6. 协变返回类型 ---\n";      covariant::demo();
}
