# 03-02 构造函数

> 标准：委托构造、继承构造、类内成员初始值 `[C++11]`
> demo：[`03_02_constructor.cpp`](../../demos/part3_class/03_02_constructor.cpp)

## 一、解决了什么问题

C 里创建一个"对象"分两步：分配空间，然后手工初始化。

```c
struct Computer pc;        /* 成员全是垃圾值 */
computer_init(&pc, "Huawei", 5699);   /* 必须记得调用 */
```

两个问题：**可能忘记调用初始化函数**，以及**忘了也没人提醒** ——
编译器不知道 `computer_init` 和 `struct Computer` 有什么关系。
用一个未初始化的对象是未定义行为，而这类 bug 往往表现为"偶尔出错"。

C++ 的构造函数把初始化变成了**语言强制的**：
只要对象被创建，构造函数就一定运行。
"对象存在"从此等价于"对象已正确初始化"，这是 RAII
（见 [02-06](../part2_memory/02-06_raii.md)）的全部基础。

## 二、构造函数的特征

```cpp
class Point {
public:
    Point() { _ix = 0; _iy = 0; }                  // 默认构造函数
    Point(int ix, int iy) { _ix = ix; _iy = iy; }  // 重载
private:
    int _ix;
    int _iy;
};
```

构造函数有几个独特之处：

- **函数名与类名相同**
- **没有返回值，也没有返回类型** —— 即使是 `void` 也不能写
- **在对象创建时自动调用**
- **可以重载**

它用来完成对象成员变量的初始化及其他操作（如为指针成员动态申请内存）。

### 编译器提供的默认构造函数

如果程序员没有显式定义任何构造函数，系统会提供一个默认构造函数。
但这有个前提：**类中没有定义任何构造函数**。

```cpp
class A { int _v; };                    // 编译器提供默认构造函数
class B { public: B(int v) {} int _v; };// 编译器不再提供

A a;        // ok
// B b;     // 编译错误：no matching function for call to 'B::B()'
```

一旦显式定义了带参构造函数，编译器就不再自动提供默认构造函数。
还需要默认构造的话，要显式写出来（C++11 可以用 `= default`，见
[03-04](03-04_copy_control.md)）。

注意编译器生成的默认构造函数对内置类型成员**不做初始化** ——
`A a;` 之后 `a._v` 是垃圾值。这是 C++ 的"不为你不用的东西付代价"原则，
也是类内成员初始值（第六节）要解决的问题。

## 三、初始化列表

初始化列表位于构造函数形参列表之后、函数体之前，用冒号开始，
多个成员用逗号分隔：

```cpp
class Point {
public:
    Point(int ix = 0, int iy = 0)
        : _ix(ix)
        , _iy(iy)
    { /* 函数体 */ }
private:
    int _ix;
    int _iy;
};
```

### 为什么优先用初始化列表

因为**初始化列表是初始化，函数体内是赋值**，两者步骤数不同。

```cpp
class InBody {
public:
    InBody() { _t = Tracer(5); }     // 函数体内赋值
private:
    Tracer _t;
};

class InInitList {
public:
    InInitList() : _t(5) {}           // 初始化列表
private:
    Tracer _t;
};
```

实测输出（`Tracer` 在每个操作里打日志）：

```
        Tracer()              <- 函数体版本：先默认构造
        Tracer(int 5)         <- 再构造临时对象
        Tracer::operator=     <- 再赋值。三步！

        Tracer(int 5)         <- 初始化列表版本：直接构造。一步
```

**函数体内赋值要三步（默认构造 + 构造临时对象 + 赋值），
初始化列表只要一步（直接构造）。**

如果没有在初始化列表中显式初始化某个成员，该成员会在构造函数体执行**之前**
完成默认初始化。所以函数体里的赋值永远是"在已初始化的对象上再改一次"。

### 必须用初始化列表的三种成员

有些成员**只能**在初始化列表中初始化，函数体内赋值会编译错误：

```cpp
class MustUseInitList {
public:
    MustUseInitList(int v, int& ref)
        : _const_member(v)       // 1. const 成员
        , _ref_member(ref)       // 2. 引用成员
        , _no_default(v)         // 3. 没有默认构造函数的类成员
    {
        // _const_member = v;    // 编译错误：assignment of read-only member
    }
private:
    const int _const_member;
    int& _ref_member;
    NoDefault _no_default;       // NoDefault 只有带参构造函数
};
```

原因：

1. **const 成员**一经初始化就只读，函数体内的 `=` 是赋值，赋值被禁止
2. **引用成员**必须在声明时绑定，函数体内的 `=` 是给被引用对象赋值，不是绑定
   （见 [01-03](../part1_basics/01-03_reference.md)）
3. **没有默认构造函数的类成员**无法先"默认初始化"再赋值 —— 第一步就做不到

还有一种情况是**基类**：基类的构造必须在派生类的初始化列表里指定，
否则会调用基类的默认构造函数（见 [04-02](../part4_oop/04-02_derived_lifecycle.md)）。

### 类对象成员

当数据成员本身是自定义类类型时，必须在初始化列表中构造它：

```cpp
class Line {
public:
    Line(int x1, int y1, int x2, int y2)
        : _pt1(x1, y1)
        , _pt2(x2, y2)
    {}
private:
    Point _pt1;
    Point _pt2;
};
```

如果不写，系统会自动调用 `Point` 的默认构造函数，
得到的就不是你想要的值了（而且如果 `Point` 没有默认构造函数，直接编译错误）。

## 四、初始化顺序陷阱（经典考题）

**每个成员在初始化列表中只能出现一次。
而它们的初始化顺序不由初始化列表的书写顺序决定，而由成员在类中的声明顺序决定。**

```cpp
class OrderTrap {
public:
    // 书写顺序：先 _iy 后 _ix
    OrderTrap(int a) : _iy(a), _ix(_iy) {}
private:
    int _ix;    // 声明在前 -> 先初始化
    int _iy;    // 声明在后 -> 后初始化
};

OrderTrap t(100);
```

实际执行顺序是：先初始化 `_ix`（用了**还未初始化**的 `_iy`），再初始化 `_iy`。

实测输出：

```
      _ix=32590 _iy=100  <- _ix 用了未初始化的 _iy，值是垃圾
      _ix=100 _iy=100    <- 把书写顺序改成与声明一致后，正确
```

`_ix` 拿到了垃圾值 32590。

### 为什么这样设计

因为**析构顺序必须是构造顺序的严格逆序**，而一个类只有一份成员声明，
却可能有多个构造函数、每个的初始化列表顺序都不同。
如果按书写顺序初始化，析构函数就无法确定该用哪个逆序 ——
所以只能以声明顺序为唯一依据。

### 编译器会警告

GCC 的 `-Wreorder` 正是为此设计的。实测警告：

```
warning: 'OrderTrap::_iy' will be initialized after [-Wreorder]
warning:   'int OrderTrap::_ix' [-Wreorder]
warning:   when initialized here [-Wreorder]
warning: '*this.OrderTrap::_iy' is used uninitialized [-Wuninitialized]
```

**实践规则：初始化列表的书写顺序永远与成员声明顺序保持一致**，
并且开启 `-Wall`（含 `-Wreorder`）。这个警告出现时不要忽略 ——
它意味着你以为的初始化顺序和实际不符。

另外，尽量**不要让一个成员的初始化依赖另一个成员** ——
即使顺序写对了，后续有人调整声明顺序就会引入 bug。

## 五、类内成员初始值 `[C++11]`

C++98 的问题：多个构造函数要重复写相同的初始化。

```cpp
// C++98
class Config {
public:
    Config() : timeout(30), retries(3), name("default") {}
    Config(int t) : timeout(t), retries(3), name("default") {}        // 重复
    Config(int t, int r) : timeout(t), retries(r), name("default") {} // 又重复
private:
    int timeout; int retries; std::string name;
};
```

C++11 允许在声明成员时直接给初始值：

```cpp
class Config {
public:
    Config() = default;                  // 全部用类内初始值
    Config(int t) : timeout(t) {}          // 只覆盖 timeout
private:
    int timeout{30};
    int retries{3};
    std::string name{"default"};
    std::vector<int> tags{1, 2, 3};
};
```

实测：

```
      timeout=30 retries=3 name=default tags=3     <- Config a;
      timeout=60 retries=3 name=default tags=3     <- Config b(60);
```

规则：**初始化列表里显式初始化的成员用列表的值，没提到的用类内初始值。**

好处不只是少打字：**新增成员时不会忘记在某个构造函数里初始化它** ——
忘记初始化成员是未定义行为的常见来源，而编译器通常不会警告。

细节见 [01-11 统一初始化](../part1_basics/01-11_uniform_init.md)。

## 六、委托构造 `[C++11]`

C++98 下，多个构造函数共享初始化逻辑只能抽一个私有的 `init()` 函数：

```cpp
// C++98 的变通做法
class Widget {
public:
    Widget() { init(0, 0); }
    Widget(int a) { init(a, 0); }
private:
    void init(int a, int b) { /* 公共逻辑 */ }   // 只能赋值，不能初始化！
};
```

这个做法有个根本缺陷：`init()` 在函数体里执行，所以只能做**赋值**，
不能初始化 const 成员、引用成员。

C++11 允许一个构造函数**委托**给另一个：

```cpp
class Delegating {
public:
    // 主构造函数：所有真正的初始化逻辑都在这里
    Delegating(int a, int b, const std::string& name)
        : _a(a), _b(b), _name(name) {}

    // 委托给主构造函数
    Delegating(int a, int b) : Delegating(a, b, "unnamed") {}
    Delegating() : Delegating(0, 0, "empty") {}
};
```

实测执行顺序：

```
    两个参数:
      主构造函数 (1,2,unnamed)      <- 先执行被委托的
      委托构造 (int,int)            <- 再执行自己的函数体
```

要点：

- 委托写在初始化列表的位置，而且**只能有它一个**（不能同时委托和初始化成员）
- 被委托的构造函数先完整执行（含函数体），然后才执行委托方的函数体
- 不能循环委托（A → B → A），编译器会报错
- 委托构造函数里抛异常，已构造的部分会被正确析构

实践模式：**写一个"主构造函数"包含全部参数和全部逻辑，其他构造函数都委托给它**。

## 七、继承构造函数 `[C++11]`

基类有多个构造函数时，派生类要为每个都写一个转发版本：

```cpp
// C++98
class DerivedOld : public Base {
public:
    DerivedOld() : Base() {}
    DerivedOld(int a) : Base(a) {}
    DerivedOld(int a, double b) : Base(a, b) {}
    // 基类每加一个构造函数，这里就要跟着加
};
```

C++11 用一行解决：

```cpp
class DerivedNew : public Base {
public:
    using Base::Base;      // 继承基类的全部构造函数
};

DerivedNew n1;             // 调 Base()
DerivedNew n2(42);         // 调 Base(int)
DerivedNew n3(1, 2.5);     // 调 Base(int, double)
```

实测三个都正确转发：

```
      Base()
      Base(int 42)
      Base(int,double) 1,2.5
```

注意事项：

- 不继承默认构造函数、拷贝构造函数、移动构造函数（这三个有自己的规则）
- 派生类**自己的成员**不会被基类构造函数初始化 ——
  所以要配合类内成员初始值使用，否则派生类成员是垃圾值
- 可以用 `using` 继承后再自己定义同签名的构造函数来覆盖某一个

```cpp
class Derived : public Base {
public:
    using Base::Base;
private:
    int _own_member{0};    // 必须有类内初始值，否则继承构造函数时它不被初始化
};
```

## 八、explicit：禁止隐式转换

**单参数构造函数会成为隐式转换通道**：

```cpp
class Implicit {
public:
    Implicit(int v) : _v(v) {}      // 没有 explicit
};

Implicit a = 42;              // 隐式转换，能编译
void f(Implicit o);
f(42);                        // 隐式转换，能编译
```

这往往不是你想要的。`42` 变成一个 `Implicit` 对象，
语义上说不通，但编译器不会提醒。

加 `explicit` 就禁止了这种转换：

```cpp
class Explicit {
public:
    explicit Explicit(int v) : _v(v) {}
};

// Explicit b = 42;           // 编译错误
Explicit b(42);               // 必须显式构造
Explicit c{42};
// f(42);                     // 编译错误
f(Explicit(42));              // 必须显式
```

实测：

```
      Implicit a = 42 能编译（a.value()=42）
      Explicit b = 42 编译错误，必须写 Explicit(42)
```

### 经典反例

`std::vector` 的 `explicit vector(size_type n)` 就是为了防止：

```cpp
std::vector<int> v = 5;       // 如果没有 explicit，这会创建 5 个元素的 vector
```

而真正想要隐式转换的场合也有 —— `std::string` 的
`string(const char*)` 故意不是 `explicit` 的，这样才能写：

```cpp
void f(const std::string& s);
f("literal");                 // 依赖隐式转换，这是设计意图
```

### 实践规则

**单参数构造函数默认加 `explicit`**，除非你明确想要隐式转换。
多参数构造函数在 C++11 之后也可能参与隐式转换
（通过 `{}` 列表初始化），所以也值得考虑加上：

```cpp
class Point {
public:
    Point(int x, int y);          // 没有 explicit
};
void f(Point p);
f({1, 2});                        // 能编译

class Point2 {
public:
    explicit Point2(int x, int y);
};
// f2({1, 2});                    // 编译错误
```

`explicit` 还能用在转换运算符上（见
[运算符重载的类型转换](../part4_oop/04-01_inheritance.md)）
和 C++20 的 `explicit(bool)` 条件 explicit。

## 九、小结

| 要点 | 说明 |
|---|---|
| 特征 | 与类名同名、无返回类型、创建对象时自动调用、可重载 |
| 默认构造函数 | 只在没有任何显式构造函数时由编译器提供；不初始化内置类型成员 |
| 初始化列表 | 是**初始化**（一步），函数体内是**赋值**（三步）|
| 必须用初始化列表 | const 成员、引用成员、无默认构造的类成员、基类 |
| **初始化顺序** | 由**声明顺序**决定，不由书写顺序决定；开 `-Wreorder` |
| 类内成员初始值 `[C++11]` | 默认值写在声明处，构造函数只写要覆盖的 |
| 委托构造 `[C++11]` | 一个构造函数委托给另一个；只能有一个委托目标；不能循环 |
| 继承构造 `[C++11]` | `using Base::Base;`；不含默认/拷贝/移动构造；派生类成员需类内初始值 |
| `explicit` | 禁止隐式转换；单参数构造函数默认应加 |

> 下一章：[03-03 析构函数与对象生命周期](03-03_destructor.md)
