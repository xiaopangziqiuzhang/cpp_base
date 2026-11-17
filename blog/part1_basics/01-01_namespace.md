# 01-01 命名空间

> 标准：`namespace` C++98、`inline namespace` `[C++11]`、嵌套命名空间简化 `[C++17]`
> demo：[`01_01_namespace.cpp`](../../demos/part1_basics/01_01_namespace.cpp)、
> [`01_01_using_pitfall.cpp`](../../demos/part1_basics/01_01_using_pitfall.cpp)

## 一、解决了什么问题

C 语言只有一个全局作用域。一个大型工程由多人分头开发，各自写的头文件里
可能用了相同的名字来命名类或函数，拼到一起就会**名字冲突** ——
同一个作用域里出现了两个或多个同名实体。

这不只是"自己人之间撞名"。你定义的名字还可能和标准库撞。
C 的应对办法是给所有公开符号加前缀，于是出现了 `OSSL_PARAM_construct_int`、
`cairo_image_surface_create` 这类手工命名空间：

```c
/* C 的办法：把命名空间写进名字里 */
int  mylib_parse(const char* s);
void mylib_free(void* p);
struct mylib_context { /* ... */ };
```

这种办法能用，但有代价：名字变长、无法按需缩短、编译器完全不理解这层结构，
撞名了还是撞名。

C++ 的办法是把作用域变成语言特性 —— `namespace` 允许用户自己定义作用域，
不同作用域里可以有相同的名字，编译器能够区分它们。

## 二、什么是命名空间

命名空间是程序员命名的一块作用域。把全局实体分别放进各个命名空间，
它们就彼此隔开了。放在命名空间里的实体称为该空间的成员。

```cpp
namespace wd {
    int number = 0;
    const double kPi = 3.14159;

    struct Foo { char ch; int val; };

    void display();                       // 只声明

    template <class T>
    T twice(T t) { return t + t; }

    namespace inner { int number = 100; } // 可以嵌套
}
```

命名空间里可以放：变量、常量、函数（声明或定义）、结构体、类、模板、以及嵌套的命名空间。

在命名空间外定义其成员，用限定名：

```cpp
void wd::display() {
    std::cout << "wd::display(), number=" << number << "\n";  // 空间内的名字可直接用
}
```

有一点要分清：**命名空间成员的作用域是全局的，但可见域不是**。
不借助作用域限定符或 using 机制，实体只在"从声明处到该命名空间结束"这段范围内可见；
在命名空间外部，它是不可见的。

命名空间还有一个常被忽略的性质：**它是开放的，可以分多次打开**。

```cpp
// a.h
namespace wd { void func_a(); }
// b.h
namespace wd { void func_b(); }   // 追加到同一个 wd，不是重定义
```

这就是标准库能把 `std` 分散在上百个头文件里的原因。

## 三、三种使用方式

### 3.1 using 指令（using directive）

```cpp
#include <iostream>
using namespace std;          // 一次性引入 std 的所有名字

int main() {
    cout << "hello,world" << endl;
}
```

这是最常见的入门写法，也是**最该避免**的一种。它把整个命名空间的名字一次性引入，
如果你对这个空间里有什么并不熟悉，很容易撞名，而且出错后不好定位。

下面这段就会报错：

```cpp
using namespace std;
double cout() { return 1.1; }   // 与 std::cout 冲突
int main() { cout(); }
```

### 3.2 作用域限定符

每次使用都显式写出 `::`：

```cpp
namespace wd {
    int number = 10;
    void display() {
        std::cout << "wd::display()" << std::endl;   // cout/endl 都显式限定
    }
}

int main() {
    std::cout << "wd::number = " << wd::number << std::endl;
    wd::display();
}
```

啰嗦，但永远不会有歧义。

### 3.3 using 声明（using declaration）

只引入需要的那几个名字，作用域从 using 语句开始到所在作用域结束：

```cpp
using std::cout;
using std::endl;

int main() {
    cout << "ok" << endl;
}
```

**这是推荐的方式**：需要哪个就引入哪个，不需要的不引入，把犯错概率压到最小。
注意同一作用域内，用 using 声明引入的不同命名空间的成员不能同名，否则是重定义。

## 四、`using namespace std` 到底坏在哪

"不要 `using namespace std`"这条建议流传很广，但多数说法停在"会污染全局"。
下面是三个能具体复现的后果（见
[`01_01_using_pitfall.cpp`](../../demos/part1_basics/01_01_using_pitfall.cpp)）。

### 后果一：二义性

```cpp
namespace bad_example {
    int count(const std::vector<int>& v) { return v.size(); }
}

void f() {
    using namespace std;          // 引入 std::count
    using namespace bad_example;  // 引入 bad_example::count

    std::vector<int> v{1, 2, 2, 3};
    int n = count(v);   // 编译错误: reference to 'count' is ambiguous
}
```

### 后果二：升级标准后，没改的代码突然编不过

这是最阴的一种。C++17 新增了 `std::clamp`。如果你的项目里早就有自己的 `clamp`，
并且某处写了 `using namespace std;`：

```cpp
namespace legacy {
    template <class T>
    const T& clamp(const T& v, const T& lo, const T& hi);
}

using namespace legacy;
using namespace std;     // C++17 起这里多出一个 std::clamp

clamp(5, 1, 3);          // C++14 下编得过，C++17 下二义性
```

代码一行没动，仅仅把 `-std=c++14` 换成 `-std=c++17` 就崩了。
标准库每一版都在加新名字（`std::byte`、`std::size`、`std::data`、`std::clamp`、
`std::gcd`……），`using namespace std` 等于把你的项目暴露在所有未来新增名字之下。

### 后果三：重载决议被悄悄改变

```cpp
namespace mymath {
    int abs(int x) { return x < 0 ? -x : x; }   // 只有 int 版本
}
using namespace mymath;

abs(-3.7);   // 没有浮点版本可选，-3.7 被隐式转成 int
```

实测输出：

```
mymath::abs(-3.7) = 3  <- 截断成 3
std::abs(-3.7)    = 3.7
```

这个不会报错，只会算错。

### 指导原则

```cpp
// 绝对不要：头文件里，或任何全局作用域
using namespace std;

// 可以接受：函数内部，范围明确
void f() {
    using std::cout;
    using std::sort;
    // ...
}
```

**头文件里禁止使用 using 指令**。这会让可用名字变得模糊、容易出现二义性，
而且包含头文件的顺序会影响程序行为 —— 调试这种问题极其痛苦。
如果非要用，放在所有 `#include` 之后。

## 五、匿名命名空间

不写名字的命名空间叫匿名命名空间。它里面的实体其它文件无法引用，
只在本文件内有效，作用域从声明处到文件结束：

```cpp
namespace {
    int file_local_counter = 0;
    void bump() { ++file_local_counter; }
}
```

它和 `static` 是同样的道理 —— 都只在本文件内有效 ——
但匿名命名空间是**更好的替代手段**，原因有两条：

1. `static` 一词在 C++ 里已经承载了太多含义（静态存储期、类的静态成员、
   函数内静态局部变量、内部链接），匿名命名空间只表达"内部链接"这一件事
2. `static` 不能作用于类型定义，匿名命名空间可以把类型也藏起来

```cpp
namespace {
    struct Helper { int x; };   // 这个类型其他翻译单元看不到
}
// static struct Helper2 { int x; };   // static 做不到这件事
```

在匿名空间中创建的变量具有全局生存期，却只能被本文件内的代码访问。

## 六、嵌套、遮蔽与 `::`

内层的同名实体会遮蔽外层的。想访问被遮蔽的名字，就用限定名；
想访问全局作用域，用前缀 `::`：

```cpp
int number = 1;                 // 全局

namespace outer {
    int number = 10;
    namespace nested {
        int number = 100;
        void show() {
            std::cout << number;          // 100，最内层
            std::cout << outer::number;   // 10
            std::cout << ::number;        // 1，一个 :: 打头表示全局
        }
    }
}
```

实测输出：

```
nested::number   = 100
outer::number    = 10
::number(global) = 1
```

### 嵌套命名空间的简化写法 `[C++17]`

深层嵌套在 C++98 下写起来很笨重：

```cpp
// C++98：三层缩进，三个右括号
namespace company {
    namespace project {
        namespace module {
            int version = 1;
        }
    }
}

// C++17：一行
namespace company::project::module {
    int version = 1;
}
```

### 命名空间别名

限定名太长时，用别名缩短：

```cpp
namespace very_long_library_name_v2 { void work(); }
namespace lib = very_long_library_name_v2;

lib::work();
```

这在用 `std::filesystem` 时几乎是标配：

```cpp
namespace fs = std::filesystem;
```

## 七、ADL：为什么 `cout << v` 不用写 `std::`

有个现象值得解释：你给自己的类型重载 `operator<<`，放在自己的命名空间里，
用的时候却不需要限定：

```cpp
namespace geometry {
    struct Vec2 { double x, y; };
    std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        return os << "(" << v.x << ", " << v.y << ")";
    }
}

geometry::Vec2 v{1.5, 2.5};
std::cout << v;   // 能找到 geometry::operator<<
```

这是 **ADL（Argument Dependent Lookup，参数依赖查找）**，也叫 Koenig 查找：
查找函数名时，除了常规作用域，编译器还会把**实参类型所属的命名空间**纳入查找范围。
因为 `v` 的类型是 `geometry::Vec2`，所以 `geometry` 被加入了查找集。

实测输出：

```
ADL found operator<<: (1.5, 2.5)
```

ADL 的实际意义：**运算符重载和非成员的"接口函数"应该和类型放在同一个命名空间**。
这样用户不用限定就能用，这也是 `std::swap` 惯用法的基础：

```cpp
using std::swap;   // 先引入兜底版本
swap(a, b);        // 若 a 的命名空间里有更优的 swap，ADL 会选它
```

## 八、inline namespace：库的版本演进 `[C++11]`

`inline namespace` 的成员会被"提升"到外层命名空间，像是直接定义在外层一样：

```cpp
namespace mylib {
    namespace v1 {
        const char* api() { return "v1::api"; }
    }
    inline namespace v2 {              // 当前默认版本
        const char* api() { return "v2::api"; }
    }
}
```

实测输出：

```
mylib::api()     = v2::api
mylib::v1::api() = v1::api
mylib::v2::api() = v2::api
```

`mylib::api()` 直接解析到 `v2`。它解决的问题是**库的版本平滑演进**：

- 用户写 `mylib::api()`，自动用到最新版
- 需要锁定旧版的用户，显式写 `mylib::v1::api()`
- 发布新版本时，把 `inline` 挪到 `v3` 上即可，用户代码不用改
- 关键之处在于：不同版本的符号在 ABI 层面是**不同的名字**（mangled name 含命名空间），
  所以链接期能检测出版本混用，而不是运行时诡异崩溃

libstdc++ 就用这个机制处理 C++11 的 ABI 变更（`std::__cxx11::basic_string`）。

## 九、小结

| 机制 | 写法 | 何时用 |
|---|---|---|
| using 指令 | `using namespace X;` | 几乎不用；只在函数内、且 X 很小时 |
| 作用域限定符 | `X::name` | 默认选择，尤其头文件里 |
| using 声明 | `using X::name;` | 函数内引入少量高频名字 |
| 匿名命名空间 | `namespace { ... }` | 文件内私有的变量/函数/类型，取代 `static` |
| 命名空间别名 | `namespace fs = ...;` | 限定名过长 |
| 嵌套简化 `[C++17]` | `namespace a::b::c {}` | 多层嵌套 |
| inline namespace `[C++11]` | `inline namespace v2 {}` | 库的版本演进 |

流行的使用指导原则：

- 提倡在已命名的命名空间中定义变量，而不是直接定义全局变量或静态全局变量
- 开发函数库或类库时，提倡将其放进一个命名空间
- 对于 using 声明，首先把作用域设为局部而非全局
- **不要在头文件中使用 using 指令**；非要用，放在所有 `#include` 之后
- 运算符重载和接口函数要和类型放在同一命名空间（让 ADL 工作）

> 下一章：[01-02 const 与 constexpr](01-02_const_constexpr.md)
