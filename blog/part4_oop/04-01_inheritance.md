# 04-01 继承基础

> demo：[`04_01_inheritance.cpp`](../../demos/part4_oop/04_01_inheritance.cpp)

## 一、为什么需要继承

在学习类和对象时，我们从对象抽象出类。但世界不是"一层对象、一层类"那么简单 ——
对象抽象出类，在类的基础上还可以继续抽象，抽象出更高层次的类。
经过抽象的对象论世界，形成的是一个**树状结构**（想想生物分类、金融产品分类、
家用电器分类）。

抽象的层次性形成了"抽象层次树"的概念，而抽象层次树为继承提供了依据。

我们需要继承这个概念，本质上是因为**世界的运作往往发生在某一抽象层次上，
而不是在最低的具体对象层次上**。

举个例子，某人发烧了，对别人说："我生病了，要去医院看医生。"
这句话里有一个代词"我"和三个名词"病"、"医院"、"医生"。
除了"我"运作在最底层的具体对象层，其他三个都运作在抽象层次 ——
在这个语境中，"病"、"医院"、"医生"都是抽象的。
但本质上他确实是生了一个具体的病，要去一个具体的医院看一个具体的医生。

哲学上如何映射这种抽象与具体的关系？靠继承。
拿"医生"来说，所有继承自"医生"类的类所指的具体对象，
都可以替换掉这句话里的那个具体医生，而不影响这句话语义的正确性。

回到编程层面：C 语言里重用代码的方式是**拷贝代码、修改代码**。
C++ 的方式之一是继承 —— 用原有类型定义新类型，
新类型既包含原有类型的成员，也能添加自己的新成员，
而不用把原有类的内容重写一遍。原有类型称为"基类"或"父类"，
在它基础上建立的类称为"派生类"或"子类"。

## 二、继承的定义

派生类需要在类派生列表中明确指出它从哪个基类继承而来：

```cpp
class 派生类名 : public/protected/private 基类名 {
    // ...
};
```

```cpp
class Person {
public:
    Person(std::string name, int age) : _name(std::move(name)), _age(age) {}
    void introduce() const { /* ... */ }
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
        std::cout << _name;            // ok：基类的 protected 成员
        // std::cout << _id_number;    // 编译错误：基类的 private 成员
    }
private:
    std::string _school;
};
```

两个要点：

- **基类必须在派生类的初始化列表中构造**（见 [04-02](04-02_derived_lifecycle.md)）
- **派生类可以访问基类的 public 和 protected 成员，不能访问 private 成员**

`protected` 的意义正在于此 —— 它是专门为继承设计的访问级别：
对外隐藏（像 private），对派生类开放（像 public）。

## 三、三种派生方式对访问权限的影响

基类成员在派生类中的访问权限，取决于**基类中的原始权限**和**继承方式**：

| 基类中的权限 | public 继承 | protected 继承 | private 继承 |
|---|---|---|---|
| `public` | public | protected | private |
| `protected` | protected | protected | private |
| `private` | 不可访问 | 不可访问 | 不可访问 |

记法：**取"原始权限"和"继承方式"中更严格的那个**。
基类的 private 成员无论如何都不可访问。

实测验证：

```
        public 继承: pub 仍 public, prot 仍 protected
        protected 继承: pub 降为 protected
        private 继承: pub/prot 都降为 private
```

### 权限如何继续传递

降级是**累积**的。再派生一层时，用的是上一层降级后的权限：

```cpp
class ProtectedDerived : protected Base {};     // Base::_pub 变成 protected
class FromProtected : public ProtectedDerived {
    void f() { _pub = 1; }      // ok：此时 _pub 是 protected，派生类可访问
};

class PrivateDerived : private Base {};         // Base::_pub 变成 private
class FromPrivate : public PrivateDerived {
    void f() { /* _pub = 1; */ }    // 编译错误：已经是 private
};
```

实测：

```
        从 protected 继承再派生：仍可访问（已是 protected）
        从 private 继承再派生：不可访问（已是 private）
```

### 类外访问

```cpp
PublicDerived pd;
pd._pub = 1;             // ok

ProtectedDerived prd;
// prd._pub = 1;         // 编译错误
```

**只有 public 继承才能在类外访问基类的 public 成员。**

注意默认继承方式：`class` 默认 **private 继承**，`struct` 默认 **public 继承**
（见 [03-01](../part3_class/03-01_class_basics.md)）。
所以写 `class D : B {}` 实际是 private 继承 —— **一定要显式写出继承方式**。

## 四、public 继承是 is-a，其他两种不是

这是继承设计中最重要的一条区分。

**public 继承表达 is-a 关系**：派生类对象**是**一个基类对象，
可以在任何需要基类对象的地方使用（这就是 Liskov 替换原则）。

```cpp
class Student : public Person {};        // Student is-a Person
void greet(const Person& p);
greet(student);                           // 合理
```

**private 继承表达"用它实现"**（implemented-in-terms-of），不是 is-a：

```cpp
class Car : private Engine {      // Car 不是 Engine，只是用 Engine 实现
public:
    void drive() { start(); }      // 内部使用基类功能
};
// 外部无法把 Car 当成 Engine 用
```

### 能用组合就不要用 private 继承

```cpp
// private 继承
class CarByInheritance : private Engine {
public:
    void drive() { start(); }
};

// 组合（推荐）
class CarByComposition {
public:
    void drive() { _engine.start(); }
private:
    Engine _engine;
};
```

两者效果相同，但组合更好：

| | private 继承 | 组合 |
|---|---|---|
| 耦合度 | 高（能访问 protected 成员）| 低（只能用 public 接口）|
| 数量 | 只能有一个同类型基类 | 可以有多个同类型成员 |
| 替换实现 | 要改继承列表 | 换个成员就行 |
| 编译依赖 | 头文件必须包含基类定义 | 可以用前向声明 + 指针 |

private 继承**唯一不可替代的场合**是需要重写基类的虚函数，
或者需要空基类优化（见第七节）。其他情况一律用组合。

这就是"**优先使用组合而非继承**"（Prefer composition over inheritance）这条准则。

## 五、名字隐藏与 using

### 派生类的同名函数会隐藏基类的全部同名重载

这是个容易踩的坑：

```cpp
class BaseOverload {
public:
    void f(int);
    void f(double);
    void f(const char*);
};

class HidesAll : public BaseOverload {
public:
    void f(int);          // 只重新定义了 f(int)
};

HidesAll a;
a.f(1);          // 调自己的
// a.f(2.5);     // 编译错误！基类的 f(double) 被隐藏了
a.BaseOverload::f(2.5);   // 只能显式限定才能调
```

注意这**不是重载**，而是**隐藏（hiding）**。
C++ 的名字查找规则是：先在派生类作用域找，找到名字 `f` 就停止查找，
**不会**再去基类作用域找 —— 哪怕参数不匹配。
然后在找到的候选里做重载决议，于是 `f(2.5)` 只能看到 `f(int)`。

实测：

```
        HidesAll::f(int) —— 隐藏了基类的全部 f 重载
        BaseOverload::f(double)      <- 必须显式限定才能调
```

### using 把基类重载拉回来

```cpp
class KeepsAll : public BaseOverload {
public:
    using BaseOverload::f;     // 把基类的全部 f 重载引入本类作用域
    void f(int);               // 自己的版本覆盖同签名的那个
};

KeepsAll b;
b.f(1);        // 自己的
b.f(2.5);      // 基类的
b.f("text");   // 基类的
```

实测三个都能调。

**设计建议**：派生类要定义与基类同名的函数时，
要么加 `using Base::f;` 保留全部重载，要么换个名字。
隐藏一半重载是最糟的结果 —— 调用方会在某些参数类型上莫名编译失败。

### using 还能改变访问权限

```cpp
class Hidden : private Base {
public:
    using Base::_pub;      // 把 private 继承来的成员重新暴露为 public
    using Base::show;
};

Hidden h;
h._pub = 42;               // ok
h.show();
```

实测可行。注意**只能恢复到基类中的原始权限，不能超过** ——
不能把基类的 private 成员用 using 变成 public。

这个技巧的用途：private 继承后选择性地暴露一部分接口。
不过多数时候用组合 + 转发函数更清楚。

## 六、继承的局限

基类的下列东西**不被继承**：

| 不被继承的 | 说明 |
|---|---|
| **构造函数** | C++11 可用 `using Base::Base;` 显式继承（见 [03-02](../part3_class/03-02_constructor.md)）|
| **析构函数** | 派生类有自己的析构函数，会自动调用基类的 |
| **赋值运算符** | 派生类会生成自己的隐式版本，隐藏基类的 |
| **友元关系** | 友元不是成员。"朋友的孩子不是我的朋友" |

友元不被继承这条值得注意：

```cpp
class Base { friend void f(const Base&); };
class Derived : public Base {};

void f(const Base& b) { /* 可以访问 Base 的私有成员 */ }
// 但 f 不能访问 Derived 新增的私有成员
```

反过来也成立：基类的友元可以通过派生类对象访问基类部分，
但不能访问派生类新增的私有成员。

## 七、空基类优化（EBO）

一个有意思的细节。空类的大小是 1（见 [03-01](../part3_class/03-01_class_basics.md)），
但作为**基类**时不占空间：

```cpp
class EmptyBase {};
class EmptyBase2 {};

struct DerivedFromEmpty : EmptyBase { int _v; };
struct TwoEmptyBases : EmptyBase, EmptyBase2 { int _v; };
struct HasEmptyMember { EmptyBase _b; int _v; };
```

实测：

```
      sizeof(EmptyBase)          = 1
      sizeof(int)                = 4
      sizeof(DerivedFromEmpty)   = 4  <- 空基类不占空间（EBO）
      sizeof(TwoEmptyBases)      = 4  <- 两个空基类也不占
      sizeof(HasEmptyMember)     = 8  <- 作为【成员】则要占空间+对齐
```

**空基类优化（Empty Base Optimization）**：
编译器允许空基类子对象与派生类的第一个成员共享地址，所以不占额外空间。
而作为成员时必须有独立地址，所以 `HasEmptyMember` 是 8 字节
（1 字节的 `_b` + 3 字节填充 + 4 字节的 `_v`）。

**这是 STL 用继承而不是组合来存放 allocator、comparator、deleter 的原因。**

```cpp
// 概念上的实现
template <class T, class Alloc = std::allocator<T>>
class vector : private Alloc {       // 继承而非组合
    T* _begin; T* _end; T* _cap;
};
```

`std::allocator` 是空类，用继承的话 `vector` 就只有三个指针（24 字节）；
用组合会变成 32 字节。同理，`std::unique_ptr<T>` 在用默认删除器时
和裸指针一样大（8 字节），正是因为删除器作为空基类被优化掉了。

C++20 提供了 `[[no_unique_address]]` 属性，让成员也能享受这个优化，
从而可以用组合替代这类继承。

## 八、小结

| 要点 | 说明 |
|---|---|
| 为什么需要 | 抽象的层次性；替代"拷贝修改代码"的重用方式 |
| 定义 | `class D : public B {}`；基类在初始化列表中构造 |
| 访问规则 | 派生类可访问基类 public/protected，不可访问 private |
| 权限降级 | 取"原始权限"和"继承方式"中更严格的；降级会累积传递 |
| 默认继承方式 | `class` 是 private，`struct` 是 public —— **一定要显式写** |
| public 继承 | 表达 **is-a**，支持 Liskov 替换 |
| private 继承 | 表达"用它实现"；**能用组合就用组合** |
| 名字隐藏 | 派生类同名函数隐藏基类**全部**同名重载；用 `using Base::f;` 拉回 |
| `using` 改权限 | 可恢复到原始权限，不能提升超过 |
| 不被继承 | 构造函数（C++11 可 `using`）、析构函数、赋值运算符、友元关系 |
| 空基类优化 | 空基类不占空间（成员会占）；STL 靠它让 allocator/deleter 零开销 |

> 下一章：[04-02 派生类的构造与销毁](04-02_derived_lifecycle.md)
