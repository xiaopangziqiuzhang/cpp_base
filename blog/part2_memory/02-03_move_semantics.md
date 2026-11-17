# 02-03 右值引用与移动语义

> 标准：`[C++11]`
> demo：[`02_03_move_semantics.cpp`](../../demos/part2_memory/02_03_move_semantics.cpp)

这是 C++11 最重要的特性。不理解它，就读不懂现代 C++ 代码，
也无法解释标准库为什么长成现在这样。

## 一、解决了什么问题

考虑一个管理堆内存的类（简化版 string）：

```cpp
class MyString {
    std::size_t _size;
    char* _data;        // 指向堆上的字符数组
};
```

C++98 下，把一个对象交给另一个对象，只有**拷贝**一条路：

```cpp
MyString a("hello");
MyString b = a;        // 拷贝构造：分配新内存 + 复制全部字符
```

这是对的 —— `a` 后面还要用，`b` 必须有自己的副本。

但有一大类场景，源对象**马上就要消失**，拷贝纯属浪费：

```cpp
MyString make() {
    MyString local("result");
    return local;             // 返回时拷贝一次，然后 local 立刻析构
}

MyString s = make();          // 又拷贝一次

std::vector<MyString> v;
v.push_back(MyString("tmp")); // 构造临时对象，拷贝进容器，临时对象析构
```

每一次"拷贝完就销毁源对象"都意味着：
**刚分配的内存立刻被释放，刚复制的数据立刻被丢弃**。
源对象的资源本来可以直接拿来用。

C++98 时代为了绕过这个问题，社区发明了一堆变通手段：

```cpp
// 办法一：传出参，接口变丑
void make(MyString& out);

// 办法二：返回裸指针，所有权不清
MyString* make();

// 办法三：写 swap 做"伪移动"
MyString tmp; tmp.swap(local);

// 办法四：引用计数 / COW（见 07-01），线程安全和性能都有代价
```

C++11 的答案是：既然源对象即将消亡，那就**允许"偷"走它的资源** ——
这就是移动语义。实现它需要一个前提：**在语言层面能够区分
"这个对象还要用"和"这个对象马上就没了"**。

于是有了右值引用。

## 二、值类别：左值与右值

先把概念理清。最朴素的判据：

- **左值（lvalue）**：有名字、可以取地址的表达式。变量、解引用的指针、返回左值引用的函数调用
- **右值（rvalue）**：临时的、即将销毁的表达式。字面量、算术表达式的结果、
  返回值不是引用的函数调用、临时对象

```cpp
int x = 10;

x          // 左值：有名字，可取地址
10         // 右值：字面量
x + 1      // 右值：临时结果
f()        // 右值（如果 f 返回值而非引用）
MyString() // 右值：临时对象
```

C++11 把右值细分成两种：

- **纯右值（prvalue）**：字面量、算术表达式结果、返回非引用的函数调用。没有身份
- **将亡值（xvalue）**：有身份但资源可以被窃取的对象 —— 主要来自 `std::move`
  和返回右值引用的函数

```
        表达式
       /      \
   glvalue    rvalue
   /    \     /    \
lvalue   xvalue    prvalue
```

实践中记住这条就够了：**有名字的是左值，没名字的是右值**。
（这也解释了一个常见困惑：`void f(MyString&& s)` 里的 `s` 虽然类型是右值引用，
但它**有名字，所以是左值** —— 这正是需要 `std::forward` 的原因，见 [02-04](02-04_perfect_forwarding.md)。）

### 右值引用

C++98 的引用（现在叫左值引用）用 `&`，只能绑定左值（`const&` 例外）。
C++11 加了用 `&&` 表示的右值引用，**只能绑定右值**：

```cpp
int& lref = x;              // ok：左值引用绑左值
// int& bad = 10;           // 错误
const int& cref = 10;       // ok：const 左值引用可以绑右值

int&& rref = 10;            // ok：右值引用绑右值
// int&& bad2 = x;          // 错误：不能绑左值
int&& rref2 = std::move(x); // ok：std::move 把左值转成右值
```

右值引用的作用，就是让函数能够**重载出一个"参数是临时对象"的版本**：

```cpp
void f(const MyString& s);  // 参数是左值 -> 必须保留它，只能拷贝
void f(MyString&& s);       // 参数是右值 -> 它马上要消失，可以偷
```

## 三、移动构造函数与移动赋值运算符

有了右值引用，就能写出"偷资源"的构造函数：

```cpp
class MyString {
public:
    // 拷贝构造：分配 + 复制，开销 O(n)
    MyString(const MyString& rhs)
        : _size(rhs._size), _data(new char[rhs._size + 1]) {
        std::strcpy(_data, rhs._data);
    }

    // 移动构造：接管指针，开销 O(1)
    MyString(MyString&& rhs) noexcept
        : _size(rhs._size), _data(rhs._data) {
        rhs._data = nullptr;      // 关键：让源对象放弃所有权
        rhs._size = 0;
    }

    // 拷贝赋值
    MyString& operator=(const MyString& rhs) {
        if (this != &rhs) {
            char* tmp = new char[rhs._size + 1];   // 先分配，失败也不破坏原状态
            std::strcpy(tmp, rhs._data);
            delete[] _data;
            _data = tmp;
            _size = rhs._size;
        }
        return *this;
    }

    // 移动赋值
    MyString& operator=(MyString&& rhs) noexcept {
        if (this != &rhs) {
            delete[] _data;         // 先释放自己的
            _data = rhs._data;      // 接管对方的
            _size = rhs._size;
            rhs._data = nullptr;    // 对方放弃
            rhs._size = 0;
        }
        return *this;
    }

    ~MyString() { delete[] _data; }    // 对 nullptr 调 delete 是安全的
};
```

移动构造函数的三个要点：

1. **接管资源**（复制指针，不复制内容）
2. **把源对象置为安全的可析构状态**（这一步不能漏，否则 double free）
3. **标记 `noexcept`**（原因见第七节）

实测各种初始化走哪个构造函数：

```
  用左值初始化 -> 拷贝构造:
    拷贝构造 "lvalue"（分配 7 字节）
  用右值(临时对象)初始化 -> 移动构造:
    构造 "temporary"
  用 std::move(左值) -> 移动构造:
    移动构造 "other"（只改指针）
  b 被移动后，c_str() = ""（空了）
```

## 四、std::move 本身不移动任何东西

这是最常见的误解。`std::move` 的实现大致是：

```cpp
template <class T>
constexpr std::remove_reference_t<T>&& move(T&& t) noexcept {
    return static_cast<std::remove_reference_t<T>&&>(t);
}
```

**它只是一个 `static_cast`** —— 把表达式的值类别从左值变成右值，
不产生任何运行时代码。

实测证明：

```cpp
MyString a("original");
MyString&& ref = std::move(a);     // 这一行什么也没发生
// 输出: a 仍然是 "original"

MyString b = std::move(ref);        // 这里才真正移动
// 输出: 移动构造 "original"（只改指针）
//      现在 a 是 ""，b 是 "original"
```

所以 `std::move` 更准确的名字应该是 `rvalue_cast`。
它的作用是**告诉编译器"我不再需要这个对象了，请选择移动版本的重载"**。
真正的移动发生在被调用的移动构造函数/移动赋值运算符里。

推论：对没有移动构造函数的类型用 `std::move`，会静默退回到拷贝 ——
不会报错，只是没有收益。

## 五、被移动之后的对象是什么状态

标准的规定是：**有效但未指定（valid but unspecified）**。

- **有效**：可以安全地析构、可以安全地赋新值、可以调用任何不需要前置条件的操作
- **未指定**：不要假设具体内容是什么

```cpp
std::string s = "hello world, long enough to heap-allocate";
std::string t = std::move(s);

s.size();          // 实测是 0，但标准不保证
s = "reassigned";  // 合法，这是推荐的做法
s.front();         // 危险！如果 s 为空，这是未定义行为
```

实测输出：

```
    size() = 0（标准只保证对象仍有效）
    可以安全地重新赋值:
    s = "reassigned"
```

libstdc++ 的实现碰巧把被移动的 string 置空了，但**不要依赖这个**。
实践规则：**被移动的对象，除了重新赋值或销毁，不要再碰它。**

## 六、容器如何受益

### 扩容时搬迁元素

`vector` 扩容要把元素搬到新内存。有了移动语义，这个搬迁从 O(n) 次深拷贝
变成 O(n) 次指针转移。实测 `push_back` 触发扩容时的输出：

```
    构造 "third"
    移动构造 "third"（只改指针）
    移动构造 "first"（只改指针）      <- 搬迁已有元素
    析构（已被移动，无资源）
    移动构造 "second"（只改指针）
    析构（已被移动，无资源）
```

每个已有元素被移动到新内存，源对象变成"无资源"状态后析构 —— 全程没有一次字符复制。

### push_back vs emplace_back

```cpp
v.push_back(MyString("a"));   // 构造临时对象 -> 移动进容器 -> 临时对象析构
v.emplace_back("b");          // 直接在容器内构造，没有临时对象
```

实测输出：

```
  push_back(MyString("a")): 构造临时对象 + 移动
    构造 "a"
    移动构造 "a"（只改指针）
    析构（已被移动，无资源）
  emplace_back("b"): 直接在容器内构造，无临时对象
    构造 "b"
```

`emplace_back` 把参数**完美转发**给元素的构造函数，连移动都省了。
原理见 [02-04 完美转发](02-04_perfect_forwarding.md)。

### 性能实测

把 20 万个 1000 字节的 string 放进 vector：

```
    push_back(s)            拷贝: 73758 us
    push_back(std::move(s)) 移动: 8299 us
```

**移动比拷贝快约 8.9 倍**。这个差距随对象大小增长 ——
拷贝是 O(n)，移动是 O(1)。

## 七、为什么移动函数必须标 noexcept

这是实践中最容易漏、后果最具体的一条。

`vector` 扩容时如果搬到一半抛异常，会处于"新旧内存都有部分元素"的破碎状态，
无法回滚（源对象已经被掏空了）。为了保证强异常安全，
**`vector` 只在元素的移动构造函数是 `noexcept` 时才使用移动，否则退回拷贝**。

实测对比（两个类唯一的区别就是移动构造函数有没有 `noexcept`）：

```
  是否 nothrow 可移动构造:
    WithoutNoexcept: 0
    WithNoexcept:    1
  反复 push_back 触发扩容搬迁 30000 个元素:
    不带 noexcept（扩容时只能拷贝）: 17054 us
    带 noexcept  （扩容时可以移动）: 10805 us
```

**少写一个 `noexcept`，性能损失约 1.6 倍** —— 而代码看起来完全正常。

写这个 demo 时还踩到一个相关的细节：如果用 `= default` 生成移动构造函数，
编译器会根据成员**自动推导** `noexcept`（成员都是 nothrow 可移动的话，
生成的函数就是 `noexcept`）。所以

```cpp
MyClass(MyClass&&) = default;          // 通常已经是 noexcept 的
MyClass(MyClass&& rhs) : _m(std::move(rhs._m)) {}   // 手写的话，必须自己加 noexcept
```

**只要手写移动函数，就要显式加 `noexcept`。**

## 八、Rule of Five（五法则）

C++98 有个 **Rule of Three**：如果你需要自己写析构函数、拷贝构造函数、
拷贝赋值运算符中的任何一个，通常三个都要写。
因为需要其中之一，说明类在管理资源。

C++11 加入移动语义后变成 **Rule of Five**：

```cpp
class Resource {
public:
    ~Resource();                                  // 1. 析构函数
    Resource(const Resource&);                    // 2. 拷贝构造
    Resource& operator=(const Resource&);         // 3. 拷贝赋值
    Resource(Resource&&) noexcept;                // 4. 移动构造
    Resource& operator=(Resource&&) noexcept;     // 5. 移动赋值
};
```

还有一条更重要的 **Rule of Zero**：

> 尽量把资源管理委托给已有的 RAII 类型，这样五个函数一个都不用写。

```cpp
// 需要写五个函数
class Bad {
    char* _data;        // 裸指针 -> 必须自己管
};

// 一个都不用写，编译器生成的全都正确
class Good {
    std::string _data;          // 或 vector / unique_ptr
};
```

**现代 C++ 的首选是 Rule of Zero。** 只有在实现底层设施
（自己写容器、智能指针、句柄包装）时才需要 Rule of Five。

### 生成规则（容易踩坑）

编译器自动生成特殊成员函数的规则相互牵连：

| 你声明了 | 后果 |
|---|---|
| 析构函数 | 移动构造/移动赋值**不再自动生成**（拷贝仍生成，但已废弃）|
| 拷贝构造或拷贝赋值 | 移动构造/移动赋值**不再自动生成** |
| 移动构造或移动赋值 | 拷贝构造/拷贝赋值被**定义为删除** |

最隐蔽的坑是第一条：

```cpp
class Logger {
public:
    ~Logger() { flush(); }     // 只是为了写日志，加了个析构函数
    std::vector<std::string> _lines;   // 成员本身是可移动的
};

std::vector<Logger> v;
v.push_back(Logger{});          // 因为声明了析构函数，移动构造没生成 -> 走拷贝
```

加一个析构函数，整个类就失去了移动能力，而且**没有任何警告**。
解决办法是显式声明全部五个：

```cpp
~Logger() { flush(); }
Logger(const Logger&) = default;
Logger& operator=(const Logger&) = default;
Logger(Logger&&) = default;
Logger& operator=(Logger&&) = default;
```

详见 [03-04 拷贝控制与 Rule of Five](../part3_class/03-04_copy_control.md)。

## 九、拷贝消除：有时连移动都不需要

```cpp
MyString make() {
    MyString local("returned");
    return local;
}
MyString s = make();
```

实测输出只有一次构造：

```
    构造 "returned"
    析构 "returned"
```

既没有拷贝也没有移动。这是**拷贝消除（copy elision）**：

- **RVO（返回值优化）**：返回临时对象时，直接在调用方的空间上构造
- **NRVO（命名返回值优化）**：返回具名局部对象时，同样可以消除

C++17 起，**纯右值的拷贝消除是强制的**（不再是可选优化）：

```cpp
MyString s = MyString("x");     // C++17 保证没有任何拷贝/移动
```

NRVO 仍然是可选的，但主流编译器都会做。想观察被消除掉的操作，用
`-fno-elide-constructors` 编译。

### 推论：不要写 `return std::move(local);`

```cpp
MyString bad() {
    MyString local("x");
    return std::move(local);    // 反而更慢！
}
```

`std::move` 把返回表达式变成了右值引用，**阻止了 NRVO** ——
原本零开销，现在强制变成一次移动构造。

规则：**直接 `return local;`**。编译器会优先尝试 NRVO，
失败时自动当作右值处理（C++11 起的规定），无需你手工 `move`。

## 十、什么时候该用 std::move

**该用：**

```cpp
// 1. 把参数存进成员，且参数是按值传递的
class Widget {
public:
    explicit Widget(std::string name) : _name(std::move(name)) {}
private:
    std::string _name;
};

// 2. 把不再需要的局部对象交给别人
auto data = load();
process(std::move(data));     // 之后不再用 data

// 3. 容器之间转移元素
dst.push_back(std::move(src[i]));

// 4. 实现移动构造函数时，移动各个成员
MyClass(MyClass&& rhs) noexcept : _member(std::move(rhs._member)) {}
```

**不该用：**

```cpp
// 1. 返回局部对象（阻止 NRVO）
return std::move(local);              // 不要

// 2. 之后还要用这个对象
process(std::move(data));
use(data);                            // 危险：data 已被掏空

// 3. 对 const 对象（移动不了，静默退回拷贝）
const std::string s = "x";
auto t = std::move(s);                // 实际是拷贝

// 4. 对基本类型（没有收益，移动等于拷贝）
int x = 1;
int y = std::move(x);                 // 无意义
```

## 十一、小结

| 概念 | 要点 |
|---|---|
| 解决什么 | "拷贝完就销毁源对象"的场景下，深拷贝纯属浪费 |
| 左值/右值 | 有名字的是左值，没名字的是右值 |
| 右值引用 `T&&` | 只能绑右值；让函数能重载出"参数是临时对象"的版本 |
| 移动构造/赋值 | 接管资源 + 把源对象置为可析构状态 + 标 `noexcept` |
| `std::move` | 只是 `static_cast` 到右值引用，自己不移动任何东西 |
| 移动后的状态 | 有效但未指定；只应重新赋值或销毁 |
| `noexcept` | 不标会让 vector 扩容退回拷贝，实测慢 1.6 倍 |
| Rule of Five | 需要写一个就要考虑全部五个 |
| **Rule of Zero** | 优先用 `string`/`vector`/`unique_ptr` 成员，五个都不用写 |
| 拷贝消除 | RVO/NRVO 可能连移动都省掉；C++17 对纯右值是强制的 |
| 性能 | 实测移动比拷贝快 8.9 倍（1000 字节 string）|
| 禁忌 | `return std::move(local)` 会阻止 NRVO |

> 下一章：[02-04 完美转发](02-04_perfect_forwarding.md)
