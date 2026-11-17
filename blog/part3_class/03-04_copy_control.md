# 03-04 拷贝控制与 Rule of Five

> 标准：`= default` / `= delete` `[C++11]`
> demo：[`03_04_copy_control.cpp`](../../demos/part3_class/03_04_copy_control.cpp)

## 一、拷贝构造函数

C++ 中经常用一个变量初始化另一个：

```cpp
int x = 1;
int y = x;
```

我们希望这个操作也能作用于自定义类型：

```cpp
Point pt1(1, 2);
Point pt2 = pt1;
```

执行 `Point pt2 = pt1;` 时，`pt1` 已经存在，而 `pt2` 还不存在 ——
所以这句话**创建了** `pt2`。既然涉及对象创建，就必然调用构造函数，
这里调用的是**拷贝构造函数**。

如果类中没有显式定义，编译器会自动提供一个缺省的拷贝构造函数，其原型是：

```cpp
类名::类名(const 类名&);
```

对 `Point` 来说，缺省版本等价于：

```cpp
Point::Point(const Point& rhs)
    : _ix(rhs._ix)
    , _iy(rhs._iy)
{}
```

逐成员拷贝。对 `Point` 这种只有内置类型成员的类，这已经够了。

### 拷贝构造函数的调用时机

1. **用一个已存在的对象初始化新对象时**

   ```cpp
   Point pt2 = pt1;      // 或 Point pt2(pt1);
   ```
2. **实参和形参都是对象，进行实参与形参结合时**（按值传参）

   ```cpp
   void f(Point p);      // 调用 f(pt) 时拷贝构造 p
   ```
3. **函数的返回值是对象，函数调用完成返回时**

   ```cpp
   Point g() { Point local; return local; }   // 可能拷贝（但通常被 RVO 消除）
   ```

注意第 3 条在现代编译器下通常不会真的发生 ——
拷贝消除（见 [02-03](../part2_memory/02-03_move_semantics.md)）把它优化掉了。

### 为什么参数必须是引用（常考题）

**参数形式可以改变吗？** 不能改成按值传递：

```cpp
Point(Point rhs);     // 编译错误
```

因为按值传参本身就需要调用拷贝构造函数来构造 `rhs` ——
而这会再次调用拷贝构造函数来构造它的参数 —— 无限递归。
编译器直接拒绝这种声明。

参数也应该是 `const` 的，否则 `Point pt2 = make_point();`（右值）就绑不上。

## 二、浅拷贝的灾难

缺省的拷贝构造函数对指针成员做的是**浅拷贝** —— 只拷贝指针的值：

```cpp
class ShallowCopy {
    char* _data;
public:
    explicit ShallowCopy(const char* s) : _data(new char[strlen(s)+1]()) { strcpy(_data, s); }
    ~ShallowCopy() { delete[] _data; }
    // 没有定义拷贝构造函数 -> 编译器生成浅拷贝版本
};

ShallowCopy a("hello");
ShallowCopy b = a;        // a._data == b._data，指向同一块内存
```

离开作用域时，两个对象各自析构，**同一块内存被 `delete` 两次** ——
这就是 **double free**。后果是堆结构损坏，程序崩溃（或者更糟：
不立即崩溃，而是在很远的地方出现诡异错误）。

### 深拷贝

正确做法是分配自己的内存，拷贝内容：

```cpp
DeepCopy(const DeepCopy& rhs)
    : _data(new char[std::strlen(rhs._data) + 1]()) {
    std::strcpy(_data, rhs._data);
}
```

实测确认两个对象的地址不同：

```
      深拷贝构造, 新地址=0x56324f725ee0
      a=hello b=hello c=hello 地址互不相同
```

两个对象拥有各自独立的堆空间，一个销毁时不影响另一个。

## 三、赋值运算符函数

赋值和初始化是两件不同的事：

```cpp
Point pt1(1, 2), pt2(3, 4);
pt1 = pt2;          // 赋值：两个对象都已存在，不涉及构造
Point pt3 = pt1;    // 初始化：pt3 是新创建的，调拷贝构造函数
```

执行 `pt1 = pt2;` 时调用的是**赋值运算符函数**：

```cpp
返回类型 类名::operator=(参数列表) { /* ... */ }
```

没有显式定义时，编译器提供缺省版本，同样是逐成员赋值，
同样对指针成员做浅拷贝，同样会 double free。

### 正确实现的五个要点

```cpp
DeepCopy& operator=(const DeepCopy& rhs) {
    if (this != &rhs) {                               // 1. 自赋值检查
        char* tmp = new char[strlen(rhs._data)+1]();  // 2. 先分配
        std::strcpy(tmp, rhs._data);
        delete[] _data;                                // 3. 再释放旧的
        _data = tmp;                                   // 4. 最后接管
    }
    return *this;                                      // 5. 返回 *this
}
```

**要点 1：自赋值检查。** 如果写成"先 delete 再 new"：

```cpp
DeepCopy& operator=(const DeepCopy& rhs) {
    delete[] _data;                          // 先删
    _data = new char[strlen(rhs._data)+1](); // 若 this == &rhs，
    std::strcpy(_data, rhs._data);           // rhs._data 已是野指针！
    return *this;
}

a = a;     // 读已释放的内存
```

`a = a` 看起来很蠢不会有人写，但 `v[i] = v[j]`、`*p = *q`
在运行时完全可能是同一个对象。

**要点 2-4：先分配，后释放。** 顺序很重要 ——
如果 `new` 抛出 `bad_alloc`，此时旧数据还没被删，对象保持原状态（强异常安全）。
反过来先 `delete` 的话，分配失败就留下一个持有野指针的对象。

**要点 5：返回 `*this`。**

**返回值类型可以改变吗？**（常考题）技术上可以返回 `void` 或者返回值，
但都不应该：

- 返回 `void` → 无法支持链式赋值 `a = b = c`
- 返回值（而非引用）→ 多一次拷贝，而且 `(a = b) = c` 的行为变得诡异

返回 `类名&` 是与内置类型行为一致的唯一选择。

## 四、copy-and-swap：一个函数搞定两种赋值

更优雅的写法，顺带解决了自赋值和异常安全：

```cpp
class CopyAndSwap {
public:
    // 注意：按值传参
    CopyAndSwap& operator=(CopyAndSwap rhs) noexcept {
        swap(*this, rhs);
        return *this;
    }

    friend void swap(CopyAndSwap& a, CopyAndSwap& b) noexcept {
        std::swap(a._data, b._data);
    }
};
```

它的巧妙之处：

- **按值传参**，所以传左值时参数被拷贝构造，传右值时被移动构造 ——
  **一个函数同时实现了拷贝赋值和移动赋值**
- `swap` 之后 `rhs` 持有旧数据，随参数一起析构 —— 不用手工 `delete`
- **自赋值天然安全** —— `rhs` 是独立副本，不需要检查
- **强异常安全** —— 可能失败的拷贝发生在参数构造阶段，
  `swap` 是 `noexcept` 的

实测：

```
      a = b 后 a=bbb                    <- 拷贝赋值
      a = 临时对象 后 a=ccc              <- 移动赋值（同一个函数）
      自赋值后 a=ccc（天然安全）
```

代价是：传左值时一定会发生一次拷贝，即使某些情况下可以复用已有内存。
对于 `std::string` 这样可以复用缓冲区的类型，手写版本可能更快。
实践中 copy-and-swap 是默认选择，有性能需求再特化。

注意 `swap` 定义为**友元函数**（而不是成员函数），
这样 ADL 能找到它（见 [01-01](../part1_basics/01-01_namespace.md)）。

## 五、= default 和 = delete `[C++11]`

### = default：明确要求编译器生成

```cpp
class UseDefault {
public:
    UseDefault() = default;                              // 要求生成默认构造函数
    UseDefault(int v) : _v(v) {}                          // 同时保留其他构造函数
    UseDefault(const UseDefault&) = default;
    UseDefault& operator=(const UseDefault&) = default;
    UseDefault(UseDefault&&) = default;
    UseDefault& operator=(UseDefault&&) = default;
    ~UseDefault() = default;
};
```

它解决两个问题：

1. **定义了其他构造函数后，恢复默认构造函数**。
   C++98 下只能手写一个空的 `UseDefault() {}`，
   但手写的版本和编译器生成的**不等价** ——
   手写的不是 trivial 的，会影响 POD 判定和某些优化
2. **表明意图**。`= default` 明确说"这里用默认行为，我想过了"，
   而什么都不写可能是忘了

### = delete：明确禁止

```cpp
class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;             // 禁止拷贝
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;                  // 但允许移动
    NonCopyable& operator=(NonCopyable&&) = default;
};
```

C++98 的做法是把拷贝构造函数**声明为 private 且不定义**：

```cpp
// C++98 的 boost::noncopyable 手法
class NonCopyable {
private:
    NonCopyable(const NonCopyable&);     // 只声明，不定义
};
```

`= delete` 的三个优势：

1. **错误信息更清楚** —— `use of deleted function` 对比 `is private`
2. **错误在编译期而非链接期** —— private 版本在类的友元和成员里调用时，
   编译能过，链接才失败
3. **可以用于任何函数，不只是特殊成员函数**

第 3 点很实用 —— 可以精确禁止某个重载，防止意外的隐式转换：

```cpp
class OnlyInt {
public:
    void f(int);
    void f(double) = delete;      // 明确禁止传 double
    void f(char) = delete;
};

o.f(42);        // ok
// o.f(3.14);   // 编译错误，而不是静默转成 int
```

这比依赖 `explicit` 更精确。标准库里 `std::unique_ptr`
的拷贝构造函数就是 `= delete` 的。

## 六、五大函数的生成规则（最容易踩的坑）

C++11 之后，特殊成员函数的自动生成规则相互牵连，而且**不会有任何警告**。

| 你声明了 | 后果 |
|---|---|
| **析构函数** | 移动构造/移动赋值**不再自动生成**（拷贝仍生成，但已废弃）|
| **拷贝构造或拷贝赋值** | 移动构造/移动赋值**不再自动生成** |
| **移动构造或移动赋值** | 拷贝构造/拷贝赋值被**定义为删除** |

### 验证：只声明析构函数

```cpp
class OnlyDtor {
public:
    ~OnlyDtor() {}                      // 只加了这一个
    std::vector<int> _data{1, 2, 3};    // 成员本身是可移动的
};
```

实测：

```
        可移动构造: 1（看起来是 true，但实际走的是拷贝！）
        nothrow 可移动构造: 0  <- 0 说明不是真正的移动
```

这是最隐蔽的一点：`is_move_constructible` 返回 **true**，
因为"用右值构造"这件事确实可行 ——
只不过走的是**拷贝构造函数**（它能接受右值）。
真正的判据是 `is_nothrow_move_constructible`，它返回 false。

后果：把这个类放进 `vector`，扩容时每个元素都被深拷贝，
而不是移动。性能损失，而且没有任何提示。
（这正是 [02-03](../part2_memory/02-03_move_semantics.md) 第七节讲的那个问题。）

### 解法：显式声明全部五个

```cpp
class FullySpecified {
public:
    ~FullySpecified() {}
    FullySpecified() = default;
    FullySpecified(const FullySpecified&) = default;
    FullySpecified& operator=(const FullySpecified&) = default;
    FullySpecified(FullySpecified&&) = default;
    FullySpecified& operator=(FullySpecified&&) = default;
};
```

实测 `nothrow 可移动构造: 1` —— 移动能力恢复了。

### 验证：声明移动构造会删除拷贝

```cpp
class HasMoveOnly {
public:
    HasMoveOnly() = default;
    HasMoveOnly(HasMoveOnly&&) = default;
};
```

实测 `可拷贝构造: 0` —— 拷贝被隐式删除了。
这个行为通常是你想要的（移动语义类型大多不该被拷贝），但要知道它会发生。

### 规则总结

**要么一个都不声明（Rule of Zero），要么全部五个都声明（Rule of Five）。
不要只声明一两个。**

## 七、Rule of Three → Rule of Five → Rule of Zero

### Rule of Three（C++98）

> 如果需要自己写**析构函数、拷贝构造函数、拷贝赋值运算符**中的任何一个，
> 通常三个都要写。

逻辑是：需要其中之一，说明这个类在管理资源；
管理资源就意味着三件事都要处理（释放、复制、替换）。

### Rule of Five（C++11）

加上移动构造和移动赋值：

```cpp
class Resource {
public:
    ~Resource();                                  // 1
    Resource(const Resource&);                    // 2
    Resource& operator=(const Resource&);         // 3
    Resource(Resource&&) noexcept;                // 4
    Resource& operator=(Resource&&) noexcept;     // 5
};
```

（用了 copy-and-swap 的话，3 和 5 合并成一个 `operator=(Resource rhs)`，
所以实际是四个。）

### Rule of Zero（推荐）

> **尽量把资源管理委托给已有的 RAII 类型，这样五个函数一个都不用写。**

```cpp
class RuleOfZero {
public:
    RuleOfZero(std::string name, std::vector<int> data)
        : _name(std::move(name)), _data(std::move(data)) {}
    // 五个特殊函数一个都不写
private:
    std::string _name;
    std::vector<int> _data;
};
```

实测编译器生成的全部正确：

```
      可拷贝: 1 可移动: 1 nothrow可移动: 1
```

对比前面的 `ShallowCopy`/`DeepCopy`/`CopyAndSwap` ——
它们之所以需要写那么多代码，根源都在于用了 `char*` 而不是 `std::string`。

**现代 C++ 的首选是 Rule of Zero。** Rule of Five 只在实现底层设施时才需要：
自己写容器、智能指针、句柄包装、内存池。

应用层代码里出现手写的拷贝构造函数，通常是把某个成员换成 RAII 类型就能消掉的。

## 八、小结

| 要点 | 说明 |
|---|---|
| 拷贝构造的时机 | 用已有对象初始化新对象、按值传参、按值返回（常被 RVO 消除）|
| 参数为什么是引用 | 按值传参会无限递归调用自己 |
| 浅拷贝的后果 | 两个对象的指针成员指向同一内存 → double free |
| 赋值运算符五要点 | 自赋值检查、先分配后释放、返回 `*this` |
| 返回类型为什么是引用 | 支持链式赋值 `a = b = c`，与内置类型一致 |
| copy-and-swap | 按值传参 + `noexcept` 的 swap；一个函数搞定拷贝和移动赋值，天然安全 |
| `= default` | 明确要求生成；比手写空函数更优（保持 trivial）|
| `= delete` | 比 private 声明更清楚；可用于任何函数以禁止特定重载 |
| **生成规则** | 声明析构/拷贝 → 移动不生成（退化为拷贝，无警告）；声明移动 → 拷贝被删除 |
| 检测移动能力 | 看 `is_nothrow_move_constructible`，不要看 `is_move_constructible` |
| Rule of Five | 要写一个就写全五个 |
| **Rule of Zero** | 成员都用 RAII 类型，五个都不写（首选）|

> 下一章：[03-05 特殊成员](03-05_special_members.md)
