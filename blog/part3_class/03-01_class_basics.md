# 03-01 类的定义与 this

> demo：[`03_01_class_basics.cpp`](../../demos/part3_class/03_01_class_basics.cpp)

## 一、从过程到对象

程序世界本质上只有两种东西：**数据**和**逻辑**。
数据构成程序的状态，逻辑作用于数据，推动程序演进。
两种编程范式的分歧在于这两者的组织方式。

**过程论**认为数据和逻辑是分离的、独立的。所谓程序的运行，
是在逻辑作用下数据被改变的一个过程 —— 有明确的开始、结束、输入、输出，
每个步骤有严格的因果关系。小过程组合成大过程。C 语言是这个范式的代表。

**对象论**认为数据和逻辑不是分离的，而是相互依存的。
相关的数据和逻辑形成个体，这些个体叫对象，程序由一个个对象组成。
对象具有相对独立性，对外提供一定的服务。所谓程序的运行，
是对象之间通过相互调用完成的交互。

两者不是你死我活的对立，而是辩证统一 ——
把对象论中的所有交互提取出来而撇开对象，就变成了过程论；
把过程论中的数据和逻辑分类封装并建立交互关系，就变成了对象论。

实际的差别在于**适用规模**：过程论相对确定，有利于明晰演进方向，
但事物过于庞大繁杂时很难理清思路 —— 过程繁多、过程中又有子过程，
整个程序成了一张纷繁交错的过程网。对象论以对象为基本元素，
即使很庞大的系统也可以分离关注点 —— 研究一个对象时，
只需关心与它相关的少数几个对象，不用总是盯着整个流程。

对象论的困难在于：**如何划分对象才合理？** 这也正是面向对象技术的核心难题。

## 二、类的产生

客观世界里只有对象，没有类。万物皆对象。
但写代码时都要先定义一个类，再通过类创建对象，为什么？

从认识论来说，先有具体认知能力，而后才有抽象认知能力。
抽象认知使我们可以从大量具体认知中，舍弃个别的、非本质的属性，
提取出共同的、本质的属性 —— 这是形成概念的必要手段。

所以**先有对象，然后才有类**。类和对象是"一般和特殊"
这一哲学原理在程序世界中的体现。

类的作用是方便地认识和定义世界中的对象。举个例子：
当今世界有 80 亿人，如果不会抽象思维，每遇到一个人都要重新认知一遍
"这个对象有眼睛、有耳朵、有鼻子有嘴、有胳膊有腿……"。
有了类的概念，只要记住类的数据和逻辑，
对于具体对象只需知道它属于什么"类"，所需区分的只是不同对象的数据有不同的值。

## 三、C++ 中类的定义

类是对现实世界中相似事物的抽象。比如同是"双轮车"的摩托车和自行车，
有共同点也有不同点，"车"类是对摩托车、自行车、汽车等共同点的提取与抽象。

类的定义分为两部分：**数据**（相当于属性）和**对数据的操作**（相当于行为）。

从程序设计的观点看，类就是用户定义的数据类型，
对象是某个类的实例。**类是对象的封装，对象是类的实例**。

```cpp
class 类名 {
public:
    // 公有数据成员和成员函数
protected:
    // 保护数据成员和成员函数
private:
    // 私有数据成员和成员函数
};  // 千万不要忘了这个分号
```

三个访问修饰符的含义：

| 修饰符 | 含义 |
|---|---|
| `public` | 该类对外提供的接口、功能、服务 |
| `protected` | 访问权限开放给它的子类 |
| `private` | 不可以在类之外访问，只能在类内部访问 —— **封装性由此体现** |

例子：

```cpp
class Computer {
public:
    void setBrand(const char* brand) { std::strncpy(_brand, brand, sizeof(_brand) - 1); }
    void setPrice(double price) { _price = price; }
    void print() const { /* ... */ }
private:
    char _brand[32]{};
    double _price = 0;
};

Computer pc;
pc.setBrand("Huawei MateBook14");
pc.setPrice(5699);
// pc._price = 0;     // 编译错误：private
```

### 成员函数可以在类外定义

在类内定义的成员函数**隐式是 inline 函数**。
也可以只在类内声明，在类外定义，这时要用作用域限定符标明所属的类：

```cpp
返回类型 类名::成员函数名(参数列表) { /* ... */ }
```

```cpp
class Point {
public:
    void set(int x, int y);        // 类内声明
    void print() const;
private:
    int _ix = 0;
    int _iy = 0;
};

void Point::set(int x, int y) { _ix = x; _iy = y; }
void Point::print() const { /* ... */ }
```

实践中的分工：头文件放类定义（声明），`.cpp` 放成员函数定义。
小而频繁调用的函数（getter/setter）直接在类内定义，让它隐式 inline。
详见 [01-07 inline](../part1_basics/01-07_inline.md)。

## 四、class 与 struct 的区别

在 C++ 中，`struct` 的功能已经扩展到和 `class` 几乎一样 ——
能定义成员函数、能继承、能有虚函数、能用模板。

**唯一的区别是默认访问权限**：

| | 默认成员访问权限 | 默认继承权限 |
|---|---|---|
| `class` | `private` | `private` |
| `struct` | `public` | `public` |

```cpp
struct AsStruct {
    void method() {}      // 默认 public
    int field = 0;        // 默认 public
};

class AsClass {
    void method() {}      // 默认 private
    int field = 0;        // 默认 private
};
```

实测：

```
      struct 默认 public，s.field = 1
      class 里显式 public 的成员
      继承时: struct 默认 public 继承，class 默认 private 继承
```

### 实践中怎么选

虽然技术上等价，但社区形成了明确的约定：

- **`struct`**：用于"纯数据聚合"—— 所有成员 public、没有不变量需要维护、
  没有虚函数。典型是配置结构、函数返回的多个值、POD 类型
- **`class`**：用于"有封装的类型"—— 有私有状态、有需要维护的不变量、
  通过公有接口对外服务

这个约定有实际意义：看到 `struct` 就知道可以直接访问成员，
看到 `class` 就知道要通过接口。另外 C++17 的结构化绑定
（见 [01-12](../part1_basics/01-12_range_for.md)）只支持全 public 成员的类型，
这也在推动这个区分。

## 五、this 指针

### 一个疑问

通过对象名调用成员函数后，都能准确访问到**对应那个对象**的数据成员，
这是怎么实现的？

```cpp
Counter a(1), b(100);
a.show();     // 输出 1
b.show();     // 输出 100
```

答案是：类中定义的**非静态成员函数都有一个隐含的 `this` 指针**，
它代表当前对象本身，作为成员函数的第一个参数，由编译器自动补全。

```cpp
// 你写的
void Counter::show() const {
    std::cout << _value;
}

// 编译器实际处理的（概念上）
void Counter::show(const Counter* const this) {
    std::cout << this->_value;
}
```

实测两个对象的 `this` 不同：

```
      this=0x7ffe797f3dbc _value=1（等价于 this->_value）
      this=0x7ffe797f3dc0 _value=100（等价于 this->_value）
```

**对于类成员函数而言，并不是一个对象对应一份单独的函数体，
而是此类的所有对象共用这个成员函数体。** 程序编译之后，
成员函数的地址即已确定。成员函数之所以能把属于此类的各个对象的数据区别开，
就是靠这个 `this` 指针。函数体内所有对数据成员的访问，
都会被转化为 `this->数据成员` 的形式。

### this 的类型

| 成员函数 | this 的类型 |
|---|---|
| 普通成员函数 | `T* const this` |
| `const` 成员函数 | `const T* const this` |

注意 `this` 本身永远是 const 的（不能改变它的指向）。
`const` 成员函数改变的是它所指对象的常量性 ——
这就是 const 成员函数"不能修改成员"的全部原因，
并不是什么额外规则。详见 [03-06](03-06_const_member.md)。

### 返回 *this 实现链式调用

```cpp
Counter& add(int n) { _value += n; return *this; }
Counter& mul(int n) { _value *= n; return *this; }

Counter c(1);
c.add(2).mul(10).add(5);     // (1+2)*10+5 = 35
```

实测结果是 35。这个模式是 `operator=`、`operator+=`、`operator<<`
返回引用的原因，也是 builder 模式的基础。
注意必须返回**引用**而不是值，否则每一步都在操作副本。

### 什么时候需要显式写 this

大部分时候不需要，但有几种情况必须：

```cpp
class Widget {
public:
    void set_value(int value) {
        this->value = value;        // 1. 参数名和成员名相同时消歧
    }
    Widget& self() { return *this; } // 2. 需要返回自身
    void f() { g(this); }            // 3. 把自己传给别人
private:
    int value;
};

// 4. 模板继承中访问基类成员（两阶段查找，见 05-06）
template <class T>
class Derived : public Base<T> {
    void f() { this->base_member(); }   // 必须写 this->
};
```

## 六、对象的内存布局

成员函数和静态成员**不占对象的空间**：

```cpp
class Empty {};
class OnlyData { int _a; double _b; };
class DataAndMethods {
public:
    void f1() {} void f2() {} void f3() {}
    static int s_counter;
private:
    int _a; double _b;
};
```

实测：

```
      sizeof(Empty)          = 1（空类也占 1 字节，保证不同对象地址不同）
      sizeof(OnlyData)       = 16
      sizeof(DataAndMethods) = 16（加了 3 个成员函数和 1 个静态成员，大小不变）
```

三个结论：

1. **空类的大小是 1，不是 0**。标准要求不同对象必须有不同地址，
   所以至少要占 1 字节。（空基类优化是例外，见 [04-01](../part4_oop/04-01_inheritance.md)）
2. **成员函数不占对象空间** —— 所有对象共用一份代码
3. **静态成员不占对象空间** —— 它在全局/静态区，见 [03-05](03-05_special_members.md)

`OnlyData` 是 16 而不是 12（4 + 8），因为**内存对齐**：
`double` 要求 8 字节对齐，所以 `int` 后面填了 4 字节。
调整声明顺序可以减小对象大小：

```cpp
struct Bad  { char a; double b; char c; };   // 24 字节
struct Good { double b; char a; char c; };   // 16 字节
```

有虚函数时还会多一个虚表指针（通常 8 字节），见
[04-04 虚函数](../part4_oop/04-04_virtual.md)。

## 七、通过空指针调用成员函数会发生什么

这是个经典的考题。先看代码：

```cpp
class NullPointCall {
public:
    static void test1();          // 静态成员函数
    void test2();                 // 非静态，但不访问成员
    void test3(int v);            // 非静态，只用参数
    void test4();                 // 访问非静态成员
    virtual void test5();         // 虚函数
private:
    static int _static;
    int _member;
};

NullPointCall* p = nullptr;
p->test1();   // ?
p->test2();   // ?
p->test3(10); // ?
p->test4();   // ?
p->test5();   // ?
```

逐个分析：

| 调用 | 结果 | 原因 |
|---|---|---|
| `p->test1()` | **正常** | 静态成员函数没有 `this` 参数，不需要对象。等价于 `NullPointCall::test1()` |
| `p->test2()` | **实践中能跑**，但是 UB | `this` 是 nullptr，但函数体没有解引用它 |
| `p->test3(10)` | 同上 | 同上 |
| `p->test4()` | **崩溃** | 访问 `_member` 要算 `this->_member`，解引用空指针 |
| `p->test5()` | **崩溃** | 虚函数要通过 `this` 找虚表指针，必须解引用 |

实测输出：

```
      test1(): 静态成员函数，不用 this -> 正常
      test1(): 静态成员函数，不用 this -> 正常     <- p->test1() 也能工作
      test2(): 非静态但不访问成员 -> 实践中能跑，但是未定义行为
      test3(10): 只用参数，不访问成员 -> 同上
      （test4/test5 若真的调用会崩溃，已注释）
```

**关键区分**：`test2`/`test3` "能跑"不代表"合法"。
标准规定通过空指针调用非静态成员函数就是未定义行为，
哪怕函数体里没用到 `this`。编译器在 `-O2` 下可能假定 `this != nullptr`
并据此优化，于是行为随优化级别变化。

这个题的实际价值是说明两件事：

1. **成员函数不存在对象里** —— 调用它不需要对象有效，只是传了个 `this`
2. **虚函数调用必须解引用 `this`** —— 这是虚函数不能是静态的原因之一

正确的写法是通过类名调用静态成员函数：

```cpp
NullPointCall::test1();    // 清楚表明不需要对象
```

## 八、小结

| 要点 | 说明 |
|---|---|
| 类的定义 | 数据成员（属性）+ 成员函数（行为），三种访问权限 |
| `private` | 封装性的体现：类外不可访问 |
| 类外定义成员函数 | `返回类型 类名::函数名(...)` |
| 类内定义的成员函数 | 隐式 `inline` |
| `class` vs `struct` | 唯一区别是默认访问权限（成员和继承都是）|
| 约定 | `struct` 用于纯数据聚合，`class` 用于有封装的类型 |
| `this` | 非静态成员函数的隐含第一参数，所有对象共用一份函数代码 |
| `this` 的类型 | 普通成员函数 `T* const`，const 成员函数 `const T* const` |
| `return *this` | 实现链式调用，必须返回引用 |
| 对象大小 | 成员函数和静态成员不占空间；空类占 1 字节；注意内存对齐 |
| 空指针调用 | 静态函数正常；非静态函数是 UB（不访问成员时实践中能跑）；虚函数必崩 |

> 下一章：[03-02 构造函数](03-02_constructor.md)
