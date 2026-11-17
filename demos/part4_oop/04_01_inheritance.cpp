// std=c++17
// 对应章节: blog/part4_oop/04-01_inheritance.md
// 主题: 三种派生方式的访问权限、继承的局限、using 改变访问权限

#include <iostream>
#include <string>

// ---------- 1. 继承的基本形式 ----------
class Person {
public:
    Person(std::string name, int age) : _name(std::move(name)), _age(age) {}
    void introduce() const {
        std::cout << "      我是 " << _name << "，" << _age << " 岁\n";
    }
protected:
    std::string _name;      // 开放给派生类
    int _age;
private:
    int _id_number = 0;     // 派生类也访问不到
};

class Student : public Person {
public:
    Student(std::string name, int age, std::string school)
        : Person(std::move(name), age)      // 必须在初始化列表中构造基类
        , _school(std::move(school)) {}

    void study() const {
        // 可以访问基类的 protected 成员
        std::cout << "      " << _name << " 在 " << _school << " 学习\n";
        // std::cout << _id_number;    // 编译错误：基类的 private 成员不可访问
    }
private:
    std::string _school;
};

void basic_inheritance() {
    Student s("张三", 20, "清华");
    s.introduce();     // 继承来的公有成员函数
    s.study();         // 自己的成员函数
    std::cout << "      派生类可以访问基类的 public/protected，不能访问 private\n";
}

// ---------- 2. 三种派生方式对访问权限的影响 ----------
class Base {
public:    int _pub = 1;
protected: int _prot = 2;
private:   int _priv = 3;
public:
    void show() const {
        std::cout << "        Base: pub=" << _pub << " prot=" << _prot
                  << " priv=" << _priv << "\n";
    }
};

class PublicDerived : public Base {
public:
    void test() {
        _pub = 10;      // 仍是 public
        _prot = 20;     // 仍是 protected
        // _priv = 30;  // 不可访问
        std::cout << "        public 继承: pub 仍 public, prot 仍 protected\n";
    }
};

class ProtectedDerived : protected Base {
public:
    void test() {
        _pub = 10;      // 变成 protected
        _prot = 20;     // 仍是 protected
        std::cout << "        protected 继承: pub 降为 protected\n";
    }
};

class PrivateDerived : private Base {
public:
    void test() {
        _pub = 10;      // 变成 private
        _prot = 20;     // 变成 private
        std::cout << "        private 继承: pub/prot 都降为 private\n";
    }
};

// 再派生一层，看权限如何继续传递
class FromProtected : public ProtectedDerived {
public:
    void test2() {
        _pub = 100;     // 在上一层变成 protected，所以这里还能访问
        std::cout << "        从 protected 继承再派生：仍可访问（已是 protected）\n";
    }
};

class FromPrivate : public PrivateDerived {
public:
    void test2() {
        // _pub = 100;  // 编译错误：在上一层已变成 private
        std::cout << "        从 private 继承再派生：不可访问（已是 private）\n";
    }
};

void derivation_modes() {
    PublicDerived a;    a.test();
    ProtectedDerived b; b.test();
    PrivateDerived c;   c.test();
    FromProtected d;    d.test2();
    FromPrivate e;      e.test2();

    // 类外访问
    PublicDerived pd;
    pd._pub = 1;                 // ok：public 继承后仍是 public
    // ProtectedDerived prd; prd._pub = 1;   // 编译错误
    // PrivateDerived prvd; prvd._pub = 1;   // 编译错误
    std::cout << "      只有 public 继承才能在类外访问基类的 public 成员\n";
}

// ---------- 3. public 继承 = is-a，private 继承 = 实现继承 ----------
class Engine {
public:
    void start() { std::cout << "        引擎启动\n"; }
    void stop()  { std::cout << "        引擎停止\n"; }
};

// private 继承：Car 用 Engine 实现，但 Car 不是 Engine
class CarByInheritance : private Engine {
public:
    void drive() {
        start();            // 内部使用基类功能
        std::cout << "        汽车行驶\n";
    }
    // 外部无法把 CarByInheritance 当成 Engine 用
};

// 更推荐：组合（has-a）
class CarByComposition {
public:
    void drive() {
        _engine.start();
        std::cout << "        汽车行驶\n";
    }
private:
    Engine _engine;
};

void inheritance_vs_composition() {
    std::cout << "      private 继承（实现继承）:\n";
    CarByInheritance a; a.drive();
    std::cout << "      组合（推荐）:\n";
    CarByComposition b; b.drive();
    std::cout << "      能用组合就不要用 private 继承：耦合更低、更灵活\n";
}

// ---------- 4. using 改变继承来的成员的访问权限 ----------
class Hidden : private Base {
public:
    using Base::_pub;      // 把 private 继承来的成员重新暴露为 public
    using Base::show;      // 也可以暴露成员函数
};

void using_access() {
    Hidden h;
    h._pub = 42;           // ok：被 using 重新暴露了
    h.show();
    std::cout << "      using 可以把继承来的成员的访问权限提升（但不能超过原始权限）\n";
}

// ---------- 5. using 解决名字隐藏 ----------
class BaseOverload {
public:
    void f(int)    { std::cout << "        BaseOverload::f(int)\n"; }
    void f(double) { std::cout << "        BaseOverload::f(double)\n"; }
    void f(const char*) { std::cout << "        BaseOverload::f(const char*)\n"; }
};

class HidesAll : public BaseOverload {
public:
    void f(int) { std::cout << "        HidesAll::f(int) —— 隐藏了基类的全部 f 重载\n"; }
};

class KeepsAll : public BaseOverload {
public:
    using BaseOverload::f;     // 把基类的全部 f 重载引入本类作用域
    void f(int) { std::cout << "        KeepsAll::f(int)\n"; }
};

void name_hiding() {
    HidesAll a;
    a.f(1);
    // a.f(2.5);        // 编译错误！基类的 f(double) 被隐藏了
    a.BaseOverload::f(2.5);     // 只能显式限定才能调用
    std::cout << "      派生类定义同名函数会隐藏基类【全部】同名重载\n";

    KeepsAll b;
    b.f(1);             // 自己的
    b.f(2.5);           // 基类的，因为有 using
    b.f("text");
    std::cout << "      using Base::f 可以把基类重载拉回来\n";
}

// ---------- 6. 继承的局限：基类的哪些东西不被继承 ----------
class NotInherited {
public:
    NotInherited() { std::cout << "        NotInherited()\n"; }
    NotInherited(int) { std::cout << "        NotInherited(int)\n"; }
    ~NotInherited() {}
    NotInherited& operator=(const NotInherited&) { return *this; }
    friend void friend_func(const NotInherited&);
};
void friend_func(const NotInherited&) { std::cout << "        friend_func\n"; }

class DerivedFrom : public NotInherited {
public:
    using NotInherited::NotInherited;   // [C++11] 显式继承构造函数
};

void inheritance_limits() {
    std::cout << "      不被继承的东西:\n";
    std::cout << "        1. 构造函数（C++11 可用 using 继承）\n";
    std::cout << "        2. 析构函数\n";
    std::cout << "        3. 赋值运算符（会被派生类的隐式版本覆盖）\n";
    std::cout << "        4. 友元关系（友元不是成员）\n";
    DerivedFrom d(42);     // 用 using 继承来的构造函数
}

// ---------- 7. 空基类优化 ----------
class EmptyBase {};
class EmptyBase2 {};
struct DerivedFromEmpty : EmptyBase { int _v; };
struct TwoEmptyBases : EmptyBase, EmptyBase2 { int _v; };
struct HasEmptyMember { EmptyBase _b; int _v; };

void empty_base_optimization() {
    std::cout << "      sizeof(EmptyBase)          = " << sizeof(EmptyBase) << "\n";
    std::cout << "      sizeof(int)                = " << sizeof(int) << "\n";
    std::cout << "      sizeof(DerivedFromEmpty)   = " << sizeof(DerivedFromEmpty)
              << "  <- 空基类不占空间（EBO）\n";
    std::cout << "      sizeof(TwoEmptyBases)      = " << sizeof(TwoEmptyBases)
              << "  <- 两个空基类也不占\n";
    std::cout << "      sizeof(HasEmptyMember)     = " << sizeof(HasEmptyMember)
              << "  <- 作为【成员】则要占空间+对齐\n";
    std::cout << "      这是 STL 用继承而非组合来放 allocator/comparator 的原因\n";
}

int main() {
    std::cout << "--- 1. 继承基础 ---\n";          basic_inheritance();
    std::cout << "--- 2. 三种派生方式 ---\n";      derivation_modes();
    std::cout << "--- 3. 继承 vs 组合 ---\n";      inheritance_vs_composition();
    std::cout << "--- 4. using 改变权限 ---\n";    using_access();
    std::cout << "--- 5. 名字隐藏 ---\n";          name_hiding();
    std::cout << "--- 6. 继承的局限 ---\n";        inheritance_limits();
    std::cout << "--- 7. 空基类优化 ---\n";        empty_base_optimization();
}
