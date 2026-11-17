# 02-02 new/delete 表达式

> demo：[`02_02_new_delete.cpp`](../../demos/part2_memory/02_02_new_delete.cpp)

## 一、解决了什么问题

C 用 `malloc`/`free` 管理堆空间。对 C 够用，对 C++ 不够 ——
因为 C++ 有**构造函数和析构函数**，而 `malloc` 完全不知道它们存在。

```cpp
// 用 malloc 创建对象会发生什么
void* raw = std::malloc(sizeof(Widget));
Widget* w = static_cast<Widget*>(raw);
w->use();     // 未定义行为：构造函数从未运行，成员是垃圾值
std::free(w); // 析构函数也不会运行，成员持有的资源全部泄漏
```

C++ 提供 `new`/`delete` 表达式来填这个缺口。

### new/delete 与 malloc/free 的区别（常考题）

1. **`malloc`/`free` 是标准库函数，`new`/`delete` 是运算符（表达式）**。
   前者可以被重定义但语法固定，后者是语言的一部分，能被重载
2. **`new` 能自动计算空间大小，`malloc` 需要传参**

   ```cpp
   int* p1 = new int;                                      // 不用 sizeof
   int* p2 = static_cast<int*>(std::malloc(sizeof(int)));  // 必须写
   ```
3. **`new` 开辟空间的同时可以做初始化，`malloc` 不行**
4. **`new`/`delete` 会调用构造函数和析构函数，`malloc`/`free` 不会** ——
   这是最关键的一条
5. **类型安全**：`new` 返回正确类型的指针，`malloc` 返回 `void*` 需要手工转换
6. **失败行为**：`new` 失败抛 `std::bad_alloc`，`malloc` 失败返回 `NULL`

实测输出：

```
  new 会调用构造函数:
    Widget() id=1
  delete 会调用析构函数:
    ~Widget() id=1
  malloc 不调用构造函数（对象未初始化，用它是未定义行为）:
```

既然 `new`/`delete` 的功能完全覆盖了 `malloc`/`free`，为什么 C++ 还保留后者？
因为 C++ 程序经常要调用 C 函数，而 C 程序只能用 `malloc`/`free` 管理动态内存。
此外 `realloc` 没有 C++ 对应物，自定义分配器也常基于 `malloc`。

**不能混用**：`malloc` 的内存必须 `free`，`new` 的必须 `delete`。
混用是未定义行为。

## 二、new 表达式的三个步骤

使用 `new` 表达式时发生三件事：

1. **调用名为 `operator new` 的标准库函数**，分配足够大的、原始的、
   未类型化的内存，以保存指定类型的一个对象
2. **运行该类型的一个构造函数**初始化对象
3. **返回指向新分配并构造的对象的指针**

拆开看就是：

```cpp
Widget* w = new Widget(arg);

// 大致等价于
void* raw = operator new(sizeof(Widget));   // 步骤 1：分配
Widget* w = static_cast<Widget*>(raw);
new (raw) Widget(arg);                      // 步骤 2：构造（placement new）
// 步骤 3：返回 w
```

这个拆分解释了一件重要的事：**如果步骤 2 抛异常，步骤 1 分配的内存会被自动释放**。
语言保证了这一点，所以 `new` 不会因为构造函数抛异常而泄漏内存。

## 三、delete 表达式的两个步骤

1. **调用析构函数**，回收对象中数据成员所申请的资源
2. **调用名为 `operator delete` 的标准库函数**，释放该对象所用的内存

顺序和 `new` 相反：先析构，后释放内存。

### delete 空指针是安全的

```cpp
Widget* p = nullptr;
delete p;        // 合法，什么也不做
```

所以不需要写 `if (p) delete p;` —— 这个检查是多余的。

### delete 和 delete[] 必须配对

```cpp
Widget* arr = new Widget[2];
delete[] arr;      // 正确：逆序调用每个元素的析构函数
// delete arr;     // 未定义行为
```

实测数组的析构顺序：

```
    Widget() id=2        <- 构造：正序
    Widget() id=3
    ~Widget() id=3       <- 析构：逆序
    ~Widget() id=2
```

为什么用错会出问题？因为 `new[]` 通常会在返回的指针**前面**多存一个元素个数
（叫 cookie），`delete[]` 要靠它知道该析构多少个对象。
用 `delete` 删数组：只析构第一个元素，而且传给 `operator delete`
的指针位置可能不对，堆结构损坏。反过来用 `delete[]` 删单对象，
会把对象前面的数据当成元素个数来读，结果同样灾难。

这是裸指针管理数组最容易出的错，也是应该用 `std::vector`
或 `std::unique_ptr<T[]>` 的理由之一。

## 四、new 的初始化形式

```cpp
int* p1 = new int;        // 不初始化，值不确定（读它是未定义行为）
int* p2 = new int();      // 零初始化 -> 0
int* p3 = new int{};      // 零初始化 -> 0   [C++11]
int* p4 = new int(42);    // 指定值
int* p5 = new int{42};    // 指定值          [C++11]
```

数组版本：

```cpp
int* a1 = new int[5];        // 5 个未初始化的 int
int* a2 = new int[5]();      // 5 个 0
int* a3 = new int[5]{1, 2};  // [C++11] 1,2,0,0,0 —— 不足的部分零初始化
```

实测：

```
  new int()  -> 0
  new int{}  -> 0
  new int[5]{1,2} 全部: 1 2 0 0 0
```

注意 `new int` 和 `new int()` 的区别只影响**基本类型**。
对有构造函数的类，两者都会调用默认构造函数。

对象数组的一个限制：用 `new` 为对象数组分配空间时，
不能显式调用带参数的构造函数。因此对象要么没有定义任何构造函数
（由编译器提供默认的），要么显式定义了一个所有参数都有默认值的构造函数。
C++11 的 `new T[n]{...}` 部分缓解了这个限制。

## 五、分配失败

`new` 失败时抛出 `std::bad_alloc`：

```cpp
try {
    int* p = new int[absurd_size()];
} catch (const std::bad_alloc& e) {
    // 实测: std::bad_alloc
}
```

不想要异常，用 `nothrow` 版本：

```cpp
int* p = new (std::nothrow) int[absurd_size()];
if (p == nullptr) { /* 分配失败 */ }
```

实测输出：

```
  new 失败抛出 bad_alloc: std::bad_alloc
  new(std::nothrow) 失败时返回 nullptr
```

写这个 demo 时我踩到一个细节值得记下来：数组长度**超过 `PTRDIFF_MAX / sizeof(T)`**
时，抛的是 `std::bad_array_new_length`，而且这个检查在分配动作**之前**，
所以连 `nothrow` 版本也会抛异常而不是返回 `nullptr`。
要演示 `bad_alloc`，得选一个"能通过长度检查但系统分配不出来"的大小。

另外，Linux 默认开启内存**过度提交（overcommit）**，
`new` 一大块内存常常会成功返回，真正使用时才因为没有物理内存而被 OOM killer 杀掉。
所以在 Linux 上靠检查 `bad_alloc` 来处理内存不足，可靠性有限。

## 六、重载 operator new / operator delete

`operator new` 和 `operator delete` 是可以重载的函数：

```cpp
// 全局版本的签名
void* operator new(std::size_t);
void* operator new[](std::size_t);
void operator delete(void*) noexcept;
void operator delete[](void*) noexcept;
```

重载它们的用途：内存池、分配统计、调试用的越界检测、特殊对齐要求。

### 类专属的重载

```cpp
class Student {
public:
    void* operator new(std::size_t sz) {
        std::cout << "Student::operator new(" << sz << ")\n";
        return std::malloc(sz);
    }
    void operator delete(void* p) {
        std::cout << "Student::operator delete\n";
        std::free(p);
    }
    // 数组版本是独立的，不会因为重载了单对象版本而自动生效
    void* operator new[](std::size_t sz) { return std::malloc(sz); }
    void operator delete[](void* p) { std::free(p); }
};
```

实测执行顺序清楚地展示了 new/delete 的分工：

```
    Student::operator new(16) 被调用     <- 步骤1：分配内存
    Student() 构造                       <- 步骤2：构造对象
    id=100 name=Jackie
    ~Student() 析构                      <- delete 步骤1：析构
    Student::operator delete 被调用       <- delete 步骤2：释放内存
```

几个要点：

- `operator new` 的第一个参数是 `size_t`，由编译器自动传入 `sizeof(T)`，
  **不要自己传**
- 它们**隐式是静态成员函数**（不用写 `static`），因为调用时对象还不存在
- 单对象版本和数组版本是独立的，重载一个不影响另一个
- 重载了 `operator new` 就应该重载对应的 `operator delete`，否则不配套
- C++11 起，推荐用 `= delete` 而不是设为 private 来禁用它们

## 七、placement new：在已有内存上构造对象

```cpp
void* operator new(std::size_t, void* p) noexcept { return p; }   // 标准库提供
```

它什么也不分配，直接在你给的地址上构造对象：

```cpp
alignas(Widget) char buffer[sizeof(Widget) * 2];

Widget* w1 = new (buffer) Widget;                     // 在 buffer 开头构造
Widget* w2 = new (buffer + sizeof(Widget)) Widget;    // 在后面构造第二个

// 必须手工调用析构函数，且绝不能 delete
w2->~Widget();
w1->~Widget();
```

实测确认地址相同：

```
    w1 在 0x7ffed5efbee0, buffer 在 0x7ffed5efbee0（相同）
```

注意三件事：

1. **内存必须正确对齐** —— 用 `alignas(T)` 或 `std::aligned_storage`
2. **必须显式调用析构函数**（`p->~T()`），它只析构不释放内存
3. **绝不能对它用 `delete`** —— 那块内存不是 `operator new` 分配的

placement new 是标准库的基础设施。`std::vector` 就靠它实现
"预分配容量，按需构造元素"：`reserve(100)` 只分配内存不构造对象，
`push_back` 时才用 placement new 在对应位置构造。
`std::optional`、`std::variant`、小对象优化、内存池全都依赖它。

C++17 之后，手写 placement new 的场合变少了 ——
`std::construct_at`（C++20）、`std::uninitialized_copy` 等算法更安全。

## 八、限制对象的创建位置

利用上面的机制，可以控制一个类只能在堆上或只能在栈上创建。

### 只能创建栈对象

把 `operator new` 禁用即可 —— 没有分配函数，`new` 表达式就无法工作：

```cpp
class StackOnly {
public:
    StackOnly() = default;
private:
    void* operator new(std::size_t) = delete;     // [C++11] 比设为 private 更明确
    void operator delete(void*) = delete;
};

StackOnly s;                      // ok
// StackOnly* p = new StackOnly;  // 编译错误
```

参考文档里的做法是把 `operator new` 放进 private 区域，效果类似。
C++11 的 `= delete` 更好：错误信息更清楚（"use of deleted function"
而不是"is private"），而且意图明确。

### 只能创建堆对象

把析构函数设为 private。栈对象离开作用域时需要自动调析构函数，
访问不到就编译失败：

```cpp
class HeapOnly {
public:
    static HeapOnly* create() { return new HeapOnly; }
    void destroy() { delete this; }
private:
    HeapOnly() = default;
    ~HeapOnly() = default;        // 关键：析构函数私有
};

HeapOnly* h = HeapOnly::create();
h->destroy();
// HeapOnly stack_obj;            // 编译错误
```

实测输出：

```
  只能建堆对象:
    HeapOnly() 构造
    HeapOnly::use()
    ~HeapOnly() 析构
  只能建栈对象:
    StackOnly() 构造
    StackOnly::use()
    ~StackOnly() 析构
```

这两个技巧在实际工程里用得不多（限制堆对象的场合主要是引用计数类型，
限制栈对象的场合主要是需要控制生命周期的句柄类），
但它们很好地说明了 `operator new` 和析构函数的可见性如何影响对象的创建方式。

`delete this` 需要小心：调用之后对象已不存在，不能再访问任何成员，
也不能让该对象是栈对象或被智能指针管理。

## 九、现代 C++ 的立场：尽量不要直接用 new/delete

本章讲了 `new`/`delete` 的全部细节，但现代 C++ 的建议是**避免直接使用它们**。

理由在 [01-08 异常](../part1_basics/01-08_exception.md) 已经讲过 ——
只要中间可能抛异常，手工 `delete` 就不可靠。再加上本章提到的
`delete`/`delete[]` 配对问题、double free、忘记释放，
裸的 `new`/`delete` 是 C++ 最大的 bug 来源。

替代方案：

| 需求 | 替代品 |
|---|---|
| 单个对象 | `std::make_unique<T>(args...)` `[C++14]` |
| 共享所有权 | `std::make_shared<T>(args...)` |
| 动态数组 | `std::vector<T>` |
| 固定大小数组 | `std::array<T, N>` |
| 字符串 | `std::string` |

```cpp
// 不要
Widget* w = new Widget;
// ... 中间任何 return 或异常都会泄漏
delete w;

// 要
auto w = std::make_unique<Widget>();
// 自动释放，异常安全
```

什么时候还需要直接用？实现底层设施时 ——
自己写容器、内存池、智能指针，或者对接只接受裸指针的 C API。
应用层代码里出现 `new`/`delete`，基本都可以改掉。

详见 [02-05 智能指针](02-05_smart_pointers.md) 和 [02-06 RAII](02-06_raii.md)。

## 十、小结

| 要点 | 说明 |
|---|---|
| new 三步 | 调 `operator new` 分配 → 调构造函数 → 返回指针 |
| delete 两步 | 调析构函数 → 调 `operator delete` 释放 |
| vs malloc | new 是运算符、自动算大小、能初始化、**会调用构造/析构函数**、类型安全、失败抛异常 |
| 不能混用 | `malloc`↔`free`，`new`↔`delete`，`new[]`↔`delete[]` |
| `delete nullptr` | 安全，不需要判空 |
| 初始化 | `new T` 不初始化；`new T()` / `new T{}` 零初始化 |
| 失败 | 抛 `bad_alloc`；`new(nothrow)` 返回 `nullptr`；长度非法抛 `bad_array_new_length` |
| `operator new` 重载 | 隐式静态；单对象和数组版本独立；要配套重载 delete |
| placement new | 在已有内存构造；需手工 `p->~T()`；不能 delete；是容器的基础设施 |
| 限制创建位置 | 禁用 `operator new` → 只能建栈对象；析构函数私有 → 只能建堆对象 |
| **现代建议** | 优先 `make_unique`/`make_shared`/`vector`，避免直接 new/delete |

> 下一章：[02-03 右值引用与移动语义](02-03_move_semantics.md)
