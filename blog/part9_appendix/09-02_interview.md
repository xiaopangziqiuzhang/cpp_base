# 09-02 高频面试题解答

[知识树](../../notes/01_knowledge_tree.md) 第四节列出了参考文档里埋的考点和未解答的思考题。
这里逐个给出答案，并指向详细章节。

---

## 一、基础语法

### Q1：const 常量与宏定义的区别？

| | 宏定义 | const 常量 |
|---|---|---|
| 处理阶段 | 预处理阶段展开，字符串替换 | 编译阶段 |
| 类型 | 没有类型 | 有具体类型 |
| 类型检查 | 不做 | 编译期检查 |
| 调试 | 符号表里没有，无法调试 | 可以调试 |
| 作用域 | 无视作用域 | 遵守作用域规则 |
| 能否是类成员 | 不能 | 可以 |

宏最危险的是**参数多次求值**：`MACRO_MAX(i++, j)` 会让 `i` 自增两次
（实测见 [01-02](../part1_basics/01-02_const_constexpr.md)）。
加括号只能解决优先级问题，解决不了多次求值。

**结论**：用 `const`/`constexpr` 替换宏常量，用 `inline` 函数替换宏函数。
唯一该保留宏的是 `assert`（需要 `__FILE__`/`__LINE__` 和 `NDEBUG`）。

详见 [01-02](../part1_basics/01-02_const_constexpr.md)、[01-07](../part1_basics/01-07_inline.md)。

### Q2：new/delete 与 malloc/free 的区别？

1. **`malloc`/`free` 是库函数，`new`/`delete` 是运算符**
2. **`new` 自动计算大小**，`malloc` 要传参
3. **`new` 可以同时初始化**，`malloc` 不行
4. **`new`/`delete` 会调用构造/析构函数**，`malloc`/`free` 不会 —— 最关键的一条
5. **类型安全**：`new` 返回正确类型，`malloc` 返回 `void*`
6. **失败行为**：`new` 抛 `bad_alloc`，`malloc` 返回 `NULL`

既然 `new`/`delete` 功能完全覆盖，为什么保留 `malloc`/`free`？
因为要调用 C 函数，而 C 只能用 `malloc`/`free`；
此外 `realloc` 没有 C++ 对应物。

**不能混用**：`malloc` 的必须 `free`，`new` 的必须 `delete`。

详见 [02-02](../part2_memory/02-02_new_delete.md)。

### Q3：引用的本质是什么？引用占内存吗？

**本质**：引用是**被限制的指针** —— 底层通常用指针实现（汇编层面是间接寻址），
但被限制掉了"可为空"和"可改指向"两个自由度。

**占不占内存要看场景**：

```cpp
int x = 0; int& r = x;
sizeof(r);            // 4，返回的是 int 的大小（语言规定），不是引用的大小

struct WithRef { int& r; };
sizeof(WithRef);      // 8，确实占一个指针
```

实测见 [01-03](../part1_basics/01-03_reference.md)。

局部的 `int& r = x;` 几乎总被编译器优化掉（直接当 `x` 用），
而类成员引用必须占空间。所以"引用不占内存"只在
"局部引用通常被优化掉"的意义上成立。

### Q4：为什么初始化列表的顺序由声明顺序决定？

因为**析构顺序必须是构造顺序的严格逆序**。
一个类只有一份成员声明，却可能有多个构造函数、每个的初始化列表顺序都不同。
如果按书写顺序初始化，析构函数就无法确定该用哪个逆序。

所以只能以声明顺序为唯一依据。

实测后果（用了未初始化的成员，得到垃圾值 32590）和编译器警告
`-Wreorder` 见 [03-02](../part3_class/03-02_constructor.md)。

**实践**：初始化列表的书写顺序永远与声明顺序一致，并开启 `-Wall`。

### Q5：拷贝构造函数的参数形式能否改变？

**不能改成按值传递**：

```cpp
Point(Point rhs);        // 编译错误
```

因为按值传参本身就要调用拷贝构造函数来构造 `rhs`，
而那又会调用拷贝构造函数来构造它的参数 —— 无限递归。
编译器直接拒绝这种声明。

参数也应该是 `const` 的，否则无法接受右值（临时对象）。

详见 [03-04](../part3_class/03-04_copy_control.md)。

### Q6：赋值运算符函数的返回值类型能否改变？

技术上可以返回 `void` 或返回值，但都不该：

- 返回 `void` → 无法支持链式赋值 `a = b = c`
- 返回值（而非引用）→ 多一次拷贝，而且 `(a = b) = c` 行为诡异

返回 `类名&` 是与内置类型行为一致的唯一选择。

详见 [03-04](../part3_class/03-04_copy_control.md)。

### Q7：定义一个 const 成员函数时，到底发生了什么？

**`this` 指针的类型变了**：

| 成员函数 | `this` 的类型 |
|---|---|
| 普通成员函数 | `T* const this` |
| const 成员函数 | `const T* const this` |

实测用 `decltype(this)` 验证见 [03-06](../part3_class/03-06_const_member.md)。

所以"不能修改成员""不能调用非 const 成员函数"并不是额外规则，
**就是 const 指针的常规行为**。

这也解释了为什么静态成员函数不能是 const —— 它没有 `this`。

### Q8：通过空指针调用成员函数会发生什么？

| 调用 | 结果 | 原因 |
|---|---|---|
| 静态成员函数 | **正常** | 没有 `this` 参数，不需要对象 |
| 非静态但不访问成员 | **实践中能跑**，但是 UB | `this` 是 nullptr，但没解引用它 |
| 访问非静态成员 | **崩溃** | 要算 `this->member`，解引用空指针 |
| 虚函数 | **崩溃** | 要通过 `this` 找虚表指针 |

实测见 [03-01](../part3_class/03-01_class_basics.md)。

**关键区分**："能跑"不代表"合法"。
标准规定通过空指针调用非静态成员函数就是未定义行为，
编译器在 `-O2` 下可能假定 `this != nullptr` 并据此优化。

这个题说明两件事：成员函数不存在对象里（调用它只是传了个 `this`）；
虚函数调用必须解引用 `this`。

---

## 二、面向对象

### Q9：哪些函数不能是虚函数？

| 不能是虚函数 | 原因 |
|---|---|
| **构造函数** | 调用时虚表指针还没初始化（它是构造过程的一部分）|
| **静态成员函数** | 没有 `this`，无法找到虚表 |
| **友元函数** | 不是成员函数 |
| **普通全局函数** | 不是成员函数 |

**析构函数可以而且通常应该是虚函数。**

`inline` 虚函数能编译，但虚调用时 `inline` 请求会被忽略
（编译期无法确定目标）。

详见 [04-04](../part4_oop/04-04_virtual.md)。

### Q10：构造/析构函数中调用虚函数会发生什么？

**虚函数调用会解析到"当前正在构造/析构的那一层"的版本。**

实测输出（构造 `Derived` 对象）：

```
        Base() 构造函数里调用 which(): Base::which()        <- 不是 Derived 的！
        Derived() 构造函数里调用 which(): Derived::which()
    ---- 销毁 ----
        ~Derived() 析构函数里调用 which(): Derived::which()
        ~Base() 析构函数里调用 which(): Base::which()
```

原因：**虚表指针随构造过程逐层更新**。构造 `Base` 部分时，
vptr 还指向 `Base` 的虚表（因为 `Derived` 部分还没构造，
调用 `Derived::which()` 会访问未初始化的成员）。析构时逆向回退。

这是必要的安全措施。最危险的情况是基类构造函数调用**纯虚函数** ——
那是未定义行为，通常导致链接错误或运行时 abort。

**规则：不要在构造/析构函数里调用虚函数。**
需要时用两阶段初始化 + 工厂函数。

详见 [04-04](../part4_oop/04-04_virtual.md)。

### Q11：为什么基类的析构函数要是虚函数？

```cpp
class Base { public: ~Base() {} };                 // 非虚
class Derived : public Base { /* 持有资源 */ };

Base* p = new Derived;
delete p;        // 只调用 ~Base()，~Derived() 不执行
```

后果不只是泄漏，而是**未定义行为**。
实测 ASan 报的是 `new-delete-type-mismatch`：

```
object passed to delete has wrong type:
  size of the allocated type:   8 bytes;
  size of the deallocated type: 1 bytes.
```

编译器警告：`-Wdelete-non-virtual-dtor`（要求基类是多态类型）。

**规则**：会被用作基类且通过基类指针删除时，析构函数必须 `virtual`。
更简单的判据：**任何有虚函数的类，析构函数都应该是虚的**。

反过来，不打算被继承的类不要加 `virtual`（会多一个虚表指针）。

详见 [04-06](../part4_oop/04-06_abstract.md)。

### Q12：重载、隐藏、覆盖的区别？

| | 条件 | 作用域 | 调用解析 |
|---|---|---|---|
| **重载** | 同一作用域，参数列表不同 | 同一个类内 | 编译期按参数选 |
| **隐藏** | 派生类有同名成员（任何签名）| 跨类 | 派生类的名字遮蔽基类的 |
| **覆盖** | 虚函数 + 签名**完全相同** | 跨类 | **运行期按对象实际类型** |

实测行为差异见 [04-05](../part4_oop/04-05_override.md)。

关键点：

- **覆盖要求签名完全一致**（含 const 限定、引用限定），差一点就变成"隐藏"
- C++11 之前覆盖失败是**静默的** —— 实测开了 `-Wall -Wextra` 也没有任何警告
- 用 `override` 把它变成编译错误，**代价为零**

### Q13：虚函数的实现机制？

编译器为每个有虚函数的**类**生成一张**虚表（vtable）**，存着虚函数地址。
每个**对象**头部存一个**虚指针（vptr）**指向虚表。

虚调用 `p->f()` 被编译成"取 vptr → 查表 → 间接调用"。

实测（直接通过虚表槽位调用函数）：

```
      Base 对象的虚表地址:    0x5571a75f4bd0
      Derived 对象的虚表地址: 0x5571a75f4ba0
      两个虚表不同: 1
      d 和 d2 的虚表相同: 1          <- 同类对象共享虚表
```

**空间代价是每个对象一个指针**，与虚函数个数无关
（实测 1 个虚函数和 5 个虚函数的对象大小相同）。

详见 [04-04](../part4_oop/04-04_virtual.md)。

### Q14：菱形继承的问题与虚继承？

**普通继承**：基类被包含两份，构造两次，访问二义。
实测两个 `_x` 的地址相差 8 字节。

**虚继承**：多条路径共享同一份基类。实测两条路径访问到**同一地址**。

虚继承的两条关键规则：

1. **虚基类由"最终派生类"构造** —— 中间类初始化列表里写的虚基类构造被忽略
2. **虚基类最先构造、最后析构**

代价：对象变大（虚基类指针）、访问多一次间接寻址、构造规则复杂。

**实践建议**：用纯接口（无数据成员的抽象类）做多继承，
就不会遇到这些问题。

详见 [04-03](../part4_oop/04-03_diamond.md)。

### Q15：什么是对象切片？

把多态对象**按值**传递时，只拷贝基类部分，派生类数据被"切掉"：

```cpp
void by_value(Shape s);              // 切片！
void by_reference(const Shape& s);   // 正确

Circle c;
by_value(c);         // 输出 Shape::draw()
by_reference(c);     // 输出 Circle::draw()
```

容器存基类对象同样会切片，所以要存智能指针。

**预防**：多态类型一律用指针/引用传递；容器存 `unique_ptr`；
基类的拷贝构造函数设为 protected 或 delete。

详见 [04-02](../part4_oop/04-02_derived_lifecycle.md)。

---

## 三、现代 C++

### Q16：std::move 做了什么？

**它只是一个 `static_cast`**，把表达式的值类别从左值变成右值，
**不产生任何运行时代码，自己不移动任何东西**。

实测：

```cpp
MyString a("original");
MyString&& ref = std::move(a);     // 这一行什么也没发生，a 仍是 "original"
MyString b = std::move(ref);        // 这里才真正移动
```

真正的移动发生在被调用的移动构造函数/移动赋值运算符里。
所以 `std::move` 更准确的名字应该是 `rvalue_cast`。

推论：对没有移动构造函数的类型用 `std::move` 会静默退回拷贝。

详见 [02-03](../part2_memory/02-03_move_semantics.md)。

### Q17：被移动之后的对象是什么状态？

**有效但未指定（valid but unspecified）**：

- **有效**：可以安全析构、可以安全赋新值
- **未指定**：不要假设具体内容

实测 libstdc++ 的 `string` 被移动后 `size()` 是 0，**但标准不保证**。

**规则**：被移动的对象，除了重新赋值或销毁，不要再碰它。

详见 [02-03](../part2_memory/02-03_move_semantics.md)。

### Q18：为什么移动构造函数要标 noexcept？

`vector` 扩容时如果搬到一半抛异常，会处于破碎状态无法回滚。
为了保证强异常安全，**`vector` 只在元素的移动构造函数是 `noexcept` 时才用移动，否则退回拷贝**。

实测两个类唯一的区别就是有没有 `noexcept`：

```
        不带 noexcept（扩容时只能拷贝）: 17054 us
        带 noexcept  （扩容时可以移动）: 10805 us
```

**少写一个 `noexcept`，性能损失 1.6 倍**，而代码看起来完全正常。

注意：用 `= default` 生成的移动函数会自动推导 `noexcept`；
**手写的必须自己加**。

另外，检测"是否真的可移动"要看 `is_nothrow_move_constructible`，
不要看 `is_move_constructible` —— 后者对只有拷贝构造函数的类也返回 true。

详见 [02-03](../part2_memory/02-03_move_semantics.md)。

### Q19：Rule of Three / Five / Zero？

- **Rule of Three**（C++98）：需要写析构函数、拷贝构造、拷贝赋值中任一个，
  通常三个都要写
- **Rule of Five**（C++11）：加上移动构造、移动赋值
- **Rule of Zero**（推荐）：把资源管理委托给已有的 RAII 类型，五个都不用写

**最容易踩的坑**是生成规则：

| 你声明了 | 后果 |
|---|---|
| 析构函数 | 移动函数**不再自动生成**（退化为拷贝，**无警告**）|
| 拷贝构造或拷贝赋值 | 移动函数不再自动生成 |
| 移动构造或移动赋值 | 拷贝函数被**定义为删除** |

所以**要么一个都不声明，要么全部五个都声明**。

详见 [03-04](../part3_class/03-04_copy_control.md)。

### Q20：lambda 捕获 this 的陷阱？

**`[=]` 和 `[this]` 捕获的都是 `this` 指针，不是对象**：

```cpp
std::function<int()> f;
{
    Widget w(42);
    f = w.get_lambda();        // 内部是 [this]
}   // w 销毁
f();                            // 未定义行为！this 已悬垂
```

三种解法：

```cpp
[*this]                         // C++17：按值捕获整个对象
[value = _value]                // C++14：只捕获需要的成员
[self = shared_from_this()]     // 延长对象寿命
```

C++20 起 `[=]` 隐式捕获 `this` 已被废弃，要写 `[=, this]`。

详见 [06-01](../part6_modern_core/06-01_lambda.md)。

### Q21：unique_ptr 和 shared_ptr 怎么选？

**默认 `unique_ptr`**：

- 零开销（和裸指针一样大、一样快）
- 不可拷贝只可移动，所有权转移在调用点可见
- 以后要改成 `shared_ptr` 很容易，反之很难

`shared_ptr` 的代价：

- 大小是裸指针的两倍 + 控制块
- 引用计数的增减是**原子操作**
- **循环引用会泄漏**（反向指针要用 `weak_ptr`）

实测循环引用的泄漏，而且 **ASan 默认检测不到** ——
因为栈上残留的指针让 LSan 误判为可达。清除栈残留后才报出 112 字节泄漏。

详见 [02-05](../part2_memory/02-05_smart_pointers.md)。

### Q22：string 的三种实现策略？为什么现在都用 SSO？

| 策略 | 做法 | 问题 |
|---|---|---|
| Eager Copy | 任何情况都深拷贝 | 频繁复制而不修改时低效 |
| COW | 浅拷贝 + 引用计数，写时才复制 | 多核下原子操作代价大 |
| **SSO** | 短字符串直接存在对象内部 | 对象变大（实测 32 字节）|

**为什么转向 SSO**：COW 的引用计数需要原子操作，而原子操作在多核下：

- 阻止 CPU 乱序执行
- 导致 cache 失效，要重新从内存读
- 系统会锁住比目标更大的区域

实测 libstdc++ 的 SSO 阈值精确地是 **15 个字符**，
`sizeof(std::string)` 是 32。短字符串拷贝快一倍多（无堆分配）。

COW 的引用计数为什么不和 string 的数据成员放在一起？
因为它必须被**所有共享该字符串的对象**看到，
放在对象内部就无法共享了 —— 所以放在堆上字符串数据的前面。
实测旧 ABI（COW）的 `sizeof(string)` 是 8，正是"只有一个指针"的布局。

详见 [07-01](../part7_stdlib/07-01_string.md)。

### Q23：单例模式的实现有什么问题？怎么改进？

经典的裸指针懒汉版本有两个问题：

1. **多线程不安全** —— 两个线程同时首次调用会创建两个实例
2. **析构函数永远不被调用** —— 实测确实没有析构输出

四次改进：

| 版本 | 线程安全 | 自动析构 | 锁开销 |
|---|---|---|---|
| 裸指针懒汉 | ✗ | ✗ | 无 |
| 全程加锁 | ✓ | ✗ | **每次调用** |
| 双检锁（裸指针 + 普通 if）| **✗（错的）** | ✗ | 仅首次 |
| **Meyers Singleton** | ✓ | ✓ | 可忽略 |

**Meyers Singleton** 利用 C++11 的保证"静态局部变量的初始化是线程安全的"：

```cpp
static Singleton& getInstance() {
    static Singleton instance;      // 三行解决所有问题
    return instance;
}
```

双检锁为什么错？因为 `_pInstance = new Singleton()` 的
"分配内存"和"运行构造函数"可能被重排，
另一个线程会拿到未构造完的对象。要写对必须用
`atomic` + acquire/release 内存序。

但更重要的问题是：**单例的本质是全局状态** ——
隐藏依赖、无法测试。优先考虑依赖注入。

详见 [03-07](../part3_class/03-07_singleton.md)。

---

## 四、模板

### Q24：为什么模板的定义必须放在头文件？

编译一个使用 `Widget<int>` 的翻译单元时，编译器需要
`Widget<int>::f()` 的**定义**才能实例化它。
如果定义在另一个 `.cpp` 里，那个翻译单元又不知道有人要 `Widget<int>`，
所以不会生成代码 → 链接错误。

连带后果：编译时间增加、实现细节暴露、代码膨胀。
C++20 的模块正是为解决这个问题。

详见 [05-06](../part5_template/05-06_template_engineering.md)。

### Q25：为什么要写 typename？

```cpp
template <class Container>
void f() {
    Container::value_type x;              // 编译错误
    typename Container::value_type y;     // 正确
}
```

编译器在**第一阶段**（模板定义时）不知道 `Container` 是什么，
无法判断 `value_type` 是**类型**还是**静态成员**。
标准规定默认当成值，所以要显式写 `typename`。

同理，依赖模板参数的**成员模板**要写 `template`：

```cpp
h.template get<int>();        // 否则 < 被当成小于号
```

而继承模板基类时要写 `this->`：

```cpp
template <class T>
class Derived : public Base<T> {
    void f() { this->base_method(); }     // 不加 this-> 编译错误
};
```

因为 `Base<T>` 是依赖基类，可能被特化成完全不同的东西，
所以编译器在第一阶段不进去查找名字。

这也解释了标准库为什么提供 `_t` 别名 ——
别名模板不能被特化，所以保证是类型，不需要 `typename`。

详见 [05-06](../part5_template/05-06_template_engineering.md)、
[06-03](../part6_modern_core/06-03_using.md)。

### Q26：函数模板能偏特化吗？

**不能。** 只有类模板可以偏特化。

而且**函数模板的全特化也不该用** —— 它不参与常规的重载决议排序。
实测这个例子里，`f<int*>` 特化**永远不会被调用**：

```cpp
template <class T> void f(T);
template <class T> void f(T*);
template <> void f<int*>(int*);     // 这是 f(T) 的特化，但重载决议会选 f(T*)

int* p;
f(p);        // 调用的是 f(T*)，不是那个特化
```

**结论（Herb Sutter 的建议）：不要特化函数模板，用重载。**

详见 [05-03](../part5_template/05-03_specialization.md)。

### Q27：SFINAE 是什么？

**Substitution Failure Is Not An Error** ——
模板实参替换失败时，编译器不报错，只是把该候选**从重载集里剔除**。

这是 C++ 模板实现"条件编译"的基础机制，`std::enable_if` 是它的标准封装。

C++17 的 `if constexpr` 取代了大部分 SFINAE 用法 ——
它更易读、错误信息更好。但两者分工不同：

- `enable_if` 控制**重载集**（这个函数是否存在）
- `if constexpr` 控制**函数体内的分支**

详见 [05-05](../part5_template/05-05_type_traits.md)。

---

## 五、并发

### Q28：为什么 ++counter 不是原子的？

它是**三步**：读取 → 加一 → 写回。

```
线程 A: 读到 100
线程 B: 读到 100        <- A 还没写回
线程 A: 写回 101
线程 B: 写回 101        <- 丢失了一次更新
```

实测 8 线程各 ++10 万次，期望 800000，**实际只有 201666**
（丢失 75%）。ThreadSanitizer 能准确报出竞争位置。

详见 [08-01](../part8_concurrency/08-01_thread.md)。

### Q29：条件变量为什么必须用带谓词的 wait？

因为**虚假唤醒** —— `wait()` 可能在没有 `notify` 的情况下返回。

两个原因：操作系统的实现细节（如信号中断）；
`notify_all` 唤醒多个线程但资源只够一个用。

```cpp
cv.wait(lk);                            // 错！被唤醒不代表条件满足
cv.wait(lk, [&]{ return condition; });  // 正确
```

带谓词的 `wait` 在标准里**就是定义为** `while (!pred()) wait(lk);`。

另外，条件变量必须配合互斥量，是为了避免 **lost wakeup** ——
保证"检查条件"和"开始等待"是原子的。

详见 [08-03](../part8_concurrency/08-03_condition_variable.md)。

### Q30：atomic 一定是无锁的吗？

**不是。** 超过机器字长的类型，`std::atomic` 内部用锁实现
（通常是按地址哈希的全局锁表）。

实测 16 字节和 24 字节的结构体 `is_lock_free()` 都是 0。

`std::atomic_flag` 是**唯一保证无锁**的原子类型。

顺带一个实际的坑：对大对象调用 `is_lock_free()` 会引用
`__atomic_is_lock_free`，不链接 `-latomic` 就是链接错误。

详见 [08-04](../part8_concurrency/08-04_atomic.md)。

### Q31：std::async 有什么坑？

**坑一：默认启动策略可能不并行**

```cpp
std::async(f);                              // 实现自己选 async 或 deferred
std::async(std::launch::async, f);          // 正确写法
```

`deferred` 不创建线程，`get()` 时在调用线程执行。
如果从不调 `get()`，任务**永远不执行**。

**坑二：future 析构会阻塞**

```cpp
std::async(std::launch::async, f);     // 临时 future 立刻析构 -> 阻塞等 f 完成
std::async(std::launch::async, g);     // 所以这是串行的！
```

实测不保存返回值时，离开作用域被阻塞了 50ms。
必须把 `future` 存起来才有并行。

详见 [08-05](../part8_concurrency/08-05_async.md)。

---

## 六、性能相关

### Q32：为什么说"减少共享"比"优化同步"更重要？

实测 8 线程各 50 万次递增：

```
        atomic:          21 ms
        mutex:           280 ms
        局部累加+汇总:   0 ms
```

`atomic` 比 `mutex` 快 13 倍，但**不共享快到测不出来**。

原因是**缓存行在核心之间来回弹跳** ——
多个核心争用同一缓存行时，每次写入都要让其他核心的缓存失效，
这个代价远超指令本身。

相关概念是 **false sharing**：两个不相关的变量恰好在同一缓存行。
解法是 `alignas(64)` 填充。

详见 [08-04](../part8_concurrency/08-04_atomic.md)。

### Q33：虚函数的开销有多大？

三项代价：

1. **对象变大** —— 每个对象多一个指针（实测 8 字节的类变成 16 字节）
2. **调用多一次间接跳转** —— 对分支预测不友好
3. **无法内联** —— **这是影响最大的一项**

实测 `-O2` 下虚函数比 CRTP 慢约 4 倍，但要注意这个对比
**包含了缓存局部性因素**（动态版本的元素分散在堆上）。

**大多数时候这些代价不值得关心** ——
只在函数体很小 + 调用极频繁 + 阻止向量化时才成为问题。

详见 [04-07](../part4_oop/04-07_crtp.md)。

### Q34：哪些常见写法有意外的性能问题？

| 写法 | 问题 | 实测 |
|---|---|---|
| 循环里 `std::endl` | 每次 flush | 慢 **15 倍** |
| 循环里构造 `std::regex` | 每次编译正则 | 慢 **102 倍** |
| 用 `ostringstream` 转换数值 | 维护格式状态 | 比 `to_string` 慢 **5 倍** |
| 漏写移动函数的 `noexcept` | 容器扩容退回拷贝 | 慢 **1.6 倍** |
| `for (auto x : container)` | 每轮拷贝元素 | 取决于元素大小 |
| 不 `reserve` 的 `push_back` | 反复扩容 | 慢 1.4 倍 |
| 按值传大对象 | 拷贝 | 取决于对象大小 |
| `return std::move(local)` | **阻止 NRVO** | 从零开销变成一次移动 |

---

## 七、未定义行为清单

面试常问"下面代码有什么问题"，多半是 UB。本书涉及的 UB 汇总：

| UB | 章节 |
|---|---|
| 修改字符串字面量 | [02-01](../part2_memory/02-01_memory_layout.md) |
| 返回局部变量的引用/指针 | [01-03](../part1_basics/01-03_reference.md) |
| 通过空指针调用非静态成员函数 | [03-01](../part3_class/03-01_class_basics.md) |
| `delete` 与 `delete[]` 不配对 | [02-02](../part2_memory/02-02_new_delete.md) |
| double free | [03-04](../part3_class/03-04_copy_control.md) |
| 通过非虚析构函数的基类指针删除派生类对象 | [04-06](../part4_oop/04-06_abstract.md) |
| 修改真正的 const 对象（通过 `const_cast`）| [01-04](../part1_basics/01-04_cast.md) |
| `static_cast` 的错误下行转换后使用 | [01-04](../part1_basics/01-04_cast.md) |
| 类型双关（`reinterpret_cast` 后读）| [01-04](../part1_basics/01-04_cast.md) |
| 使用被移动的对象（超出"重新赋值或销毁"）| [02-03](../part2_memory/02-03_move_semantics.md) |
| 数据竞争 | [08-01](../part8_concurrency/08-01_thread.md) |
| 析构函数抛异常（栈展开期间）| [01-08](../part1_basics/01-08_exception.md) |
| `operator[]` 越界 | [07-01](../part7_stdlib/07-01_string.md) |
| 空 `optional` 的 `*opt` | [07-04](../part7_stdlib/07-04_tuple_variant.md) |
| 悬垂的 `string_view` | [07-01](../part7_stdlib/07-01_string.md) |
| 迭代器失效后继续使用 | [07-02](../part7_stdlib/07-02_containers.md) |
| 构造函数中调用纯虚函数 | [04-04](../part4_oop/04-04_virtual.md) |
| 零长度数组 | [05-04](../part5_template/05-04_variadic.md) |

**检测工具**：

```bash
-fsanitize=address,undefined     # 内存错误 + UB
-fsanitize=thread                 # 数据竞争（不能和 address 同时用）
-D_GLIBCXX_DEBUG                  # libstdc++ 的迭代器调试
```

> 下一章：[09-03 现代 C++ 编码清单](09-03_guidelines.md)
