# 04-03 名字冲突与菱形继承

> demo：[`04_03_diamond.cpp`](../../demos/part4_oop/04_03_diamond.cpp)

多基继承带来两类问题：**成员名冲突**和**菱形继承**。
这一章讲它们的表现、虚继承的解法，以及为什么实践中应该绕开它们。

## 一、成员名冲突的二义性

两个基类有同名成员时，派生类访问它就是二义的：

```cpp
class A1 { public: void common() const; int _value = 1; };
class A2 { public: void common() const; int _value = 2; };

class BothBases : public A1, public A2 {};

BothBases b;
// b.common();       // 编译错误: request for member 'common' is ambiguous
// b._value = 10;    // 编译错误：同样二义
```

注意这**不是重载决议失败**，而是名字查找本身就有两个结果。
即使两个 `common()` 的参数列表不同，也依然二义 ——
因为查找在两个不同的作用域里各找到一个，根本到不了重载决议那一步。

### 解法一：限定名

```cpp
b.A1::common();
b.A2::common();
std::cout << b.A1::_value << " " << b.A2::_value;   // 1 2
```

实测确认两份数据独立存在。

### 解法二：在派生类里用 using 挑一个

```cpp
class PicksOne : public A1, public A2 {
public:
    using A1::common;      // 明确选 A1 的版本
    using A1::_value;
};

PicksOne p;
p.common();      // 不再二义，走 A1 的版本
```

实测可行。这样调用方就不需要每次写限定名了。

## 二、菱形继承：基类被重复包含

当两个基类又有共同的基类时，形成菱形结构：

```
       Animal
       /    \
   Mammal  WingedAnimal
       \    /
        Bat
```

```cpp
class Animal { public: void breathe() const; std::string _name; };
class Mammal : public Animal {};
class WingedAnimal : public Animal {};
class BatBad : public Mammal, public WingedAnimal {};
```

问题是：**`BatBad` 对象里有两份 `Animal`**。

实测构造过程：

```
        + Animal(mammal)       <- 第一份
        + Mammal
        + Animal(winged)       <- 第二份
        + WingedAnimal
        + BatBad
```

`Animal` 的构造函数被调用了**两次**。于是：

```cpp
BatBad bat;
// bat.breathe();            // 编译错误：二义，不知道用哪份 Animal
bat.Mammal::breathe();        // 必须指定路径
bat.WingedAnimal::breathe();
```

实测输出 `mammal 呼吸` 和 `winged 呼吸` —— 两份数据各自独立。

这显然不符合语义：一只蝙蝠不应该有两套呼吸系统、两个名字。

### 验证确实是两份

```cpp
class PlainBase { public: int _x = 1; };
class PlainD1 : public PlainBase { public: int _y = 2; };
class PlainD2 : public PlainBase { public: int _z = 3; };
class PlainDiamond : public PlainD1, public PlainD2 { public: int _w = 4; };
```

实测：

```
        sizeof(PlainBase)    = 4
        sizeof(PlainDiamond) = 20  <- 含【两份】_x
        &pd.PlainD1::_x = 0x7ffd2e7daa30
        &pd.PlainD2::_x = 0x7ffd2e7daa38  <- 地址不同，确实是两份
```

20 字节 = `_x`(4) + `_y`(4) + `_x`(4) + `_z`(4) + `_w`(4)。
两个 `_x` 的地址相差 8 字节，是两个独立的对象。

## 三、虚继承

在继承方式前加 `virtual`，让中间类**共享**同一份基类：

```cpp
class VMammal : public virtual Animal {};
class VWingedAnimal : public virtual Animal {};

class BatGood : public VMammal, public VWingedAnimal {
public:
    BatGood() : Animal("bat") {}      // 关键：最终派生类负责构造虚基类
};
```

实测构造过程：

```
        + Animal(bat)          <- 只构造一次
        + VMammal
        + VWingedAnimal
        + BatGood
```

于是不再二义：

```cpp
BatGood bat;
bat.breathe();        // ok，只有一份 Animal
```

### 验证确实共享

```
        sizeof(VirtBase)    = 16
        sizeof(VirtDiamond) = 48  <- 只有【一份】_x，但多了虚基类指针
        &vd.VirtD1::_x = 0x7ffd2e7daa78
        &vd.VirtD2::_x = 0x7ffd2e7daa78  <- 地址相同，是同一份
```

两条路径访问到**同一个地址** —— 确实是共享的。

### 代价：对象变大

虚继承不是免费的。对比两种菱形：

```
      sizeof(BatBad)  = 80     （普通继承）
      sizeof(BatGood) = 56     （虚继承）
```

这个例子里虚继承反而更小（因为省掉了一整份 `Animal` 的 `std::string`），
但在上面的 `int` 例子里：

```
      sizeof(PlainDiamond) = 20    （普通继承，两份 int）
      sizeof(VirtDiamond)  = 48    （虚继承，一份 int + 虚基类指针）
```

虚继承大得多。原因是**虚基类的位置必须在运行期确定** ——
派生类里存的是指向虚基类子对象的指针（或偏移量），每条继承路径一个。
访问虚基类成员时要多一次间接寻址。

## 四、虚继承的构造顺序（最反直觉的部分）

```cpp
class VBase { public: VBase(const char* who); };
class VMid1 : public virtual VBase { public: VMid1() : VBase("VMid1") {} };
class VMid2 : public virtual VBase { public: VMid2() : VBase("VMid2") {} };
class VMost : public VMid1, public VMid2 { public: VMost() : VBase("VMost") {} };

VMost m;
```

实测输出：

```
        + VBase (由 VMost 构造)      <- 虚基类最先构造，且由最终派生类构造
        + VMid1
        + VMid2
        + VMost
    ---- 销毁 ----
        - VMost
        - VMid2
        - VMid1
        - VBase                      <- 虚基类最后析构
```

两条关键规则：

**规则一：虚基类由"最终派生类"（most derived class）构造。**
`VMid1` 和 `VMid2` 的初始化列表里写的 `VBase("VMid1")`、`VBase("VMid2")`
**被完全忽略了**。实测输出显示是 `由 VMost 构造`。

这个规则是必要的 —— 如果每个中间类都能构造虚基类，那共享的那一份该听谁的？
所以只能由最终派生类决定。

推论：**如果虚基类没有默认构造函数，每个最终派生类都必须显式构造它。**
而中间类也仍然要写（因为它们自己也可能被当作最终派生类使用）。
这是虚继承用起来麻烦的主要原因。

**规则二：虚基类最先构造、最后析构**，优先于所有非虚基类。

## 五、虚函数在菱形继承中的问题

菱形继承里虚函数的覆盖也会二义：

```cpp
class Animal { public: virtual void speak() = 0; };
class Mammal : public virtual Animal { public: void speak() override; };
class WingedAnimal : public virtual Animal { public: void speak() override; };
class Bat : public Mammal, public WingedAnimal {};

Bat b;
// b.speak();      // 编译错误：二义，Mammal::speak 和 WingedAnimal::speak
                   // 都覆盖了同一个虚函数
```

必须在最终派生类里重新覆盖：

```cpp
class Bat : public Mammal, public WingedAnimal {
public:
    void speak() override { Mammal::speak(); }    // 明确选一个，或自己实现
};
```

这类情况叫 **dominance 问题**，在深层菱形里会变得非常难理清。

## 六、推荐做法：用纯接口避开整个问题

实践中的结论很明确：**不要用带数据的多基继承，用纯接口。**

```cpp
class IFlyable {
public:
    virtual ~IFlyable() = default;
    virtual void fly() const = 0;       // 纯虚，没有数据成员
};

class ISwimmable {
public:
    virtual ~ISwimmable() = default;
    virtual void swim() const = 0;
};

class Duck : public IFlyable, public ISwimmable {    // 多接口
public:
    void fly() const override  { /* ... */ }
    void swim() const override { /* ... */ }
};
```

实测：

```
      sizeof(Duck) = 16（只有一个虚表指针）
      接口类没有数据成员 -> 不会有菱形问题，也不需要虚继承
```

为什么这样就没问题了？

- **接口类没有数据成员** → 即使被重复继承，也没有"两份数据"的问题
- **接口类只有纯虚函数** → 不存在"继承到两个实现"的二义性
- **不需要虚继承** → 没有虚基类指针的开销和构造顺序的麻烦

这正是 Java 和 C# 的设计 —— 单继承（实现）+ 多接口。
C++ 允许更灵活的多继承，但实践经验表明那份灵活性的代价高于收益。

### 实践准则

1. **最多一个基类带数据和实现**，其余基类都是纯接口
2. 需要复用多份实现时，用**组合**而不是多继承
3. 真的遇到菱形（通常是改造遗留代码）才用虚继承，并接受它的复杂性
4. 接口类要有虚析构函数（见 [04-06](04-06_abstract.md)）

### 什么时候虚继承是必要的

少数正当场合：

- **`std::basic_iostream`** 虚继承自 `basic_istream` 和 `basic_ostream`，
  而它们都虚继承自 `basic_ios` —— 这是标准库里最著名的菱形
- 需要"多个接口共享同一份状态"的框架设计
- CRTP 的某些高级用法

即使在这些场合，设计者也是在权衡之后才用的，不是默认选择。

## 七、小结

| 问题 | 表现 | 解法 |
|---|---|---|
| 成员名冲突 | `b.common()` 二义 | 限定名 `b.A1::common()`，或 `using A1::common;` |
| 菱形继承 | 基类被包含两份，构造两次，访问二义 | 虚继承（`: public virtual Base`）|
| 虚函数二义 | 两条路径都覆盖了同一虚函数 | 在最终派生类重新覆盖 |

虚继承的要点：

| 要点 | 说明 |
|---|---|
| 效果 | 多条继承路径共享同一份基类子对象（实测地址相同）|
| 构造者 | **最终派生类**负责构造虚基类，中间类的写法被忽略 |
| 构造顺序 | 虚基类**最先**构造、**最后**析构 |
| 代价 | 对象变大（虚基类指针）、访问多一次间接寻址、构造规则复杂 |
| 要求 | 虚基类无默认构造函数时，每个最终派生类都要显式构造它 |

**最重要的一条：用纯接口（无数据成员的抽象类）做多继承，
就不会遇到本章的任何问题。**

> 下一章：[04-04 虚函数与多态](04-04_virtual.md)
