# 04-06 纯虚函数、抽象类、接口

> demo：[`04_06_abstract.cpp`](../../demos/part4_oop/04_06_abstract.cpp)

## 一、纯虚函数与抽象类

有些基类只是为了定义接口，它自己没有合理的实现。
比如"形状"这个概念 —— 问一个抽象的"形状"面积是多少，没有意义。

C++ 用**纯虚函数**表达"这个函数必须由派生类实现"：

```cpp
class Shape {
public:
    virtual ~Shape() = default;

    virtual double area() const = 0;        // 纯虚函数：= 0 表示没有实现
    virtual std::string name() const = 0;

    // 抽象类也可以有普通成员函数和数据成员
    void describe() const {
        std::cout << name() << " 的面积是 " << area() << "\n";
    }
};
```

**含有纯虚函数的类称为抽象类，不能实例化**：

```cpp
// Shape s;      // 编译错误：cannot declare variable 's' to be of abstract type 'Shape'
```

但可以有指向它的指针和引用 —— 这正是多态的用法：

```cpp
std::vector<std::unique_ptr<Shape>> shapes;
shapes.push_back(std::make_unique<Circle>(1.0));
shapes.push_back(std::make_unique<Rectangle>(2.0, 3.0));
for (const auto& s : shapes) s->describe();
```

实测：

```
        圆 的面积是 3.14159
        矩形 的面积是 6
```

### 部分实现仍然是抽象类

```cpp
class Incomplete : public Shape {
public:
    double area() const override { return 0; }
    // 没实现 name()
};

// Incomplete i;      // 编译错误：仍然是抽象类
```

**只要还有未实现的纯虚函数，派生类就仍然是抽象类。**
这个规则保证了"只有实现了全部接口的类才能被实例化"。

### 抽象类的价值

`describe()` 这个函数说明了抽象类的核心价值：
**它可以用自己还不知道如何实现的函数来编写逻辑**。

```cpp
void describe() const {
    std::cout << name() << " 的面积是 " << area() << "\n";
}
```

`Shape` 不知道怎么算面积，但它知道"打印描述"这件事该怎么组织。
具体的 `area()` 留给派生类 —— 这就是依赖倒置：
高层逻辑依赖抽象接口，而不是具体实现。

## 二、纯虚函数也可以有实现

这是个不太为人所知的细节：**纯虚函数可以有函数体**。

```cpp
class Base {
public:
    virtual void f() = 0;        // 纯虚
};

void Base::f() {                  // 但在类外给了定义
    std::cout << "Base::f() 的默认实现\n";
}

class Derived : public Base {
public:
    void f() override {
        Base::f();               // 用限定名显式调用，复用公共逻辑
        std::cout << "Derived::f() 自己的部分\n";
    }
};
```

实测：

```
        Base::f() 的默认实现（纯虚函数也能有实现）
        Derived::f() 自己的部分
```

用途：**强制派生类覆盖（因为是纯虚的），但同时提供可复用的默认逻辑**。
派生类必须写自己的版本，但可以显式调用基类的实现来复用公共部分。

对比一下三种写法的语义：

| 写法 | 派生类必须覆盖吗 | 有默认实现吗 |
|---|---|---|
| `virtual void f();` | 不必 | 有，直接继承 |
| `virtual void f() = 0;` | **必须** | 没有 |
| `virtual void f() = 0;` + 类外定义 | **必须** | 有，但要显式调用 |

## 三、纯虚析构函数

想让一个类成为抽象类，但它没有任何适合做纯虚的函数时，
可以把析构函数声明为纯虚：

```cpp
class AbstractBase {
public:
    virtual ~AbstractBase() = 0;      // 纯虚析构函数
};

// 必须提供定义！
AbstractBase::~AbstractBase() { /* ... */ }
```

**纯虚析构函数必须提供定义**，这是它和其他纯虚函数的区别 ——
因为派生类的析构函数一定会调用基类的析构函数，没有定义就是链接错误。

实测析构顺序正常：

```
        Concrete::~Concrete()
        AbstractBase::~AbstractBase()
```

这个技巧用得不多。更常见的做法是把构造函数设为 `protected` ——
同样能阻止实例化，而且不需要绕这个弯。

## 四、虚析构函数的必要性

这是本章最重要的一节。

### 不用虚析构函数的后果

```cpp
class BaseNonVirtual {
public:
    ~BaseNonVirtual() { /* ... */ }        // 非虚！
};

class DerivedWithResource : public BaseNonVirtual {
public:
    DerivedWithResource() : _data(new int[100]) {}
    ~DerivedWithResource() { delete[] _data; }
private:
    int* _data;
};

BaseNonVirtual* p = new DerivedWithResource();
delete p;        // 只调用 ~BaseNonVirtual()
```

实测输出：

```
    基类析构函数【非虚】时通过基类指针删除:
        + BaseNonVirtual
        + DerivedWithResource（分配了 400 字节）
        - BaseNonVirtual
      ^ 注意【没有】DerivedWithResource 的析构输出 -> 400 字节泄漏
```

派生类的析构函数**根本没有执行**，`_data` 指向的 400 字节泄漏了。

### 这不只是泄漏，是未定义行为

标准规定：通过基类指针 `delete` 派生类对象，如果基类的析构函数不是虚的，
是**未定义行为** —— 不保证只是泄漏，什么都可能发生。

AddressSanitizer 能检测到它。实测：

```
==3896907==ERROR: AddressSanitizer: new-delete-type-mismatch on 0x502000000010
  object passed to delete has wrong type:
  size of the allocated type:   8 bytes;
  size of the deallocated type: 1 bytes.
```

错误不是"泄漏"，而是 **new-delete-type-mismatch** ——
分配时是 8 字节的 `Derived`，释放时按 1 字节的 `Base` 处理。
`operator delete` 拿到了错误的大小，堆的元数据可能被破坏。

### 编译器会警告（条件是基类有虚函数）

```
warning: deleting object of polymorphic class type 'Base' which has
         non-virtual destructor might cause undefined behavior
         [-Wdelete-non-virtual-dtor]
```

注意这个警告的触发条件是**基类是多态类型**（有其他虚函数）。
如果基类完全没有虚函数，编译器不会警告 ——
因为那种情况下它无法判断你是否打算多态使用。

### 加上 virtual 就正确了

```cpp
class BaseVirtual {
public:
    virtual ~BaseVirtual() { /* ... */ }
};
```

实测：

```
    基类析构函数【虚】时:
        - DerivedVirtual（释放了 400 字节）
        - BaseVirtual
      ^ 两层析构函数都被调用，资源正确释放
```

机制很简单：析构函数也放在虚表里（见 [04-04](04-04_virtual.md)），
`delete p` 通过虚表找到实际类型的析构函数，
执行完后它会自动调用基类的析构函数（见 [03-03](../part3_class/03-03_destructor.md)）。

### 规则

> **如果一个类会被用作基类，并且可能通过基类指针删除派生类对象，
> 它的析构函数必须是 `virtual`。**

实践上更简单的一条：

> **任何有虚函数的类，析构函数都应该是虚的。**

因为有虚函数意味着它是为多态设计的，而多态使用必然涉及基类指针。

反过来，**不打算被继承的类不要加 `virtual`** ——
会多一个虚表指针，让小对象的体积翻几倍（见 [04-04](04-04_virtual.md) 的实测）。
明确不被继承的类可以标 `final`（见 [04-05](04-05_override.md)）。

### 三种正确的做法

```cpp
// 做法一：公有虚析构函数（最常见，允许多态删除）
class Base {
public:
    virtual ~Base() = default;
};

// 做法二：protected 非虚析构函数（禁止多态删除，但允许派生）
class Base {
protected:
    ~Base() = default;          // 外部无法 delete Base*
};

// 做法三：final，表明不被继承
class Concrete final {
public:
    ~Concrete() = default;       // 不需要 virtual
};
```

做法二适用于"接口类，但不希望通过接口指针管理生命周期"的场合 ——
比如用户总是持有具体类型的智能指针，接口只用来调用。

## 五、接口类

**接口类 = 只有纯虚函数 + 虚析构函数 + 没有数据成员的抽象类。**

```cpp
class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& data) = 0;
};

class IComparable {
public:
    virtual ~IComparable() = default;
    virtual int compare_to(const IComparable& other) const = 0;
};

class Document : public ISerializable, public IComparable {
public:
    std::string serialize() const override { /* ... */ }
    void deserialize(const std::string& d) override { /* ... */ }
    int compare_to(const IComparable& other) const override { /* ... */ }
};
```

实测：

```
      d1.serialize() = DOC:hello
      通过 ISerializable* 调用: DOC:hello
      sizeof(Document) = 48（两个接口基类都是空的，只多虚表指针）
```

接口类的三个好处：

1. **多继承安全** —— 没有数据成员就没有菱形问题，不需要虚继承
   （见 [04-03](04-03_diamond.md)）
2. **编译依赖小** —— 头文件里只有函数声明，不需要包含实现相关的头文件
3. **表达"能力"而非"类别"** —— 一个类可以实现多个接口，
   `Document` 既能序列化又能比较

### 接口类的约定写法

```cpp
class IExample {
public:
    virtual ~IExample() = default;          // 1. 虚析构函数

    virtual void required() = 0;            // 2. 全部纯虚

    // 3. 没有数据成员
    // 4. 不提供构造函数（或设为 protected）
    // 5. 禁止拷贝（避免切片）
    IExample(const IExample&) = delete;
    IExample& operator=(const IExample&) = delete;
protected:
    IExample() = default;
};
```

命名上通常用 `I` 前缀（来自 COM/Java 传统）或 `-able` 后缀。
C++ 标准库不用这个约定，但应用代码里它能帮助读者快速识别。

## 六、抽象类的典型用法：模板方法模式

抽象类最常见的设计模式用法：**基类定义流程骨架，派生类只填定制点**。

```cpp
class DataProcessor {
public:
    virtual ~DataProcessor() = default;

    // 非虚的公开接口，定义流程
    void process(const std::string& input) {
        if (!validate(input)) { /* 处理失败 */ return; }
        auto result = transform(input);
        output(result);
    }

protected:
    // 子类定制点，都是 protected（只给派生类用）
    virtual bool validate(const std::string& input) const = 0;
    virtual std::string transform(const std::string& input) const = 0;
};

class UpperCaseProcessor : public DataProcessor {
protected:
    bool validate(const std::string& input) const override { return !input.empty(); }
    std::string transform(const std::string& input) const override { /* 转大写 */ }
};
```

实测：

```
        [1] 校验输入
        [2] 转换数据
        [3] 输出: HELLO WORLD
        [1] 校验输入
        校验失败，中止
```

注意这里的设计细节：

- **公开接口 `process()` 是非虚的** ——
  派生类不能改变整体流程，只能改定制点
- **虚函数是 `protected` 的** —— 它们是给派生类的扩展点，不是公开 API

这叫 **NVI（Non-Virtual Interface）惯用法**，
也是 [04-04](04-04_virtual.md) 提到的"虚函数不要带默认参数"的解法。
它的好处是基类可以在 `process()` 里统一加日志、加锁、做参数检查、
保证前置后置条件 —— 这些是派生类无法绕过的。

## 七、小结

| 概念 | 要点 |
|---|---|
| 纯虚函数 | `virtual void f() = 0;`，声明"必须由派生类实现" |
| 抽象类 | 含纯虚函数的类，**不能实例化**，但可以有指针和引用 |
| 部分实现 | 还有未实现的纯虚函数时，派生类仍是抽象类 |
| 纯虚函数的实现 | 可以有函数体（类外定义），派生类用 `Base::f()` 复用 |
| 纯虚析构函数 | 让类变成抽象类，但**必须提供定义** |
| **虚析构函数** | 基类可能通过基类指针删除派生类对象时，**必须是虚的** |
| 不虚的后果 | 派生类析构函数不执行；标准上是 UB；ASan 报 new-delete-type-mismatch |
| 编译器警告 | `-Wdelete-non-virtual-dtor`（要求基类是多态类型）|
| 规则 | 有虚函数的类，析构函数就应该是虚的；不打算继承的类不要加 virtual |
| 接口类 | 纯虚 + 虚析构 + **无数据成员**；多继承安全，不需要虚继承 |
| 模板方法 / NVI | 公开接口非虚（定义流程），虚函数 protected（定制点）|

> 下一章：[04-07 多态的代价与 CRTP](04-07_crtp.md)
