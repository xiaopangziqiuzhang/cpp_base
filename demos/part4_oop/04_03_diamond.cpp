// std=c++17
// 对应章节: blog/part4_oop/04-03_diamond.md
// 主题: 成员名冲突的二义性、菱形继承、虚继承、对象布局

#include <iostream>
#include <string>

// ---------- 1. 多基继承的成员名冲突 ----------
class A1 {
public:
    void common() const { std::cout << "        A1::common()\n"; }
    int _value = 1;
};

class A2 {
public:
    void common() const { std::cout << "        A2::common()\n"; }
    int _value = 2;
};

class BothBases : public A1, public A2 {};

void name_ambiguity() {
    BothBases b;
    // b.common();        // 编译错误：request for member 'common' is ambiguous
    // b._value = 10;     // 编译错误：同样二义
    b.A1::common();        // 用限定名消歧
    b.A2::common();
    std::cout << "      A1::_value=" << b.A1::_value
              << " A2::_value=" << b.A2::_value << "\n";
    std::cout << "      两个基类有同名成员 -> 必须用【类名::】消歧\n";
}

// 用 using 在派生类里挑一个
class PicksOne : public A1, public A2 {
public:
    using A1::common;      // 明确选 A1 的版本
    using A1::_value;
};

void using_disambiguation() {
    PicksOne p;
    p.common();            // 不再二义，走 A1 的版本
    std::cout << "      p._value=" << p._value << "（来自 A1）\n";
}

// ---------- 2. 菱形继承：普通继承会产生两份基类 ----------
class Animal {
public:
    explicit Animal(std::string name = "unnamed") : _name(std::move(name)) {
        std::cout << "        + Animal(" << _name << ")\n";
    }
    virtual ~Animal() { std::cout << "        - Animal(" << _name << ")\n"; }
    void breathe() const { std::cout << "        " << _name << " 呼吸\n"; }
    std::string _name;
};

class Mammal : public Animal {              // 普通继承
public:
    Mammal() : Animal("mammal") { std::cout << "        + Mammal\n"; }
    ~Mammal() override { std::cout << "        - Mammal\n"; }
};

class WingedAnimal : public Animal {        // 普通继承
public:
    WingedAnimal() : Animal("winged") { std::cout << "        + WingedAnimal\n"; }
    ~WingedAnimal() override { std::cout << "        - WingedAnimal\n"; }
};

class BatBad : public Mammal, public WingedAnimal {
public:
    BatBad() { std::cout << "        + BatBad\n"; }
    ~BatBad() override { std::cout << "        - BatBad\n"; }
};

void diamond_problem() {
    std::cout << "    普通继承的菱形:\n";
    BatBad bat;
    // bat.breathe();          // 编译错误：二义，不知道用哪份 Animal
    bat.Mammal::breathe();      // 必须指定路径
    bat.WingedAnimal::breathe();
    std::cout << "      注意 Animal 被构造了【两次】，对象里有两份 Animal 数据\n";
    std::cout << "      sizeof(BatBad) = " << sizeof(BatBad) << "\n";
    std::cout << "    ---- 销毁 ----\n";
}

// ---------- 3. 虚继承：共享同一份基类 ----------
class VMammal : public virtual Animal {      // virtual 继承
public:
    VMammal() { std::cout << "        + VMammal\n"; }
    ~VMammal() override { std::cout << "        - VMammal\n"; }
};

class VWingedAnimal : public virtual Animal {
public:
    VWingedAnimal() { std::cout << "        + VWingedAnimal\n"; }
    ~VWingedAnimal() override { std::cout << "        - VWingedAnimal\n"; }
};

class BatGood : public VMammal, public VWingedAnimal {
public:
    // 关键：最终派生类负责构造虚基类
    BatGood() : Animal("bat") { std::cout << "        + BatGood\n"; }
    ~BatGood() override { std::cout << "        - BatGood\n"; }
};

void virtual_inheritance() {
    std::cout << "    虚继承的菱形:\n";
    BatGood bat;
    bat.breathe();              // 不再二义，只有一份 Animal
    std::cout << "      Animal 只被构造【一次】\n";
    std::cout << "      sizeof(BatGood) = " << sizeof(BatGood)
              << "（比普通继承大，因为多了虚基类指针）\n";
    std::cout << "    ---- 销毁 ----\n";
}

// ---------- 4. 虚继承的构造顺序 ----------
class VBase {
public:
    VBase(const char* who) { std::cout << "        + VBase (由 " << who << " 构造)\n"; }
    virtual ~VBase() { std::cout << "        - VBase\n"; }
};

class VMid1 : public virtual VBase {
public:
    VMid1() : VBase("VMid1") { std::cout << "        + VMid1\n"; }
    ~VMid1() override { std::cout << "        - VMid1\n"; }
};

class VMid2 : public virtual VBase {
public:
    VMid2() : VBase("VMid2") { std::cout << "        + VMid2\n"; }
    ~VMid2() override { std::cout << "        - VMid2\n"; }
};

class VMost : public VMid1, public VMid2 {
public:
    VMost() : VBase("VMost") { std::cout << "        + VMost\n"; }
    ~VMost() override { std::cout << "        - VMost\n"; }
};

void virtual_ctor_order() {
    std::cout << "    创建最终派生类 VMost:\n";
    {
        VMost m;
        std::cout << "    ---- 销毁 ----\n";
    }
    std::cout << "      注意：虚基类由【最终派生类】构造，中间类的 VBase(...) 被忽略\n";
    std::cout << "      虚基类【最先】构造、【最后】析构\n";
}

// ---------- 5. 对象布局对比 ----------
class PlainBase { public: int _x = 1; };
class PlainD1 : public PlainBase { public: int _y = 2; };
class PlainD2 : public PlainBase { public: int _z = 3; };
class PlainDiamond : public PlainD1, public PlainD2 { public: int _w = 4; };

class VirtBase { public: int _x = 1; virtual ~VirtBase() = default; };
class VirtD1 : public virtual VirtBase { public: int _y = 2; };
class VirtD2 : public virtual VirtBase { public: int _z = 3; };
class VirtDiamond : public VirtD1, public VirtD2 { public: int _w = 4; };

void layout_comparison() {
    std::cout << "      普通菱形继承:\n";
    std::cout << "        sizeof(PlainBase)    = " << sizeof(PlainBase) << "\n";
    std::cout << "        sizeof(PlainDiamond) = " << sizeof(PlainDiamond)
              << "  <- 含【两份】_x\n";
    PlainDiamond pd;
    std::cout << "        &pd.PlainD1::_x = " << static_cast<void*>(&pd.PlainD1::_x) << "\n";
    std::cout << "        &pd.PlainD2::_x = " << static_cast<void*>(&pd.PlainD2::_x)
              << "  <- 地址不同，确实是两份\n";

    std::cout << "      虚继承菱形:\n";
    std::cout << "        sizeof(VirtBase)    = " << sizeof(VirtBase) << "\n";
    std::cout << "        sizeof(VirtDiamond) = " << sizeof(VirtDiamond)
              << "  <- 只有【一份】_x，但多了虚基类指针\n";
    VirtDiamond vd;
    std::cout << "        &vd.VirtD1::_x = " << static_cast<void*>(&vd.VirtD1::_x) << "\n";
    std::cout << "        &vd.VirtD2::_x = " << static_cast<void*>(&vd.VirtD2::_x)
              << "  <- 地址相同，是同一份\n";
}

// ---------- 6. 推荐做法：接口用纯虚类（无数据成员）----------
class IFlyable {
public:
    virtual ~IFlyable() = default;
    virtual void fly() const = 0;       // 纯虚，无数据成员
};

class ISwimmable {
public:
    virtual ~ISwimmable() = default;
    virtual void swim() const = 0;
};

class Duck : public IFlyable, public ISwimmable {    // 多接口，无菱形问题
public:
    void fly() const override  { std::cout << "        Duck::fly()\n"; }
    void swim() const override { std::cout << "        Duck::swim()\n"; }
};

void interface_approach() {
    Duck d;
    d.fly();
    d.swim();
    std::cout << "      sizeof(Duck) = " << sizeof(Duck) << "（只有一个虚表指针）\n";
    std::cout << "      接口类没有数据成员 -> 不会有菱形问题，也不需要虚继承\n";
}

int main() {
    std::cout << "--- 1. 成员名冲突 ---\n";        name_ambiguity();
    std::cout << "--- 2. using 消歧 ---\n";        using_disambiguation();
    std::cout << "--- 3. 菱形继承问题 ---\n";      diamond_problem();
    std::cout << "--- 4. 虚继承 ---\n";            virtual_inheritance();
    std::cout << "--- 5. 虚继承构造顺序 ---\n";    virtual_ctor_order();
    std::cout << "--- 6. 对象布局 ---\n";          layout_comparison();
    std::cout << "--- 7. 推荐：接口方式 ---\n";    interface_approach();
}
