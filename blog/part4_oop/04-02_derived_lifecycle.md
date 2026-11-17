# 04-02 派生类的构造与销毁

> demo：[`04_02_derived_lifecycle.cpp`](../../demos/part4_oop/04_02_derived_lifecycle.cpp)

## 一、构造与析构的完整顺序

派生类对象的构造分四步，析构严格逆序。实测输出：

```
    创建 Derived 对象:
        + 成员 Base::_m            <- 1. 基类的成员
      [Base 构造函数体]            <- 2. 基类的构造函数体
        + 成员 Derived::_m1        <- 3. 派生类的成员（按声明顺序）
        + 成员 Derived::_m2
      [Derived 构造函数体]         <- 4. 派生类的构造函数体

    ---- 即将销毁 ----
      [Derived 析构函数体]         <- 1. 派生类的析构函数体
        - 成员 Derived::_m2        <- 2. 派生类的成员（逆序）
        - 成员 Derived::_m1
      [Base 析构函数体]            <- 3. 基类的析构函数体
        - 成员 Base::_m            <- 4. 基类的成员
```

归纳成规则：

**构造：基类（含其成员）→ 自己的成员（声明顺序）→ 自己的构造函数体**
**析构：自己的构造函数体 → 自己的成员（逆序）→ 基类**

这个顺序不是随意规定的，它保证了两件事：

- 进入构造函数体时，基类和所有成员都已完整构造 —— 可以安全使用它们
- 执行析构函数体时，基类和所有成员都还没销毁 —— 可以安全使用它们

成员的顺序由**声明顺序**决定（见 [03-02](../part3_class/03-02_constructor.md)
的初始化顺序陷阱），这也是析构能够有唯一确定逆序的前提。

## 二、派生类必须构造基类

基类的构造必须在派生类的**初始化列表**中指定：

```cpp
class BaseWithArgs {
public:
    explicit BaseWithArgs(int v) : _v(v) {}
    // 没有默认构造函数
};

class DerivedCorrect : public BaseWithArgs {
public:
    explicit DerivedCorrect(int v) : BaseWithArgs(v) {}   // 必须显式构造
};

class DerivedWrong : public BaseWithArgs {
public:
    DerivedWrong(int v) { }    // 编译错误：
                               // no matching function for call to 'BaseWithArgs()'
};
```

如果不写，编译器会尝试调用基类的**默认构造函数** ——
基类没有默认构造函数就直接编译失败。

即使基类有默认构造函数，不写也往往是 bug：

```cpp
class Derived : public Base {
public:
    Derived(int v) {        // 悄悄调用了 Base()，v 没传给基类
        // 想把 v 传给基类？这里已经晚了，基类已经构造完毕
    }
};
```

基类的构造发生在派生类构造函数体**之前**，所以在函数体里无法影响它。

C++11 的继承构造函数（`using Base::Base;`）可以省掉一批转发代码，
见 [03-02](../part3_class/03-02_constructor.md)。

## 三、多基继承

C++ 允许一个类从多个基类派生：

```cpp
class Duck : public Flyable, public Swimmable {
    // ...
};
```

**基类的构造顺序由派生列表中的书写顺序决定**，析构顺序相反。实测：

```
        + Flyable        <- 派生列表里 Flyable 在前
        + Swimmable
        + Duck
        ...
        - Duck
        - Swimmable      <- 逆序
        - Flyable
```

注意这和成员的规则不同：成员看**声明顺序**，基类看**派生列表顺序**。

多基继承带来的问题（名字冲突、菱形继承）在
[04-03](04-03_diamond.md) 专门讨论。这里只说一条实践建议：

**多基继承时，让最多一个基类有数据和实现，其余的只做纯接口。**
这是 Java/C# 只允许单继承 + 多接口的设计依据，
在 C++ 里也是推荐做法 —— 能避开绝大多数多继承的麻烦。

## 四、基类与派生类之间的转换

### 向上转换（派生 → 基类）：隐式、安全

```cpp
Circle c;
Shape* ps = &c;      // 指针，隐式转换
Shape& rs = c;       // 引用，隐式转换
```

安全的原因很直观：派生类对象里**确实包含**一个完整的基类子对象。
把指针/引用当成基类来用，访问的是真实存在的那部分。

这正是多态的基础 —— 虚函数调用会分发到实际类型：

```cpp
ps->draw();          // 调用 Circle::draw()
```

实测两次都输出 `Circle::draw()`。

### 向下转换（基类 → 派生）：需要显式，且要检查

```cpp
if (Circle* pc = dynamic_cast<Circle*>(ps)) {
    pc->circle_only();
}
```

因为基类指针可能指向任何派生类（或基类本身），
编译器无法保证转换的目标类型是对的。
`dynamic_cast` 在运行期检查，失败返回 `nullptr`。
细节见 [01-04 类型转换](../part1_basics/01-04_cast.md)。

## 五、对象切片（object slicing）

这是把多态对象**按值**传递时的后果，是 C++ 特有的一类 bug。

```cpp
void by_value(Shape s)            { s.draw(); }   // 按值
void by_reference(const Shape& s) { s.draw(); }   // 按引用

Circle c;
by_value(c);         // 输出 Shape::draw()   <- 切片了！
by_reference(c);     // 输出 Circle::draw()  <- 正确
```

实测：

```
        按值传基类:   Shape::draw()
        按引用传基类: Circle::draw()
```

原因：按值传参要构造一个 `Shape` 类型的副本，
调用的是 `Shape` 的拷贝构造函数，**只拷贝 `Shape` 那部分**，
派生类新增的数据被"切掉"了。副本的实际类型就是 `Shape`，
虚函数自然分发到 `Shape::draw()`。

### 容器里存基类对象同样会切片

```cpp
std::vector<Shape> shapes;
shapes.push_back(c);        // 切片！存进去的是 Shape
shapes[0].draw();           // Shape::draw()
```

实测确认。这是初学者最常犯的错误之一 ——
以为 `vector<Shape>` 能存各种形状，实际上存进去的全是被切片的 `Shape`。

**正确做法：容器存指针**

```cpp
std::vector<std::unique_ptr<Shape>> ptrs;
ptrs.push_back(std::make_unique<Circle>());
ptrs[0]->draw();            // Circle::draw()
```

实测输出 `Circle::draw()`，多态生效。

### 如何预防切片

1. **多态类型一律通过指针或引用传递**，函数参数用 `const Base&`
2. **容器存智能指针**（`vector<unique_ptr<Base>>`），不存对象
3. **把基类的拷贝构造函数设为 protected 或 delete**，
   让切片在编译期就失败：

   ```cpp
   class Shape {
   protected:
       Shape(const Shape&) = default;      // 只允许派生类拷贝基类部分
   public:
       virtual ~Shape() = default;
   };
   ```
4. 抽象基类（含纯虚函数）天然不能实例化，所以不会被按值传递 ——
   这是用抽象基类的额外好处（见 [04-06](04-06_abstract.md)）

## 六、派生类的复制控制

派生类的拷贝构造函数和拷贝赋值运算符**必须显式处理基类部分**。

### 正确写法

```cpp
class DerivedCopy : public BaseCopy {
public:
    DerivedCopy(const DerivedCopy& rhs)
        : BaseCopy(rhs)              // 显式调用基类的拷贝构造
        , _d(rhs._d) {}

    DerivedCopy& operator=(const DerivedCopy& rhs) {
        if (this != &rhs) {
            BaseCopy::operator=(rhs);    // 显式调用基类的赋值运算符
            _d = rhs._d;
        }
        return *this;
    }
private:
    int _d;
};
```

实测输出，基类的版本被正确调用：

```
    正确的派生类拷贝:
        BaseCopy 拷贝构造
        DerivedCopy 拷贝构造
        BaseCopy 拷贝赋值
        DerivedCopy 拷贝赋值
```

注意两个写法细节：

- 拷贝构造里是 `BaseCopy(rhs)` ——
  `rhs` 是 `DerivedCopy&`，会隐式转成 `const BaseCopy&`（向上转换）
- 赋值里是 `BaseCopy::operator=(rhs)` ——
  必须用限定名显式调用，否则会递归调用自己

### 忘记调用基类版本的后果

```cpp
class DerivedCopyWrong : public BaseCopy {
public:
    DerivedCopyWrong(const DerivedCopyWrong& rhs) : _d(rhs._d) {
        // 没写 BaseCopy(rhs)
    }
};
```

实测：

```
    忘记调用基类拷贝构造:
        DerivedCopyWrong 拷贝构造（基类部分丢了！）
      注意上面【没有】BaseCopy 拷贝构造的输出
```

基类部分走了**默认构造函数**，基类的数据没有被拷贝 ——
对象处于"一半是源对象的拷贝，一半是默认值"的状态。
这个 bug 不会报错，只会导致数据莫名丢失。

### 更好的办法：让编译器生成

```cpp
class Derived : public Base {
public:
    Derived(const Derived&) = default;              // 自动正确处理基类
    Derived& operator=(const Derived&) = default;
private:
    std::string _d;        // RAII 成员
};
```

编译器生成的版本**一定会**正确调用基类的对应函数。
这又回到了 Rule of Zero（见 [03-04](../part3_class/03-04_copy_control.md)）——
能不手写就不手写。

只有在基类或成员需要特殊处理时才手写，而这时一定要记得第一行写基类。

## 七、多态基类的拷贝问题

多态类型的拷贝本身是个设计难题：

```cpp
Shape* p1 = new Circle;
Shape* p2 = new Square;
*p1 = *p2;        // 调用 Shape::operator=，只赋值了 Shape 部分
                  // p1 仍然是 Circle，但数据来自 Square 的基类部分 —— 无意义
```

这叫**部分赋值（partial assignment）**。标准做法有两种：

**一、禁止拷贝，提供 clone 虚函数**

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual std::unique_ptr<Shape> clone() const = 0;   // 虚拷贝
protected:
    Shape(const Shape&) = default;        // 只允许派生类用
    Shape& operator=(const Shape&) = default;
};

class Circle : public Shape {
public:
    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Circle>(*this);
    }
};
```

这是"**虚构造函数**"惯用法 —— C++ 的构造函数不能是虚函数，
但可以用虚函数返回新对象来达到同样效果。

**二、彻底禁止拷贝，只通过指针操作**

```cpp
class Shape {
public:
    Shape(const Shape&) = delete;
    Shape& operator=(const Shape&) = delete;
    virtual ~Shape() = default;
};
```

两种做法都避开了部分赋值和切片。
选哪个取决于你是否需要"复制一个多态对象"这个操作。

## 八、小结

| 要点 | 说明 |
|---|---|
| 构造顺序 | 基类（含其成员）→ 自己的成员（声明顺序）→ 自己的函数体 |
| 析构顺序 | 自己的函数体 → 自己的成员（逆序）→ 基类 |
| 基类的构造 | 必须在派生类**初始化列表**中指定；函数体里已经来不及 |
| 多基继承顺序 | 构造按**派生列表顺序**，析构相反 |
| 向上转换 | 隐式、安全，是多态的基础 |
| 向下转换 | 需要 `dynamic_cast` 并检查结果 |
| **对象切片** | 按值传递多态对象会丢掉派生部分，虚函数退化到基类版本 |
| 预防切片 | 用 `const Base&` 传参；容器存智能指针；基类拷贝设为 protected |
| 派生类拷贝 | 必须显式写 `Base(rhs)` 和 `Base::operator=(rhs)` |
| 更好的办法 | `= default` 让编译器生成，它一定会正确处理基类 |
| 多态拷贝 | 用 `clone()` 虚函数，或彻底禁止拷贝 |

> 下一章：[04-03 名字冲突与菱形继承](04-03_diamond.md)
