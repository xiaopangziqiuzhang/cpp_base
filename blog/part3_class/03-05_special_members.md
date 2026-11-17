# 03-05 特殊成员

> 标准：`static inline` 数据成员 `[C++17]`
> demo：[`03_05_special_members.cpp`](../../demos/part3_class/03_05_special_members.cpp)

类中有四种数据成员的初始化方式与普通成员不同：
**常量成员、引用成员、类对象成员、静态成员**。
前三种的共同点是"必须在初始化列表中初始化"，静态成员则完全独立于对象。

## 一、常量数据成员

用 `const` 修饰的数据成员，一经初始化就具有只读属性。

```cpp
class Point {
public:
    // 错误写法
    Point(int ix, int iy) {
        _ix = ix;      // 编译错误：assignment of read-only member
        _iy = iy;      // 编译错误
    }

    // 正确写法
    Point(int ix, int iy) : _ix(ix), _iy(iy) {}
private:
    const int _ix;
    const int _iy;
};
```

**在构造函数体内初始化 const 数据成员是非法的** ——
因为函数体内的 `=` 是赋值，而 const 成员不能被赋值。
它们只能在构造函数初始化列表中进行初始化。

原因回到 [03-02](03-02_constructor.md) 讲的：
成员在构造函数体执行**之前**就已经完成初始化。
进入函数体时，`_ix` 已经是一个初始化好的 const 对象了，再改它就是违反 const。

### const 成员的代价

加了 const 成员，类就**失去了赋值能力**：

```cpp
class WithConst { const int _v; public: WithConst(int v) : _v(v) {} };

WithConst a(1), b(2);
// a = b;      // 编译错误：拷贝赋值运算符被隐式删除
```

因为赋值需要修改 `_v`，而它是 const 的。
所以这类对象不能放进 `std::vector` 后再排序、不能用 `std::swap`。

实践建议：**成员的 const 不如函数的 const 有价值**。
想表达"这个值构造后不变"，更好的办法通常是只提供 getter 不提供 setter。
真正需要 const 成员的场合不多。

## 二、引用数据成员

和常量成员相同，**引用成员也必须在构造函数初始化列表中初始化**，否则编译报错。

```cpp
class Holder {
public:
    Holder(int& ref) : _ref(ref) {}      // 必须在这里绑定
private:
    int& _ref;
};
```

函数体内的 `_ref = ref;` 不是绑定，而是"给被引用的对象赋值" ——
而此时 `_ref` 还没绑定到任何对象，是未定义行为。

引用成员和 const 成员一样会删除赋值运算符（引用不能重新绑定）。
另外它还有个风险：**被引用的对象可能先于本对象销毁**，留下悬垂引用。

```cpp
class Dangerous {
public:
    Dangerous(const std::string& s) : _s(s) {}    // 只存引用
private:
    const std::string& _s;
};

Dangerous d(std::string("temporary"));   // 临时对象立刻销毁 -> _s 悬垂
```

实践建议：**优先用指针或值成员，而不是引用成员**。
指针表达了"可能为空、可以重新指向"，值成员表达了"我拥有它"，
两者都比引用成员灵活。需要"非拥有的引用"时，考虑 `std::reference_wrapper`
（它可以重新绑定，所以不破坏赋值）。

## 三、类对象成员

当数据成员本身是自定义类类型时，对它的构造必须放在初始化列表中：

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

如果 `Line` 的构造函数没有在初始化列表中初始化 `_pt1`/`_pt2`，
系统会自动调用 `Point` 的**默认构造函数** ——
此时就会与预期的构造不一致。而如果 `Point` 根本没有默认构造函数，直接编译错误。

实测构造顺序：

```
        Inner(100)              <- 成员对象先构造
      const=1 ref=42 obj=100 count=1
```

成员对象在外层对象的构造函数体之前就构造完毕，
顺序由声明顺序决定（见 [03-03](03-03_destructor.md) 的完整顺序表）。

## 四、静态数据成员

`static` 修饰的数据成员，**其实例只有一个，被该类的所有对象共享** ——
就像住在同一宿舍的同学共享一个房间号。

```cpp
class Computer {
public:
    Computer(std::string brand, double price)
        : _brand(std::move(brand)), _price(price) {
        _totalPrice += _price;      // 累加到共享的总价
    }
private:
    std::string _brand;
    double _price;
    static double _totalPrice;      // 这是声明，不是定义
};

double Computer::_totalPrice = 0;    // 必须在类外定义和初始化
```

实测共享效果：

```
      品牌:Huawei MateBook14 价格:5699 总价:5699
      品牌:ThinkPad X1 价格:9999 总价:15698      <- 累加了
```

### 为什么必须在类外定义

因为静态数据成员**不属于类的任何一个对象**，
所以它们并不是在创建类对象时被定义的 ——
这意味着它们不是由类的构造函数初始化的。

一般来说不能在类的内部初始化静态数据成员，
必须在类的外部定义和初始化，格式是：

```cpp
类型 类名::变量名 = 初始化表达式;     // 普通变量
类型 类名::对象名(构造参数);          // 对象变量
```

**注意类外定义时不再写 `static` 关键字。**

静态数据成员在编译时就被创建并初始化（与之相比，对象是运行时创建的），
存储在**全局/静态区**，**并不占据对象的存储空间**。

实测验证：

```
      sizeof(Computer) = 40（只含 string + double，不含 static）
```

`std::string`（32 字节）+ `double`（8 字节）= 40，静态成员不在其中。

### 类外定义容易出的错

这是初学者最常见的链接错误：

```cpp
// header.h
class Foo { static int _count; };

// 忘了在某个 .cpp 里写 int Foo::_count = 0;
// 链接错误: undefined reference to 'Foo::_count'
```

而且这个定义**只能出现在一个** `.cpp` 里，
放在头文件会导致重复定义。这个麻烦正是 C++17 要解决的。

## 五、static inline 数据成员 `[C++17]`

C++17 允许在类内直接定义并初始化静态成员：

```cpp
class ModernStatic {
public:
    static inline int counter = 0;                      // [C++17]
    static inline const std::string kName = "modern";    // [C++17]
    static constexpr int kMax = 100;                     // constexpr 隐式 inline
};
```

实测：

```
      counter=2 kName=modern kMax=100
      [C++17] static inline 成员可以在类内直接初始化
```

不需要类外定义，而且多个翻译单元包含同一头文件也只有一个实体
（原理见 [01-07 的 inline 语义](../part1_basics/01-07_inline.md)
和 [01-02 的 inline 变量](../part1_basics/01-02_const_constexpr.md)）。

这对 **header-only 库**意义重大 —— 以前要么用函数包一层，
要么强迫用户提供一个 `.cpp`。

注意 `static constexpr` 从 C++17 起**隐式就是 inline** 的，
所以它一直不需要类外定义（C++17 之前如果取它的地址才需要）。

## 六、静态成员函数

成员函数也可以是静态的，特点是：

1. **静态成员函数内部不能使用非静态的成员变量和非静态的成员函数**
2. **只能访问静态数据成员和静态成员函数**

```cpp
class Computer {
public:
    static void printTotalPrice() {
        std::cout << _totalPrice;     // ok：静态成员
        // std::cout << _brand;       // 编译错误：没有 this
    }
private:
    std::string _brand;
    static double _totalPrice;
};
```

原因很直接：**静态成员函数的参数列表中不含隐含的 `this` 指针**。
没有 `this`，就无从访问任何属于某个具体对象的东西。

### 调用方式

```cpp
Computer::printTotalPrice();     // 推荐：通过类名调用
pc1.printTotalPrice();           // 也合法，但不推荐
```

通过对象调用是合法的（编译器只是用对象的类型来确定调哪个函数），
但看起来像在操作 `pc1`，有误导性。**推荐一律用类名调用。**

这也解释了 [03-01](03-01_class_basics.md) 里那个空指针调用的例子 ——
`p->test1()` 能工作，因为静态成员函数压根不需要对象。

### 两个硬性限制

```cpp
class StaticLimits {
public:
    static void f() {}
    // static void g() const {}      // 编译错误
    // static virtual void h() {}    // 编译错误
};
```

- **不能是 `const`** —— `const` 修饰的是 `this` 指向的对象，而静态函数没有 `this`
- **不能是 `virtual`** —— 虚调用需要通过对象找到虚表指针，
  而静态函数调用时可能连对象都没有（见 [04-04](../part4_oop/04-04_virtual.md)）

## 七、静态成员的典型用途

### 用途一：实例计数

```cpp
class Counted {
public:
    Counted()  { ++_alive; ++_total; }
    ~Counted() { --_alive; }
    static int alive() { return _alive; }
    static int total() { return _total; }
private:
    static inline int _alive = 0;      // [C++17]
    static inline int _total = 0;
};
```

实测：

```
        存活=3 累计=3
        作用域结束后 存活=0 累计=3
```

用于资源追踪、泄漏检测、性能分析。

### 用途二：共享的常量表

```cpp
class HttpStatus {
public:
    static const char* text(int code);
private:
    static inline const char* kOk = "OK";
    static inline const char* kNotFound = "Not Found";
};
```

比全局常量好 —— 名字限定在类作用域内，不污染命名空间。

### 用途三：编译期常量

```cpp
template <std::size_t N>
class FixedBuffer {
public:
    static constexpr std::size_t kCapacity = N;
private:
    char _data[N]{};
};

static_assert(FixedBuffer<64>::kCapacity == 64, "");
int arr[FixedBuffer<64>::kCapacity]{};      // 可以作数组大小
```

标准库大量使用这个模式：`std::numeric_limits<T>::max()`、
`std::tuple_size<T>::value`、各种 type_traits 的 `::value`。

### 用途四：单例与工厂

见 [03-07 单例模式](03-07_singleton.md)。

## 八、嵌套类型与成员类型别名

类内还可以定义类型 —— 嵌套类、嵌套枚举、类型别名：

```cpp
class Container {
public:
    using value_type = int;           // 成员类型别名
    using size_type = std::size_t;

    class Iterator { /* ... */ };      // 嵌套类

    enum class Mode { Fast, Safe };    // 嵌套枚举
};

Container::value_type v = 42;
Container::Iterator it(&v);
Container::Mode m = Container::Mode::Fast;
```

嵌套类型的作用域是外层类，所以要用 `Container::` 限定访问。

**成员类型别名是泛型编程的基础设施。** 所有标准容器都提供
`value_type`、`size_type`、`iterator`、`const_iterator`、`reference` 等别名，
这样泛型代码才能在不知道具体容器的情况下工作：

```cpp
template <class Container>
typename Container::value_type sum(const Container& c) {
    typename Container::value_type total{};
    for (const auto& x : c) total += x;
    return total;
}
```

（这里的 `typename` 是必需的，原因见
[05-06 模板的工程问题](../part5_template/05-06_template_engineering.md)。）

## 九、小结

| 成员类型 | 初始化方式 | 注意 |
|---|---|---|
| const 成员 | **只能**在初始化列表 | 会删除赋值运算符 |
| 引用成员 | **只能**在初始化列表 | 会删除赋值运算符；有悬垂风险 |
| 类对象成员 | 初始化列表（否则调默认构造）| 无默认构造时必须写 |
| static 成员 | 类外定义（C++17 可用 `static inline` 类内定义）| 不占对象空间，所有对象共享 |

静态成员函数：

| 要点 | 说明 |
|---|---|
| 没有 `this` | 只能访问静态成员 |
| 调用方式 | 推荐 `类名::函数()` |
| 不能是 `const` | 没有 `this` 可修饰 |
| 不能是 `virtual` | 虚调用需要对象 |

实践建议：

- 成员的 const 和引用会删除赋值运算符，用之前想清楚
- 优先用值成员或指针成员，而不是引用成员
- C++17 起静态数据成员用 `static inline`，省掉类外定义
- 编译期常量用 `static constexpr`
- 类型别名（`value_type` 等）是泛型接口的一部分，设计容器时要提供

> 下一章：[03-06 const 成员函数与 mutable](03-06_const_member.md)
