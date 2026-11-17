# 01-05 函数重载与默认参数

> 标准：C++98
> demo：[`01_05_overload.cpp`](../../demos/part1_basics/01_05_overload.cpp)

## 一、重载解决了什么问题

实际开发中经常需要几个功能相同、只是细节不同的函数。
交换两个变量的值就是典型 —— 变量可以是 `short`、`int`、`float`、`double`……

C 里只有一个全局名字空间，而且不允许同名函数，所以必须手工区分：

```c
void swap_short(short*, short*);
void swap_int(int*, int*);
void swap_float(float*, float*);
void swap_double(double*, double*);
```

调用方要自己选对函数名。类型改了，所有调用点都得跟着改。
标准库里到处是这种痕迹：`abs` / `labs` / `llabs` / `fabs` / `fabsf` / `fabsl`，
六个名字做同一件事。

C++ 允许多个函数拥有相同的名字，只要**参数列表不同**，这就是函数重载。
一个函数名可以有多种用途，调用方只需要写 `swap(a, b)`，由编译器根据实参类型挑选。

好处有三个：减少了函数名的数量、避免了名字空间污染、提高了可读性。

## 二、构成重载的条件

```cpp
void print(int x);
void print(double x);
void print(const char* x);
void print(const std::string& x);
void print(int x, int y);
```

规则：

1. **函数名称必须相同**
2. **参数列表必须不同** —— 类型不同、个数不同、或顺序不同
3. 函数的返回类型可以相同也可以不同
4. **仅仅返回类型不同，不足以构成重载**

第 4 点是硬性限制：

```cpp
int    f(int x);
// double f(int x);     // 编译错误：ambiguating new declaration
```

原因很实际：调用 `f(1);` 而不使用返回值时，编译器无法判断该调哪个。
C++ 的重载决议只看实参，不看返回值期望的类型。

### 顶层 const 不参与区分

```cpp
void f(int x);
// void f(const int x);   // 编译错误：和上面是同一个函数
```

因为参数是按值传递的，`const` 只影响函数内部能否修改这个副本，
对调用方毫无差别，所以不构成不同的签名。

但**底层 const 可以**：

```cpp
void f_ref(int& x);         // 非常量引用
void f_ref(const int& x);   // 常量引用 —— 这是合法的重载
```

实测调用结果：

```
f_ref(a);   // int&        —— a 是非 const 左值
f_ref(b);   // const int&  —— b 是 const
f_ref(3);   // const int&  —— 右值只能绑 const 引用
```

指针同理：`f(int*)` 和 `f(const int*)` 是两个不同的重载。
这个机制是 `begin()`/`end()`、`operator[]` 有 const 和非 const 两个版本的基础。

## 三、实现原理：name mangling

C++ 实现重载的机制叫**名字改编（name mangling）**：
编译器把参数类型信息编码进符号名，所以在链接层面它们本来就是不同的函数。

用 `nm` 看编译产物（实测）：

```bash
$ g++ -c 01_05_overload.cpp && nm m.o | grep mangle
_Z11mangle_demod
_Z11mangle_demoi
_Z11mangle_demoiPc
```

解码一下（`nm -C` 做 demangle）：

```
_Z11mangle_demod    ->  mangle_demo(double)
_Z11mangle_demoi    ->  mangle_demo(int)
_Z11mangle_demoiPc  ->  mangle_demo(int, char*)
```

规则看得很清楚：`_Z` 是前缀，`11` 是函数名长度，`mangle_demo` 是名字，
后面跟参数类型编码 —— `i`=int、`d`=double、`Pc`=char*。
**返回类型没有被编码进去**，这正是"仅返回类型不同不能重载"的底层原因。

### extern "C" 关闭 mangling

```cpp
extern "C" void c_linkage(int);
// extern "C" void c_linkage(double);   // 编译错误：C 链接的函数不能重载
```

实测符号名：

```
c_linkage        <- 没有 _Z 前缀，没有类型编码
```

`extern "C"` 告诉编译器按 C 的方式生成符号名，于是 C 代码能调用它。
代价是失去重载能力。这是 C++ 库提供 C 接口的标准做法：

```cpp
#ifdef __cplusplus
extern "C" {
#endif
    void mylib_init(void);
    void mylib_free(void*);
#ifdef __cplusplus
}
#endif
```

mangling 规则**不是标准的一部分**，各编译器实现不同（GCC/Clang 用 Itanium ABI，
MSVC 用自己的格式）。这是不同编译器编译的 C++ 库无法互相链接的原因之一，
也是跨编译器的二进制接口通常只能用 `extern "C"` 的原因。

## 四、重载决议：编译器怎么挑

候选函数不止一个时，编译器按"**转换代价**"排序，选代价最小的。
从好到差大致是：

1. **精确匹配**（含数组转指针、函数转指针、顶层 const 调整）
2. **提升**（`char`/`short`/`bool` → `int`，`float` → `double`）
3. **标准转换**（整型 ↔ 浮点、指针 → `void*`、派生类指针 → 基类指针）
4. **用户定义的转换**（转换构造函数、转换运算符）
5. **可变参数**（`...`）

实测验证：

```cpp
void resolve(int);
void resolve(double);

resolve('a');    // 选 int    —— char -> int 是提升，优先于 char -> double 的转换
resolve(1.0f);   // 选 double —— float -> double 是提升
resolve(true);   // 选 int    —— bool -> int 是提升
```

输出：

```
  精确匹配 int        <- resolve('a')
  精确匹配 double     <- resolve(1.0f)
  精确匹配 int        <- resolve(true)
```

同级别的多个候选会导致**二义性**：

```cpp
resolve(1L);   // 编译错误：long -> int 和 long -> double 都是"标准转换"，同级
```

这类错误在混用整型宽度时很常见。解决办法是显式转换，或者补一个精确匹配的重载。

## 五、默认参数

### 目的

通常调用函数要给每个参数提供实参。默认参数让你省掉那些"通常就是这个值"的参数：

```cpp
void func1(int x = 0, int y = 0);

func1();        // x=0, y=0
func1(1);       // x=1, y=0
func1(1, 2);    // x=1, y=2
```

允许设置默认参数值，是为了让编程简单，同时让编译器承担更多检查工作。

### 顺序规定：从右往左连续

```cpp
// void bad(int a = 1, int b, int c = 0, int d);   // 编译错误
void ok(int a, int b, int c = 0, int d = 0);       // 正确
```

如果给某个参数设了默认值，它右边的所有参数都必须也有默认值。
原因很直接：调用时实参是从左往右匹配的，中间跳过一个参数没有语法可以表达，
编译器无法判断调用时哪个参数被省略了。

### 写在声明处还是定义处

```cpp
// 头文件：声明处给默认值（推荐）
void f(int x = 100);

// 源文件：定义处不能重复给
void f(int x) { /* ... */ }
```

一个函数既有声明又有定义时，**只需要在其中一个里设置默认值**，而且不能都设置。

为什么推荐放在声明里？因为声明是给编译器和调用方看的"向导"。
如果只在定义时设默认值，那么编译器只有看到定义才知道有默认参数 ——
定义之前的调用点会报参数不足。所以通常把默认值放在声明中。

还要注意：**默认参数是调用点的属性**。调用方看到哪个声明，就用哪个默认值。
这意味着在不同头文件里给同一函数声明不同的默认值，会导致不同编译单元行为不一致 ——
这类 bug 极难查。

### 求值时机：每次调用时求值

```cpp
int counter = 0;
int next_id() { return ++counter; }

void f(int id = next_id());

f();       // next_id() 被调用，id=1
f();       // 又被调用，id=2
f(100);    // 不调用 next_id()
```

实测输出：

```
  id = 1
  id = 2
  id = 100
  counter = 2
```

默认参数的表达式在**每次调用时**求值，不是在编译期或首次调用时。
顺带说一句，这和 Python 的默认参数语义完全相反（Python 的默认值在定义时求值一次，
所以可变默认值是个经典坑），从 Python 转来的人要注意。

## 六、默认参数与重载的关系

默认参数可以把一系列简单的重载合成一个：

```cpp
// 三个重载
void f();
void f(int x);
void f(int x, int y);

// 合成一个
void f(int x = 0, int y = 0);
```

好处是只有一份函数体，不需要让几个重载互相转发。

但两者混用容易撞出**二义性** —— 如果一组重载（可能带默认参数）
都允许相同实参个数的调用：

```cpp
void risky(int);
void risky(int, int = 0);

risky(1);      // 编译错误：两个候选都能接受一个 int
risky(1, 2);   // ok：只有第二个能匹配
```

更隐蔽的版本：

```cpp
void f(int);
void f(int x, int y = 0);
void f(int x = 0, int y = 0);   // 三个声明放在一起，f(1) 和 f() 都是歧义
```

所以在函数重载时要谨慎使用默认参数。经验做法：
**同一个函数名，要么全用重载，要么全用默认参数，不要混。**

### 虚函数的默认参数：别用

```cpp
class Base    { public: virtual void f(int x = 10) { /* ... */ } };
class Derived : public Base { public: void f(int x = 20) override { /* ... */ } };

Base* p = new Derived;
p->f();      // 调用 Derived::f，但用的是 Base 的默认值 10！
```

默认参数是**静态绑定**（按指针的静态类型），虚函数是**动态绑定**。
两者混在一起的结果是"调了派生类的实现，用了基类的默认值"，几乎总是 bug。
规则：虚函数不要带默认参数。

## 七、小结

| 要点 | 说明 |
|---|---|
| 重载条件 | 同名、参数列表不同（类型/个数/顺序）|
| 不构成重载 | 仅返回类型不同、仅顶层 const 不同 |
| 底层 const 可区分 | `f(int&)` vs `f(const int&)`、`f(int*)` vs `f(const int*)` |
| 实现原理 | name mangling，把参数类型编入符号名；返回类型不编入 |
| `extern "C"` | 关闭 mangling，因此不能重载；用于提供 C 接口 |
| 决议顺序 | 精确匹配 > 提升 > 标准转换 > 用户转换 > 省略号 |
| 默认参数顺序 | 必须从右往左连续 |
| 默认值位置 | 放声明处，定义处不重复 |
| 求值时机 | 每次调用时求值 |
| 混用风险 | 重载 + 默认参数易产生二义性；虚函数不要用默认参数 |

> 下一章：[01-06 bool / nullptr / enum class](01-06_bool_nullptr_enum.md)
