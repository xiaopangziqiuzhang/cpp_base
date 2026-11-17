# 03-03 析构函数与对象生命周期

> demo：[`03_03_destructor.cpp`](../../demos/part3_class/03_03_destructor.cpp)

## 一、析构函数的特征

构造函数在创建对象时被自动调用，析构函数（destructor）在对象被销毁时被自动调用。
相比构造函数，它要简单得多：

- **与类同名，之前冠以波浪号 `~`**，以区别于构造函数
- **没有返回类型，也不能指定参数** —— 因此析构函数**只能有一个，不能被重载**
- **对象超出作用域被销毁时自动调用**

```cpp
class Point {
public:
    ~Point() {}
};
```

它用来执行清理任务，如释放构造函数中动态申请的内存。
如果程序员没有显式定义，系统会提供一个默认的析构函数。

## 二、什么时候需要显式写析构函数

### 需要：成员持有需要手工释放的资源

```cpp
class Computer {
public:
    Computer(const char* brand, double price)
        : _brand(new char[std::strlen(brand) + 1]())    // 构造时申请
        , _price(price) {
        std::strcpy(_brand, brand);
    }

    ~Computer() {
        delete[] _brand;      // 析构时必须回收，否则泄漏
    }
private:
    char* _brand;
    double _price;
};
```

成员里有裸指针（或文件句柄、socket、锁），
编译器生成的默认析构函数只会析构成员本身 ——
对指针成员来说就是"什么也不做"，指向的堆内存泄漏。

实测：

```
      Computer(Huawei MateBook14)
      ~Computer(Huawei MateBook14) 回收堆空间
```

**写了析构函数就要考虑拷贝控制。** 默认的拷贝构造会做浅拷贝，
两个对象的 `_brand` 指向同一块内存，析构两次就是 double free。
这是 Rule of Three / Rule of Five 的由来，详见 [03-04](03-04_copy_control.md)。

### 不需要：Rule of Zero

```cpp
class ModernComputer {
public:
    ModernComputer(std::string brand, double price)
        : _brand(std::move(brand)), _price(price) {}
    // 不需要写析构函数
private:
    std::string _brand;       // string 自己会清理
    double _price;
};
```

成员都是 RAII 类型（`string`/`vector`/`unique_ptr`/`fstream`）时，
**一个特殊函数都不用写** —— 编译器生成的析构函数会依次析构每个成员，
而每个成员自己知道怎么清理。

这是现代 C++ 的首选。前面 `Computer` 那个例子之所以需要析构函数，
根源在于用了 `char*` 而不是 `std::string`。

## 三、析构函数在哪些时候被调用

参考文档给出了四条，逐个用实测验证。

### 1. 局部对象：离开作用域时，逆序析构

```cpp
void f() {
    Tracer a("local-a");
    Tracer b("local-b");
    Tracer c("local-c");
}
```

实测：

```
      + local-a
      + local-b
      + local-c
      - local-c      <- 逆序
      - local-b
      - local-a
```

构造顺序 a→b→c，析构顺序 c→b→a，严格逆序。
原因是后构造的对象可能依赖先构造的对象，必须先销毁依赖方。

嵌套作用域同理：

```
      + outer
      + inner
      - inner        <- 内层先结束
      inner 已析构，outer 还活着
      - outer
```

### 2. 静态局部对象：首次执行到定义处才构造，程序结束才析构

```cpp
void f(int call_id) {
    static Tracer s("static-local");     // 只构造一次
}
f(1); f(2); f(3);
```

实测：

```
      第 1 次调用
      + static-local     <- 只在第一次构造
      第 2 次调用
      第 3 次调用
      ...
      === main 结束 ===
      - static-local     <- main 之后才析构
```

C++11 起，**静态局部变量的初始化是线程安全的** ——
多个线程同时首次执行到这一行时，只有一个会执行构造，其他线程等待。
这是 Meyers Singleton 的基础，见 [03-07](03-07_singleton.md)。

### 3. 全局对象：main 之前构造，main 之后析构

```cpp
Tracer g_global("global");
int main() { /* ... */ }
```

实测输出的第一行就是 `+ global`，出现在 `=== main 开始 ===` 之前；
最后一行是 `- global`，在 `=== main 结束 ===` 之后。

每当程序开始运行，在主函数 main 接受程序控制权之前，就调用构造函数创建全局对象；
整个程序结束时，自动调用全局对象的析构函数。

**注意静态初始化顺序问题**：不同翻译单元（`.cpp` 文件）中全局对象的构造顺序
是**未指定**的。如果 `a.cpp` 的全局对象在构造时用到了 `b.cpp` 的全局对象，
可能用到一个还没构造的对象 —— 这就是 static initialization order fiasco。
解决办法是用静态局部变量（函数内的 static）替代全局对象，
让构造推迟到首次使用时。

### 4. 堆对象：delete 时才析构

```cpp
Tracer* p = new Tracer("heap-raw");
// 不 delete 就永远不析构
delete p;
```

实测确认只有 `delete` 才触发析构。这正是裸指针容易泄漏的原因。
用智能指针就回到了"离开作用域自动析构"：

```cpp
{
    auto up = std::make_unique<Tracer>("heap-unique");
}   // 自动析构
```

## 四、成员对象与基类的析构顺序

这是最完整的一组顺序规则。先看实测结果：

```
    构造:
      + Base成员                <- 1. 基类的成员
      BaseC() 构造完成           <- 2. 基类的构造函数体
      + Derived成员1            <- 3. 派生类的成员（按声明顺序）
      + Derived成员2
      DerivedC() 构造完成        <- 4. 派生类的构造函数体

    析构:
      ~DerivedC() 开始          <- 1. 派生类的析构函数体
      - Derived成员2            <- 2. 派生类的成员（逆序）
      - Derived成员1
      ~BaseC() 开始             <- 3. 基类的析构函数体
      - Base成员                <- 4. 基类的成员
```

**构造顺序**：基类 → 自己的成员（按声明顺序）→ 自己的构造函数体
**析构顺序**：自己的析构函数体 → 自己的成员（逆序）→ 基类

完全逆序，没有例外。这个顺序保证了：

- 构造函数体执行时，所有成员和基类都已就绪
- 析构函数体执行时，所有成员和基类还未销毁（可以安全使用它们）

成员的析构顺序同样由**声明顺序**决定（逆序），
这也是 [03-02](03-02_constructor.md) 里"初始化顺序由声明顺序决定"的原因 ——
只有这样才能有唯一确定的逆序。

## 五、容器中的对象

```cpp
{
    std::vector<Tracer> v;
    v.reserve(3);
    v.emplace_back("vec-0");
    v.emplace_back("vec-1");
    v.emplace_back("vec-2");
}
```

实测：

```
      + vec-0
      + vec-1
      + vec-2
      ---- vector 即将销毁 ----
      - vec-0        <- 注意：正序，不是逆序
      - vec-1
      - vec-2
```

`vector` 析构时会析构所有元素，但**顺序是正序**（libstdc++ 的实现）。
标准没有规定容器元素的析构顺序，所以不要依赖它。
这和局部对象的"严格逆序"不同 —— 局部对象的逆序是标准规定的。

## 六、显式调用析构函数

析构函数除了自动调用，也可以显式手动调用，但**一般不建议这样使用**：

```cpp
Tracer t("obj");
t.~Tracer();        // 显式调用
// 作用域结束时还会再析构一次 -> 二次析构，未定义行为
```

**唯一的正当场合是配合 placement new**（见 [02-02](../part2_memory/02-02_new_delete.md)）：

```cpp
alignas(Tracer) unsigned char buf[sizeof(Tracer)];
Tracer* p = new (buf) Tracer("placement");
p->~Tracer();       // 必须显式析构，因为没有对应的 delete
```

实测确认这个用法正常工作。标准库容器内部就是这么做的 ——
`vector::pop_back` 调用元素的析构函数但不释放内存。

## 七、析构函数的三条禁忌

### 一、绝不能抛异常

[01-08](../part1_basics/01-08_exception.md) 详细讲过：
栈展开期间析构函数再抛异常 → `std::terminate`。
C++11 起析构函数默认 `noexcept`。

### 二、基类的析构函数要是虚函数（如果要多态删除）

```cpp
class Base { public: ~Base() {} };           // 非虚
class Derived : public Base { /* 有资源 */ };

Base* p = new Derived;
delete p;        // 只调用 ~Base()，~Derived() 不会执行 -> 泄漏
```

规则：**如果一个类会被用作基类且通过基类指针删除，它的析构函数必须是 `virtual`**。
详见 [04-06](../part4_oop/04-06_abstract.md)。

反过来，不打算被继承的类不要加 `virtual` ——
会多一个虚表指针，增大对象体积。

### 三、不要在析构函数里调用虚函数

```cpp
class Base {
public:
    virtual void cleanup() {}
    ~Base() { cleanup(); }      // 危险：此时派生类部分已经析构了
};
```

析构到基类时，派生类部分已经销毁，虚函数调用会解析到**基类的版本**
而不是派生类的。详见 [04-04](../part4_oop/04-04_virtual.md)。

## 八、小结

| 要点 | 说明 |
|---|---|
| 特征 | `~类名()`、无返回类型、无参数、**不能重载**、自动调用 |
| 何时需要显式写 | 成员持有裸指针/句柄等需手工释放的资源 |
| 何时不需要 | 成员都是 RAII 类型（Rule of Zero，首选）|
| 局部对象 | 离开作用域时**逆序**析构（标准规定）|
| 静态局部对象 | 首次执行到定义处构造（C++11 起线程安全），程序结束析构 |
| 全局对象 | main 前构造，main 后析构；跨翻译单元的顺序未指定 |
| 堆对象 | `delete` 时才析构（用智能指针恢复自动性）|
| 构造顺序 | 基类 → 自己的成员（声明顺序）→ 自己的函数体 |
| 析构顺序 | 自己的函数体 → 自己的成员（逆序）→ 基类 |
| 容器元素 | 会被析构，但顺序未规定（libstdc++ 实测是正序）|
| 显式调用 | 只在配合 placement new 时正当；普通对象会二次析构 |
| 三条禁忌 | 不抛异常、基类要虚析构、不在析构里调虚函数 |

> 下一章：[03-04 拷贝控制与 Rule of Five](03-04_copy_control.md)
