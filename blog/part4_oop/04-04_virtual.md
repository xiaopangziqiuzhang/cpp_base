# 04-04 虚函数与多态

> demo：[`04_04_virtual.cpp`](../../demos/part4_oop/04_04_virtual.cpp)

## 一、为什么需要多态

有了继承，我们可以用基类指针指向派生类对象。但如果没有虚函数，
调用哪个函数由**指针的类型**决定，而不是对象的实际类型：

```cpp
class Shape {
public:
    void draw() const { std::cout << "Shape::draw()\n"; }     // 没有 virtual
};
class Circle : public Shape {
public:
    void draw() const { std::cout << "Circle::draw()\n"; }
};

Circle c;
Shape* p = &c;
p->draw();      // 输出 Shape::draw()  —— 不是我们想要的！
c.draw();       // 输出 Circle::draw()
```

实测确认：

```
        Shape::draw()        <- p->draw()，按指针类型决定
        Circle::draw()       <- c.draw()
```

这叫**静态绑定**（早绑定）：编译期就根据表达式的静态类型确定了调用目标。

问题在于，这让"用统一接口处理不同类型"变得不可能：

```cpp
// C 的做法：手工维护类型标签 + switch
struct Shape {
    enum Kind { CIRCLE, SQUARE } kind;    /* 手工打标签 */
};
void draw(struct Shape* s) {
    switch (s->kind) {                    /* 每加一个类型，所有 switch 都要改 */
        case CIRCLE: draw_circle(s); break;
        case SQUARE: draw_square(s); break;
    }
}
```

这个模式的问题是**新增类型要修改所有已有代码** ——
违反开闭原则，而且漏改一处就是 bug（编译器不会提醒）。

## 二、虚函数：动态绑定

在基类的成员函数前加 `virtual`：

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual void draw() const { std::cout << "Shape::draw()\n"; }
    virtual double area() const { return 0; }
};

class Circle : public Shape {
public:
    void draw() const override { std::cout << "Circle::draw()\n"; }
    double area() const override { return 3.14159 * _r * _r; }
};
```

现在调用目标由**对象的实际类型**决定：

```cpp
Circle c(1.0);
Shape* p = &c;
p->draw();      // 输出 Circle::draw()
```

这叫**动态绑定**（晚绑定）—— 运行期才确定调用目标。

### 多态的实际价值

```cpp
std::vector<std::unique_ptr<Shape>> shapes;
shapes.push_back(std::make_unique<Circle>(1.0));
shapes.push_back(std::make_unique<Square>(2.0));
shapes.push_back(std::make_unique<Circle>(3.0));

double total = 0;
for (const auto& s : shapes) {
    s->draw();
    total += s->area();        // 每个对象用自己的实现
}
```

实测：

```
        Circle::draw()
        Square::draw()
        Circle::draw()
      总面积 = 35.4159
```

**这段循环代码不需要知道有哪些具体类型。** 新增一个 `Triangle` 类，
这段代码一行都不用改 —— 这是开闭原则在语言层面的支持。

### 关键字的细节

- `virtual` 只需在**基类**中写一次，派生类中自动是虚函数
- 派生类覆盖时应该写 `override`（C++11），让编译器检查（见 [04-05](04-05_override.md)）
- 派生类的覆盖版本可以不写 `virtual`，但写了也不错（旧代码风格）
- 覆盖要求签名**完全一致**（参数、const 限定、引用限定）；返回类型允许协变

## 三、实现机制：虚表与虚指针

编译器为每个有虚函数的**类**生成一张**虚函数表（vtable）**，
表里存着该类所有虚函数的地址。每个**对象**的头部存一个指向虚表的
**虚指针（vptr）**。

```
Derived 对象            Derived 的虚表
┌──────────┐          ┌─────────────────┐
│  vptr    │ ───────> │ &Derived::f0    │  槽位 0
├──────────┤          ├─────────────────┤
│  基类数据 │          │ &Derived::f1    │  槽位 1
├──────────┤          ├─────────────────┤
│  自己数据 │          │ &Derived::~Derived │
└──────────┘          └─────────────────┘
```

虚调用 `p->f0()` 被编译成大致这样：

```cpp
(*(p->vptr[0]))(p);      // 取虚表槽位 0 的函数指针，调用它，传 p 作为 this
```

### 实测虚表确实存在

GCC/Clang 用 Itanium ABI，对象头部就是 vptr，所以可以直接探测：

```cpp
Base b;
Derived d;

void** b_vptr = *reinterpret_cast<void***>(&b);
void** d_vptr = *reinterpret_cast<void***>(&d);

// 直接通过虚表槽位调用函数
using FuncPtr = void(*)(void*);
reinterpret_cast<FuncPtr>(b_vptr[0])(&b);
reinterpret_cast<FuncPtr>(d_vptr[0])(&d);
```

实测输出：

```
      Base 对象的虚表地址:    0x5571a75f4bd0
      Derived 对象的虚表地址: 0x5571a75f4ba0
      两个虚表不同: 1
      直接通过 Base 的虚表槽位 0 调用:
        Base::f0()
      直接通过 Derived 的虚表槽位 0 调用:
        Derived::f0()
      同一个类的所有对象共享一张虚表:
        d 和 d2 的虚表相同: 1
```

三个结论：

1. **每个类一张虚表**，不同类的虚表地址不同
2. **同一个类的所有对象共享一张虚表** —— `d` 和 `d2` 的 vptr 相同
3. **虚表的槽位顺序与声明顺序一致**，槽位 0 是第一个虚函数

（这段代码依赖具体 ABI，只用于理解机制，**不要在生产代码里这么写**。）

### 空间代价

```cpp
class NoVirtual { int _a; };
class OneVirtual { int _a; public: virtual ~OneVirtual() = default; };
class ManyVirtual { int _a; public: virtual ~ManyVirtual() = default;
                    virtual void f1() {} /* ... 共 5 个虚函数 */ };
```

实测：

```
      sizeof(NoVirtual)   = 4
      sizeof(OneVirtual)  = 16  <- 多了一个虚表指针（8 字节）+ 对齐
      sizeof(ManyVirtual) = 16  <- 5 个虚函数，仍然只多一个指针
```

**空间代价是每个对象一个指针，与虚函数个数无关** ——
虚函数表本身是类级别的，只有一份。

注意 `OneVirtual` 是 16 而不是 12：vptr 要求 8 字节对齐，
所以 `int _a`（4 字节）后面填了 4 字节。
这意味着给一个小类加虚函数，开销可能比想象的大
（4 字节的类变成 16 字节，翻了四倍）。

## 四、哪些函数不能是虚函数

| 不能是虚函数 | 原因 |
|---|---|
| **构造函数** | 调用构造函数时，vptr 还没初始化（它是构造过程的一部分）|
| **静态成员函数** | 没有 `this`，无法找到虚表 |
| **友元函数** | 不是成员函数 |
| **普通全局函数** | 不是成员函数 |

**析构函数可以是虚函数，而且通常应该是** —— 见 [04-06](04-06_abstract.md)。

构造函数不能是虚函数这一点，解释了 C++ 为什么没有"虚构造函数" ——
要做"根据运行期信息创建对象"，需要用工厂模式或 `clone()` 虚函数
（见 [04-02](04-02_derived_lifecycle.md)）。

### inline 虚函数

```cpp
class Base {
public:
    virtual inline void f() { /* ... */ }      // 合法，但 inline 通常不生效
};
```

虚调用在编译期无法确定目标，所以无法内联。
`inline` 请求会被忽略（除非编译器能确定实际类型，比如通过对象直接调用，
或者开了 LTO 做去虚化）。详见 [04-07](04-07_crtp.md)。

## 五、构造/析构函数中调用虚函数

这是个经典陷阱。参考文档里留了这个问题，答案是：
**虚函数调用会解析到"当前正在构造/析构的那一层"的版本。**

```cpp
class Base {
public:
    Base() { which(); }          // 调用 Base::which()
    virtual ~Base() { which(); } // 也是 Base::which()
    virtual void which() const { std::cout << "Base::which()\n"; }
};

class Derived : public Base {
public:
    Derived() { which(); }       // 这里才是 Derived::which()
    ~Derived() override { which(); }
    void which() const override { std::cout << "Derived::which()\n"; }
};

Derived d;
```

实测输出：

```
        Base() 构造函数里调用 which(): Base::which()        <- 不是 Derived::which()!
        Derived() 构造函数里调用 which(): Derived::which()
    构造完成后调用 which(): Derived::which()
    ---- 销毁 ----
        ~Derived() 析构函数里调用 which(): Derived::which()
        ~Base() 析构函数里调用 which(): Base::which()
```

### 为什么

**vptr 随构造过程逐层更新。** 构造 `Derived` 对象时：

1. 先构造 `Base` 部分 —— 此时 vptr 指向 **`Base` 的虚表**
   （因为 `Derived` 部分还没构造，调用 `Derived::which()` 会访问未初始化的成员）
2. `Base` 构造完成后，vptr 更新为指向 **`Derived` 的虚表**
3. 构造 `Derived` 部分

析构时逆向回退：先把 vptr 退回 `Base` 的虚表，再析构 `Base` 部分。

这个设计是**必要的安全措施** —— 否则 `Base` 的构造函数会调用
`Derived::which()`，而那个函数可能访问尚未初始化的 `Derived` 成员。

### 后果与对策

最危险的情况是**基类构造函数调用纯虚函数**：

```cpp
class Base {
public:
    Base() { init(); }                  // 调用纯虚函数
    virtual void init() = 0;
};
```

这是未定义行为，实践中通常是链接错误或运行时崩溃
（纯虚函数在虚表里的槽位指向 `__cxa_pure_virtual`，调用它会 abort）。

**规则：不要在构造函数和析构函数里调用虚函数。**

如果确实需要"构造完成后做一些由派生类决定的初始化"，用两阶段初始化：

```cpp
class Base {
public:
    virtual void init() = 0;
protected:
    Base() = default;      // 构造函数不调 init
};

// 工厂函数保证 init 被调用
template <class T, class... Args>
std::unique_ptr<T> create(Args&&... args) {
    auto p = std::make_unique<T>(std::forward<Args>(args)...);
    p->init();             // 对象完整构造后才调用
    return p;
}
```

## 六、虚函数的五种访问方式

```cpp
Derived d;

Base* p = &d;  p->f();        // 1. 指针访问  -> 虚调用
Base& r = d;   r.f();         // 2. 引用访问  -> 虚调用
Base b = d;    b.f();         // 3. 对象访问  -> 非虚（切片了）
               d.call_in_member();   // 4. 成员函数中  -> 虚调用
               d.Base::f();  // 5. 限定名访问 -> 非虚，强制调基类版本
```

实测：

```
        指针访问: Derived::f()
        引用访问: Derived::f()
        对象访问: Base::f()          <- 切片，非虚
        在成员函数中调用: Derived::f()
        限定名访问: Base::f()        <- 强制非虚
```

**只有通过指针或引用访问才有多态。** 对象访问时编译器知道确切类型，
会直接调用（而且这里发生了切片，见 [04-02](04-02_derived_lifecycle.md)）。

第 4 种情况值得注意：成员函数内部调用虚函数，隐含是 `this->f()`，
所以是虚调用 —— 这是模板方法模式的基础：

```cpp
class Algorithm {
public:
    void run() {            // 非虚，定义流程骨架
        prepare();
        execute();          // 虚调用，派生类决定具体行为
        cleanup();
    }
protected:
    virtual void execute() = 0;
private:
    void prepare() { /* ... */ }
    void cleanup() { /* ... */ }
};
```

第 5 种情况（限定名）用于在派生类里调用基类版本：

```cpp
void Derived::f() override {
    Base::f();          // 先做基类的事
    // 再做自己的事
}
```

## 七、虚函数与默认参数：不要混用

```cpp
class Base    { public: virtual void f(int x = 10) { /* ... */ } };
class Derived : public Base { public: void f(int x = 20) override { /* ... */ } };

Base* p = new Derived;
p->f();      // 调用 Derived::f，但用的是 Base 的默认值 10！
```

原因：**默认参数是静态绑定**（按指针的静态类型），
**虚函数是动态绑定**（按对象的实际类型）。
两者混用的结果是"调了派生类的实现，用了基类的默认值"。

规则：**虚函数不要带默认参数。** 需要默认值时用非虚的转发函数：

```cpp
class Base {
public:
    void f(int x = 10) { do_f(x); }      // 非虚，带默认参数
protected:
    virtual void do_f(int x) = 0;         // 虚，不带默认参数
};
```

这叫 **NVI（Non-Virtual Interface）惯用法**，
顺带还能在转发函数里做参数检查、加锁、日志。

## 八、小结

| 要点 | 说明 |
|---|---|
| 静态绑定 | 无 `virtual` 时，按**指针/引用的静态类型**决定调用目标 |
| 动态绑定 | 有 `virtual` 时，按**对象的实际类型**决定 |
| 多态的价值 | 调用方不需要知道具体类型；新增类型不用改已有代码 |
| 实现机制 | 每类一张**虚表**，每对象一个**虚指针**；虚调用 = 查表 + 间接调用 |
| 实测验证 | 不同类虚表不同，同类对象共享虚表，槽位顺序与声明顺序一致 |
| 空间代价 | 每个对象一个指针（8 字节 + 对齐），与虚函数个数无关 |
| 不能是虚函数 | 构造函数、静态成员函数、友元函数、全局函数 |
| **构造/析构中的虚调用** | 解析到当前正在构造/析构的那一层；**不要这么做** |
| 原因 | vptr 随构造过程逐层更新，析构时逐层回退 |
| 五种访问 | 只有**指针**和**引用**（含成员函数内的 `this`）有多态 |
| 虚函数 + 默认参数 | 默认参数静态绑定，虚函数动态绑定 —— **不要混用**，改用 NVI |

> 下一章：[04-05 override/final 与三种"同名"](04-05_override.md)
