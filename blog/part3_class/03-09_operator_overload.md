# 03-09 友元与运算符重载

> 标准：C++98（三路比较 `<=>` 是 C++20，仅在对比处提及）
> demo：[`03_09_operator_overload.cpp`](../../demos/part3_class/03_09_operator_overload.cpp)

## 一、友元：有限度地打开封装

`private` 保证了只有类自己能访问内部数据。但有些场合，
**类外的函数确实需要访问私有成员**：

```cpp
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << "(" << p._x << "," << p._y << ")";      // 需要访问 _x、_y
}
```

这个函数**不能是成员函数**（左操作数是 `ostream`，不是 `Point`），
但它又需要读私有成员。`friend` 就是为这类情况设计的 ——
**授权特定的函数或类访问本类的私有成员**。

### 友元函数

```cpp
class Point {
public:
    Point(int x, int y) : _x(x), _y(y) {}

    friend void print_point(const Point& p);                          // 全局函数
    friend std::ostream& operator<<(std::ostream& os, const Point& p); // 运算符

private:
    int _x, _y;
};

void print_point(const Point& p) {
    std::cout << p._x << "," << p._y;        // 可以访问私有成员
}
```

实测正常访问。

**关键认识：友元不是成员函数。**

- 没有 `this` 指针
- 不受 `public`/`private`/`protected` 的位置影响
  （写在类的哪个区域都一样）
- 调用时不用 `对象.函数()`，而是 `函数(对象)`

### 友元类

```cpp
class Record {
public:
    explicit Record(int id) : _id(id) {}
    friend class Database;          // 整个 Database 类都是友元
private:
    int _id;
    std::string _secret;
};

class Database {
public:
    void dump(const Record& r) const {
        use(r._id, r._secret);      // 可以访问 Record 的全部私有成员
    }
};
```

实测 `Database` 能访问 `Record` 的所有私有成员。

### 友元的三个性质

| 性质 | 含义 |
|---|---|
| **不对称** | A 声明 B 为友元，不代表 B 把 A 当友元 |
| **不传递** | "友元的友元不是我的友元" |
| **不继承** | "朋友的孩子不是我的朋友"（见 [04-01](../part4_oop/04-01_inheritance.md)）|

这三条都是刻意设计的 —— 友元是**单向的、精确的**授权，
不会意外扩散。

### 代价与使用建议

友元**破坏封装**：它让类的实现细节暴露给外部代码，
以后改私有成员时要同时检查所有友元。

| 用法 | 评价 |
|---|---|
| 友元函数（单个，为了运算符重载）| **正当**，几乎无法避免 |
| 友元函数（为了方便访问数据）| 可疑 —— 应该加公开接口 |
| **友元类** | **危险** —— 开放了全部私有成员 |

实践建议：

1. **优先用公开接口** —— 如果一个外部函数需要数据，考虑加个 getter
2. **友元只用于运算符重载和紧密耦合的实现类**
   （如 Pimpl 的 `Impl`、迭代器访问容器内部）
3. **避免友元类** —— 它的权限太大。需要时考虑把友元函数列出来，
   而不是整个类

## 二、为什么需要运算符重载

C 里自定义类型只能用函数名操作：

```c
Complex r = complex_add(complex_mul(complex_add(a, b), c), a);
```

这行代码的问题：**读的顺序和计算的顺序相反**，
而且括号嵌套三层后基本不可读。

运算符重载让自定义类型用起来像内置类型：

```cpp
Complex r = (a + b) * c + a;
```

实测两种写法结果相同（`-15+56i`），但可读性差别巨大：

```
        a.add(b).mul(c).add(a) = -15+56i
        (a + b) * c + a        = -15+56i
```

标准库大量依赖它：

| 类型 | 重载的运算符 |
|---|---|
| `std::string` | `+` `+=` `[]` `==` `<` `<<` `>>` |
| `std::vector` | `[]` `==` `<` |
| `std::iostream` | `<<` `>>` `bool` |
| 智能指针 | `*` `->` `bool` `==` |
| 迭代器 | `*` `->` `++` `--` `+` `-` `[]` `==` |

**没有运算符重载，STL 就不可能存在**。

## 三、重载的规则

### 可以重载的

```
算术: +  -  *  /  %  ++  --
关系: ==  !=  <  >  <=  >=
逻辑: &&  ||  !
位:   &  |  ^  ~  <<  >>
赋值: =  +=  -=  *=  /=  %=  &=  |=  ^=  <<=  >>=
其他: []  ()  ->  ->*  ,  new  delete
```

### 不能重载的

| 运算符 | 原因 |
|---|---|
| `::` | 作用域解析是编译期概念，不作用于对象 |
| `.` | 会让 `obj.member` 的含义变得不确定 |
| `.*` | 同上 |
| `?:` | 它的短路语义无法用函数调用表达 |
| `sizeof` / `alignof` / `typeid` | 编译期运算符 |

### 其他限制

1. **不能创造新运算符** —— 不能定义 `**` 做幂运算
2. **不能改变优先级和结合性** —— 重载 `+` 后它仍然低于 `*`
3. **不能改变操作数个数** —— 唯一例外是 `operator()`
4. **至少有一个操作数是自定义类型** —— 不能重载 `int + int`
5. **`=` `[]` `()` `->` 必须是成员函数**

### 一条重要的实践规则

**不要重载 `&&`、`||`、`,`**。

内置的这三个运算符有特殊语义：

```cpp
if (p != nullptr && p->valid())      // && 短路：p 为空时不求值右边
a = f(), g();                         // , 保证从左到右求值
```

**重载后它们退化为普通函数调用，失去短路和顺序保证** ——
所有操作数都会被求值。这会静默地改变程序行为。

（C++17 起 `,` 的内置版本有了明确的求值顺序，但重载版本仍然没有。）

## 四、三种重载形式

### 形式一：成员函数

```cpp
class Money {
public:
    Money operator+(const Money& rhs) const {
        return from_cent(_total_cent + rhs._total_cent);
    }
};
```

左操作数是隐含的 `this`，所以**左操作数必须是本类对象**。

### 形式二：友元函数

```cpp
class Money {
    friend Money operator*(int factor, const Money& m);     // 左操作数是 int！
    friend Money operator*(const Money& m, int factor);
};
```

两个操作数都是显式参数，所以**支持左操作数是其他类型**。

这是友元函数存在的关键理由。实测：

```
      成员函数形式 a + b = 15.75 元
      友元函数形式 3 * a = 31.50 元
      友元函数形式 a * 3 = 31.50 元
      （成员函数做不到 3 * a，因为左操作数必须是 Money）
```

`3 * a` 用成员函数写不出来 —— 那需要给 `int` 加成员函数。

### 形式三：普通（非友元）全局函数

```cpp
bool operator==(const Money& a, const Money& b) { return a.cent() == b.cent(); }
bool operator<(const Money& a, const Money& b)  { return a.cent() < b.cent(); }
```

只用公开接口，不需要友元授权。

### 选择原则

| 运算符 | 推荐形式 | 原因 |
|---|---|---|
| `=` `[]` `()` `->` | **必须**成员函数 | 语言规定 |
| `++` `--` `+=` `-=` 等改变自身的 | 成员函数 | 语义上是"对自己的操作" |
| `+` `-` `*` `/` `==` `<` 等对称的 | **非成员函数** | 保证两侧对称，支持左操作数转换 |
| `<<` `>>` | **必须**非成员 | 左操作数是流 |
| 需要访问私有成员的非成员 | 友元 | — |
| 能用公开接口实现的 | **普通全局函数** | 封装性最好 |

**对称性为什么重要**：

```cpp
class Money {
public:
    Money(long yuan);                      // 允许 int -> Money 隐式转换
    bool operator==(const Money&) const;    // 成员函数版本
};

Money m(10);
m == 10;        // ok：10 隐式转成 Money
10 == m;        // 编译错误！左操作数不能隐式转换以匹配成员函数
```

改成非成员函数后两边都能工作。这就是"**对称的二元运算符应该是非成员函数**"
这条规则的依据。

### 最佳实践：从少数核心运算符派生其余

```cpp
class Money {
public:
    Money& operator+=(const Money& rhs);    // 核心实现
    bool operator==(const Money&) const;     // 核心实现
    bool operator<(const Money&) const;      // 核心实现
};

// 其余由核心派生
Money operator+(Money a, const Money& b) { a += b; return a; }    // 按值传第一个参数
bool operator!=(const Money& a, const Money& b) { return !(a == b); }
bool operator>(const Money& a, const Money& b)  { return b < a; }
bool operator<=(const Money& a, const Money& b) { return !(b < a); }
bool operator>=(const Money& a, const Money& b) { return !(a < b); }
```

注意 `operator+` 的第一个参数**按值传递** ——
这样可以直接复用 `+=`，而且对右值实参能触发移动。

这个模式可以用 CRTP 自动化（见 [04-07](../part4_oop/04-07_crtp.md)），
而 **C++20 的三路比较运算符 `<=>` 让六个比较运算符只需写一个**：

```cpp
// C++20
auto operator<=>(const Money&) const = default;      // 自动生成全部六个
bool operator==(const Money&) const = default;
```

C++17 下仍然要手工写或用 CRTP。

## 五、特殊运算符：自增自减

前置和后置用**哑元参数**区分：

```cpp
class Counter {
public:
    // 前置 ++：返回引用，效率更高
    Counter& operator++() {
        ++_value;
        return *this;
    }

    // 后置 ++：多一个不使用的 int 参数，返回旧值的副本
    Counter operator++(int) {
        Counter old = *this;      // 必须保存旧值
        ++_value;
        return old;                // 返回副本
    }
};
```

实测：

```
      ++c 返回 6，c 现在是 6
      d++ 返回 5，d 现在是 6
```

那个 `int` 参数**从不使用**，纯粹是为了让两个重载有不同签名。
（这是语言的一个历史妥协，没有更好的办法。）

### 性能差别

| | 返回 | 开销 |
|---|---|---|
| 前置 `++i` | 引用 | 无额外开销 |
| 后置 `i++` | **旧值的副本** | 多一次构造 + 一次拷贝 |

**所以循环里优先用 `++it`**：

```cpp
for (auto it = v.begin(); it != v.end(); ++it)      // 推荐
for (auto it = v.begin(); it != v.end(); it++)      // 多一次拷贝
```

对 `int` 这类内置类型编译器会优化掉差别，但对迭代器
（尤其是 `map`/`set` 的节点迭代器）差别是实在的。

## 六、特殊运算符：函数调用

```cpp
class Accumulator {
public:
    int operator()(int x) { _sum += x; return _sum; }     // 可以有状态
private:
    int _sum = 0;
};

Accumulator acc;
acc(10); acc(20);        // 像函数一样调用
```

实测有状态的累加正确工作（110、130、135）。

`operator()` 的特殊之处：

- **唯一可以有任意多个参数的运算符**
- 可以重载多个版本（不同参数列表）
- 必须是成员函数

这个运算符让对象能像函数一样调用 —— 这种对象叫**仿函数**
（functor）或**函数对象**。

### 它是 STL 和 lambda 的基础

```cpp
// 作为比较器传给容器
struct ByLength {
    bool operator()(const std::string& a, const std::string& b) const {
        return a.size() != b.size() ? a.size() < b.size() : a < b;
    }
};
std::map<std::string, int, ByLength> sorted_by_len;
```

实测按长度排序正确。

**lambda 的底层就是一个带 `operator()` 的匿名类**
（见 [06-01](../part6_modern_core/06-01_lambda.md)）。
`mutable` lambda 对应的就是非 const 的 `operator()`。

仿函数相对函数指针的优势：**可以有状态**，而且**可以被内联**
（类型在编译期已知）。这是 `std::sort` 配仿函数比 `qsort`
配函数指针快得多的原因。

## 七、特殊运算符：下标访问

```cpp
class IntArray {
public:
    int& operator[](std::size_t i) { return _data[i]; }              // 非 const
    const int& operator[](std::size_t i) const { return _data[i]; }  // const
};
```

实测：

```
      非 const 对象可写: arr[0] = 10
      const 对象只读:    carr[0] = 10
```

四个要点：

1. **必须是成员函数**
2. **要提供 const 和非 const 两个版本**
   （见 [03-06](03-06_const_member.md)）
3. **返回引用**才能作为赋值目标（`arr[0] = 10`）
4. C++23 之前**只能接受一个参数** ——
   多维访问要用 `operator()(i, j)`

### 是否做边界检查

标准库的做法是提供两套：

```cpp
arr[i];        // 不检查，快
arr.at(i);     // 检查，抛 std::out_of_range
```

实测 `at(100)` 正确抛出异常。
自己写容器时建议模仿这个设计 —— 让调用方选择。

## 八、特殊运算符：成员访问

这是智能指针能工作的核心：

```cpp
template <class T>
class SimplePtr {
public:
    T& operator*() const { return *_ptr; }       // 解引用
    T* operator->() const { return _ptr; }        // 成员访问
    explicit operator bool() const { return _ptr != nullptr; }
private:
    T* _ptr;
};

SimplePtr<Resource> p(new Resource("res-1"));
(*p).use();        // 通过 operator*
p->use();          // 通过 operator->
```

实测两种方式都正确。

### operator-> 的特殊语义

`operator->` 的行为很特别：**它返回一个指针，然后编译器自动继续应用 `->`**。

```cpp
p->use();
// 实际是: (p.operator->())->use()
```

这个"链式解引用"会一直进行，直到遇到真正的指针。
所以可以写出代理的代理：

```cpp
class Proxy {
    SimplePtr<Resource> _inner;
public:
    SimplePtr<Resource>& operator->() { return _inner; }     // 返回另一个有 -> 的对象
};
// proxy->use() 会先调 Proxy::operator->()，再调 SimplePtr::operator->()
```

这个机制让智能指针、迭代器、各种代理类能够**透明地转发成员访问**。

## 九、特殊运算符：流输入输出

```cpp
class Date {
    // 必须是非成员函数：左操作数是 ostream，不是 Date
    friend std::ostream& operator<<(std::ostream& os, const Date& date) {
        return os << date._y << "-" << date._m << "-" << date._d;
    }

    friend std::istream& operator>>(std::istream& is, Date& date) {
        is >> date._y >> date._m >> date._d;
        if (!is) date = Date{};        // 失败时置为已知状态
        return is;
    }
};
```

实测输出 `2024-01-15`，解析 `2025-12-31`，链式调用正常。

五个要点：

1. **必须是非成员函数** —— 左操作数是流
2. **返回 `ostream&` / `istream&`** 才能链式调用 `cout << a << b`
3. 需要访问私有成员时**声明为友元**
4. **直接在类内定义友元最省事** ——
   避免模板类的各种友元声明语法陷阱（见
   [05-02](../part5_template/05-02_class_template.md)）
5. 参数的 const 性：`operator<<` 用 `const T&`，
   `operator>>` 用 `T&`（要修改）

第 5 点还有个细节：**`operator>>` 失败时应该把对象置为已知状态**，
而不是留下半解析的垃圾。C++11 对内置类型的 `>>` 就是这么规定的
（失败时置 0，见 [07-07](../part7_stdlib/07-07_iostream.md)）。

### 为什么要靠 ADL

运算符重载应该和类定义在**同一个命名空间**，
这样 ADL 能找到它（见 [01-01](../part1_basics/01-01_namespace.md)）：

```cpp
namespace geometry {
    struct Vec2 { double x, y; };
    std::ostream& operator<<(std::ostream&, const Vec2&);     // 同一命名空间
}

geometry::Vec2 v;
std::cout << v;          // ADL 找到 geometry::operator<<
```

## 十、小结

### 友元

| 要点 | 说明 |
|---|---|
| 作用 | 授权特定函数/类访问私有成员 |
| 本质 | **不是成员函数**；没有 `this`；不受访问区域影响 |
| 三个性质 | 不对称、不传递、**不继承** |
| 正当用途 | 运算符重载（尤其 `<<` `>>`）、紧密耦合的实现类 |
| 代价 | 破坏封装；**避免友元类**（权限太大）|

### 运算符重载

| 要点 | 说明 |
|---|---|
| 价值 | 让自定义类型像内置类型一样用；STL 的基础 |
| 不能重载 | `::` `.` `.*` `?:` `sizeof` `typeid` |
| 必须是成员 | `=` `[]` `()` `->` |
| 必须是非成员 | `<<` `>>`（左操作数是流）|
| **不要重载** | `&&` `||` `,` —— 会失去短路和顺序语义 |
| 对称运算符 | 用**非成员函数**，保证两侧都能隐式转换 |
| 最佳实践 | 实现少数核心运算符（`+=` `==` `<`），其余由它们派生 |
| 自增自减 | 后置版本用哑元 `int` 区分；前置返回引用更快 |
| `operator()` | 唯一可有任意参数；仿函数/lambda 的基础 |
| `operator[]` | 必须成员；要 const/非 const 成对；返回引用 |
| `operator->` | 返回指针后编译器**自动继续应用 `->`**；智能指针的核心 |
| `operator<<` | 非成员 + 友元 + 返回 `ostream&`；类内定义最省事 |
| C++20 | `<=>` 让六个比较运算符只需写一个 |

> 下一章：[03-10 类型转换](03-10_type_conversion.md)
