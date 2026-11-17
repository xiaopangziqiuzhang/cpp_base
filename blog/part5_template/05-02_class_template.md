# 05-02 类模板

> 标准：C++98；变量模板 `[C++14]`；CTAD `[C++17]`
> demo：[`05_02_class_template.cpp`](../../demos/part5_template/05_02_class_template.cpp)

## 一、解决了什么问题

函数模板让"算法"与类型解耦，类模板让"数据结构"与类型解耦。

没有类模板时，一个栈要为每种元素类型写一遍：

```cpp
class IntStack { int* _base; /* ... */ };
class DoubleStack { double* _base; /* ... */ };
class StringStack { std::string* _base; /* ... */ };
```

或者用 `void*` 绕过类型（C 的做法）：

```c
struct Stack { void** _base; };    /* 丢失类型安全，使用时要强转 */
```

类模板允许用户为类定义一种**模式**，
使得类中的某些数据成员、成员函数的参数、某些成员函数的返回值
能够取任意类型：

> 如果一个类中数据成员的类型不能确定，
> 或者某个成员函数的参数或返回值类型不能确定，
> 就必须将此类声明为模板。它的存在不代表一个具体的、实际的类，
> 而是代表**一类类**。

参考文档里的比喻很贴切：类模板是做蛋糕的**模具**，
模板类是用这个模具做出来的**蛋糕** ——
至于蛋糕是什么味道的，取决于实例化时用的什么材料。
可以做巧克力蛋糕，也可以做草莓蛋糕，除了材料不同，其它都一样。

## 二、定义形式

```cpp
template <class T, int kMax = 10>
class Stack {
public:
    Stack() : _top(-1), _base(new T[kMax]()) {}
    ~Stack() { delete[] _base; }

    bool empty() const;          // 类内声明
    void push(const T& t);
    T& top();

    int size() const { return _top + 1; }    // 类内定义

private:
    int _top;
    T* _base;
};
```

类模板可以有类型参数（`T`）和非类型参数（`kMax`），都可以有默认值。

### 类外定义成员函数

每个在类外定义的成员函数都要带**完整的模板参数列表**，
并且类名要写成 `类名<模板参数>`：

```cpp
template <class T, int kMax>
bool Stack<T, kMax>::empty() const { return _top == -1; }

template <class T, int kMax>
void Stack<T, kMax>::push(const T& t) {
    if (!full()) { _base[++_top] = t; }
}
```

注意三处细节：

- `template <class T, int kMax>` 要重复写，而且**不能带默认值**
  （默认值只在类定义处写一次）
- 类名是 `Stack<T, kMax>`，不是 `Stack`
- 返回类型如果是类内的嵌套类型，要加 `typename`
  （见 [05-06](05-06_template_engineering.md)）

这种写法很啰嗦，所以实践中**类模板的成员函数常常直接在类内定义** ——
反正模板的实现必须在头文件里（见 [05-06](05-06_template_engineering.md)），
分开写没有编译期收益。

实测使用：

```
      Stack<int,3>: size=3 top=3
        栈已满，无法 push
      pop 之后 top=2
      Stack<string>: size=2 top=world
```

## 三、每个实例是独立的类型

这是类模板最重要的性质：

```cpp
std::is_same<Stack<int>, Stack<double>>::value         // false
std::is_same<Stack<int,10>, Stack<int,20>>::value      // false
std::is_same<Stack<int>, Stack<int,10>>::value         // true（默认参数相同）
```

实测确认。**每组不同的模板实参产生一个完全独立的类**，
它们之间没有任何继承或转换关系：

```cpp
Stack<int> si;
Stack<double> sd;
// si = sd;         // 编译错误：类型完全无关
```

连 `Stack<int>` 和 `Stack<long>` 都是毫不相干的两个类型。
这与 Java 的泛型（类型擦除，运行期都是同一个类）有本质区别。

### 推论：静态成员各自独立

```cpp
template <class T>
class Counter {
    static int _count;
public:
    Counter() { ++_count; }
    static int count() { return _count; }
};

template <class T>
int Counter<T>::_count = 0;      // 类外定义
```

实测：

```
      Counter<int>::count()    = 3
      Counter<double>::count() = 1
```

因为 `Counter<int>` 和 `Counter<double>` 是两个类，
各有一份 `_count`。这个性质正是 [04-07](../part4_oop/04-07_crtp.md)
里 CRTP 实例计数能工作的原因。

注意静态成员的类外定义语法 —— 也要带模板参数列表。
C++17 之后可以用 `static inline` 省掉它（见
[03-05](../part3_class/03-05_special_members.md)）：

```cpp
template <class T>
class Counter {
    static inline int _count = 0;      // [C++17] 不需要类外定义
};
```

## 四、成员函数模板

成员函数模板既可以定义在普通类中，也可以定义在类模板中。

### 普通类里的成员函数模板

```cpp
class CharArray {
public:
    // 使用时不用提供模板参数，自动从实参推导
    template <class Iterator>
    void print_elements(Iterator beg, Iterator end) const {
        while (beg != end) { std::cout << *beg; ++beg; }
    }
};

CharArray ca("hello,world");
ca.print_elements(ca.c_str(), ca.c_str() + 5);     // Iterator = const char*
std::vector<char> v{'a','b','c'};
ca.print_elements(v.begin(), v.end());              // Iterator = vector<char>::iterator
```

实测输出 `hello` 和 `abc` —— 同一个成员函数接受了两种完全不同的迭代器类型。

这是 STL 的核心设计 —— 容器的构造函数、`insert`、`assign`
都是成员函数模板，所以可以接受任意迭代器：

```cpp
std::vector<int> v(list.begin(), list.end());     // 从 list 构造 vector
```

### 类模板里的成员函数模板

```cpp
template <class T>
class Wrapper {
public:
    // 可以和任意类型的 Wrapper 比较
    template <class U>
    bool equals(const Wrapper<U>& other) const { return _value == other.get(); }
private:
    T _value;
};

Wrapper<int> wi(42);
Wrapper<double> wd(42.0);
wi.equals(wd);        // true
```

两层模板参数：`T` 来自类，`U` 来自成员函数。
标准库里 `shared_ptr<Base>` 能从 `shared_ptr<Derived>` 构造，
靠的就是这个机制。

## 五、CTAD：类模板实参推导 `[C++17]`

C++17 之前，类模板**必须显式写出模板实参**，即使能从构造函数推出来：

```cpp
// C++14
std::pair<int, double> p(1, 2.5);              // 要写两个类型
std::vector<int> v{1, 2, 3};
std::lock_guard<std::mutex> lk(mtx);
```

这就是 `make_pair`、`make_tuple`、`make_shared` 这些工厂函数存在的原因 ——
函数模板能推导，类模板不能，所以用函数包一层：

```cpp
auto p = std::make_pair(1, 2.5);      // 靠函数模板推导
```

C++17 的 **CTAD（Class Template Argument Deduction）**让类模板也能推导：

```cpp
std::pair p(1, 2.5);                   // 推导为 pair<int, double>
std::vector v{1, 2, 3};                // 推导为 vector<int>
std::lock_guard lk(mtx);               // 推导为 lock_guard<mutex>
```

实测验证：

```
      Box b3(42) 推导为 Box<int>: 1
      Box b4(string) 推导为 Box<string>: 1
      std::vector v{1,2,3} 推导为 vector<int>: 1
      std::pair p(1, 2.5) 的类型正确: 1
```

### 推导指引（deduction guide）

有时自动推导不够用，需要显式告诉编译器怎么推：

```cpp
template <class T>
class Pair2 {
    Pair2(T a, T b);      // 两个参数必须同类型
};

// Pair2 pr(1, 2.5);      // 推导失败：T 不能既是 int 又是 double

// 推导指引：从两个不同类型推出共同类型
template <class T, class U>
Pair2(T, U) -> Pair2<std::common_type_t<T, U>>;

Pair2 pr(1, 2.5);          // 现在推导为 Pair2<double>
```

实测确认推导为 `Pair2<double>`。

推导指引的语法是：`模板名(参数类型...) -> 特化类型;`，写在类定义之外。
标准库大量使用它，比如让 `std::vector` 能从迭代器对推导元素类型：

```cpp
template <class Iter>
vector(Iter, Iter) -> vector<typename iterator_traits<Iter>::value_type>;
```

### CTAD 的限制

- **只在声明变量时生效**，不能用于函数参数、返回类型、成员声明
- 不能部分指定：`std::pair<int> p(1, 2.5);` 不合法（要么全写，要么全不写）
- 继承的构造函数不参与 CTAD

有了 CTAD，`make_pair`/`make_tuple` 基本可以退休了。
但 `make_shared`/`make_unique` 仍然必要 ——
它们不只是为了推导，还有一次分配和异常安全的好处
（见 [02-05](../part2_memory/02-05_smart_pointers.md)）。

## 六、变量模板 `[C++14]`

C++14 允许模板化变量：

```cpp
template <class T>
constexpr T pi = T(3.14159265358979323846L);

pi<float>      // 3.14159f
pi<double>     // 3.141592653589793
```

实测两者精度不同。这解决了"数学常量的精度应该匹配使用的类型"这个问题 ——
C++14 之前要么用宏，要么用函数（`pi<T>()`），要么写一个
`template <class T> struct Pi { static constexpr T value = ...; };`。

### 最常见的用途：type_traits 的 `_v` 后缀

```cpp
template <class T>
constexpr bool is_numeric = std::is_arithmetic<T>::value;

is_numeric<int>            // true
is_numeric<std::string>    // false
```

标准库从 C++17 起为所有 type_traits 提供了 `_v` 版本：

```cpp
std::is_same<int, int>::value       // C++11 写法
std::is_same_v<int, int>            // C++17 写法，就是变量模板
```

实测 `std::is_same_v<int,int>` 为 1。这个改进让模板元编程的代码短了很多
（见 [05-05](05-05_type_traits.md)）。

## 七、类模板的友元

```cpp
template <class T>
class Secret {
public:
    template <class U>
    friend void peek(const Secret<U>& s);     // 声明模板函数为友元
private:
    T _value;
};

template <class U>
void peek(const Secret<U>& s) {
    std::cout << s._value;      // 可以访问私有成员
}
```

实测可行。类模板的友元有三种写法，语义不同：

```cpp
template <class T>
class Secret {
    // 1. 非模板友元：所有 Secret<T> 都把这个具体函数当友元
    friend void global_peek(const Secret<int>&);

    // 2. 模板友元（一对一）：Secret<T> 只把 peek<T> 当友元
    friend void peek_one(const Secret<T>&);

    // 3. 模板友元（多对多）：Secret<T> 把所有 peek<U> 都当友元
    template <class U>
    friend void peek_all(const Secret<U>&);
};
```

第 2 种最常用也最合理 —— 只开放必要的访问权。
典型场合是 `operator<<`：

```cpp
template <class T>
class Matrix {
    friend std::ostream& operator<<(std::ostream& os, const Matrix<T>& m) {
        // 直接在类内定义最简单，避免了友元声明的各种语法陷阱
    }
};
```

## 八、小结

| 要点 | 说明 |
|---|---|
| 解决什么 | 数据结构与元素类型解耦，同时保持类型安全 |
| 定义 | `template <class T, int N = 10> class X {};` |
| 类外定义成员 | 要重复模板参数列表（不带默认值），类名写成 `X<T,N>` |
| **每个实例是独立类型** | `X<int>` 和 `X<double>` 毫无关系，不能互相赋值 |
| 推论 | 静态成员各自独立（CRTP 实例计数的原理）|
| 成员函数模板 | 普通类和类模板里都可以有；是 STL 接受任意迭代器的基础 |
| CTAD `[C++17]` | 从构造函数实参推导类模板参数，`make_pair` 之类基本可退休 |
| 推导指引 | `模板名(参数) -> 特化类型;`，用于自动推导不够用的情况 |
| CTAD 限制 | 只用于变量声明；不能部分指定模板实参 |
| 变量模板 `[C++14]` | `template <class T> constexpr T pi = ...;` |
| 典型用途 | type_traits 的 `_v` 后缀（`is_same_v`）|
| 友元 | 三种写法；一对一的模板友元最常用；`operator<<` 直接类内定义最简单 |

> 下一章：[05-03 特化与偏特化](05-03_specialization.md)
