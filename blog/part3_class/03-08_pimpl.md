# 03-08 Pimpl 惯用法

> demo：[`03_08_pimpl.cpp`](../../demos/part3_class/03_08_pimpl.cpp)

## 一、解决了什么问题

C++ 的头文件必须包含类的**完整定义**，包括所有私有成员。
因为编译器要知道对象的大小和布局，才能生成创建对象的代码。

这导致了一个结构性矛盾：**私有成员在语义上是隐藏的，在物理上却是公开的。**

```cpp
// WithoutPimpl.h —— 用户必须看到的内容
#include <heavy_lib/big_config.h>     // 被迫传递依赖
#include <heavy_lib/connection.h>
#include <vector>
#include <string>

class WithoutPimpl {
public:
    explicit WithoutPimpl(std::string name);
    void work() const;
private:
    std::string _name;                   // 用户看得到
    heavy_lib::BigConfig _config;         // 用户必须 include BigConfig 的头文件
    heavy_lib::Connection _conn;
    std::vector<int> _cache;
};
```

具体的三个问题：

**问题一：依赖传递。** 头文件里的每个成员类型都要 `#include`，
这些 include 又会带来它们自己的 include。
用户只想用 `WithoutPimpl`，却被迫让整个依赖树进入自己的翻译单元。
大型项目里，一个头文件展开后几万行是常事，编译时间直接受影响。

**问题二：重编译传染。** 只要动了私有成员（加一个字段、换一个容器类型），
对象的大小和布局就变了，**所有包含这个头文件的 `.cpp` 都必须重新编译**。
在一个几千文件的项目里，改一行私有成员可能触发半小时的全量重编。

**问题三：ABI 不稳定。** 发布动态库时，私有成员的任何变化都会改变对象布局。
老的调用方不重新编译就会内存错乱 —— 这叫 ABI 破坏。
这也是为什么很多 C++ 库的升级要求用户重新编译。

**Pimpl（Pointer to implementation，也叫编译防火墙、Cheshire Cat）**
把全部实现细节挪到一个只有源文件可见的类里。

## 二、写法

### 头文件：用户看到的全部内容

```cpp
// WithPimpl.h
#include <memory>
#include <string>

class WithPimpl {
public:
    explicit WithPimpl(std::string name);
    ~WithPimpl();                                 // 必须声明！原因见第四节

    WithPimpl(WithPimpl&&) noexcept;              // 声明在头文件
    WithPimpl& operator=(WithPimpl&&) noexcept;
    WithPimpl(const WithPimpl&);
    WithPimpl& operator=(const WithPimpl&);

    void work() const;
    void add_option(const std::string& opt);

private:
    class Impl;                                   // 仅前向声明
    std::unique_ptr<Impl> _impl;                  // 唯一的数据成员
};
```

注意头文件里**只有 `<memory>` 和 `<string>`** ——
`heavy_lib` 的头文件不再出现。

### 源文件：实现细节

```cpp
// WithPimpl.cpp
#include "WithPimpl.h"
#include <heavy_lib/big_config.h>     // 依赖只在这里
#include <heavy_lib/connection.h>
#include <vector>

class WithPimpl::Impl {
public:
    explicit Impl(std::string name) : _name(std::move(name)) {}
    void work() const { /* ... */ }
    void add_option(const std::string& opt) { _config.options.push_back(opt); }
private:
    std::string _name;
    heavy_lib::BigConfig _config;      // 全部实现细节藏在这里
    heavy_lib::Connection _conn;
    std::vector<int> _cache;
};

WithPimpl::WithPimpl(std::string name)
    : _impl(std::make_unique<Impl>(std::move(name))) {}

WithPimpl::~WithPimpl() = default;     // 在这里定义，此时 Impl 是完整类型

// 转发函数：每个公开接口写一行
void WithPimpl::work() const { _impl->work(); }
void WithPimpl::add_option(const std::string& opt) { _impl->add_option(opt); }
```

## 三、效果

实测对象大小的对比：

```
      sizeof(WithoutPimpl) = 120 字节（随成员增加而变化）
      sizeof(WithPimpl) = 8 字节（永远是一个指针大小，与实现无关）
```

`WithPimpl` 永远是 8 字节（一个指针），**无论 `Impl` 里有多少成员**。
这正是 ABI 稳定的来源 —— 实现怎么改，对象布局都不变。

### 验证编译防火墙

我用一个真实的多文件例子验证过：

```
main_good.o 只依赖 good.h，Impl 怎么改都不用重编
```

`main.cpp` 只 `#include "WithPimpl.h"`，而这个头文件不包含任何实现细节。
修改 `Impl` 的成员只需要重新编译 `WithPimpl.cpp` 一个文件。

### 代价

Pimpl 不是免费的：

| 代价 | 说明 |
|---|---|
| **一次额外的堆分配** | 每个对象都要 `make_unique<Impl>` |
| **一次额外的间接寻址** | 每次访问成员要先解引用 `_impl` |
| **无法内联** | 转发函数的实现在 `.cpp` 里，跨翻译单元无法内联（除非开 LTO）|
| **样板代码** | 每个公开接口都要写一行转发 |
| **对象无法放在栈上内联展开** | 实现部分总在堆上 |

所以它适合**接口类、库的边界类、编译时间敏感的大类**，
不适合小而热的值类型（`Point`、`Vector3` 这种用 Pimpl 是灾难）。

## 四、不完整类型问题（最容易踩的坑）

这是 Pimpl 唯一需要特别注意的地方。

### 必须声明析构函数

如果头文件里**不声明**析构函数，编译器会在**使用处**（用户的 `.cpp` 里）
生成一个隐式析构函数。而那里 `Impl` 只有前向声明，是**不完整类型** ——
`unique_ptr` 无法 `delete` 一个不完整类型。

实测错误信息：

```
error: invalid application of 'sizeof' to incomplete type 'Bad::Impl'
   83 |         static_assert(sizeof(_Tp)>0,
...required from 'std::unique_ptr<_Tp, _Dp>::~unique_ptr()
                  [with _Tp = Bad::Impl; ...]'
```

`unique_ptr` 的默认删除器里有个 `static_assert(sizeof(_Tp) > 0)` ——
这是它为了给出明确错误而特意加的检查。
如果没有这个断言，`delete` 一个不完整类型是未定义行为
（不会调用析构函数，静默泄漏）。

解法：**在头文件里声明，在源文件里定义**：

```cpp
// 头文件
~WithPimpl();

// 源文件（此时 Impl 已完整）
WithPimpl::~WithPimpl() = default;
```

验证通过：

```
=== 声明了析构函数后 ===
Impl::work
编译运行成功
```

### 其他也要挪到源文件的函数

同样的道理适用于所有"需要 `Impl` 是完整类型"的函数：

| 函数 | 为什么 |
|---|---|
| 析构函数 | 要 `delete Impl` |
| 移动构造 / 移动赋值 | 移动赋值隐含要销毁旧的 `Impl` |
| 拷贝构造 / 拷贝赋值 | 要构造新的 `Impl` |

所以标准写法是：**头文件只声明这五个，源文件给定义**。

```cpp
// 源文件
WithPimpl::~WithPimpl() = default;
WithPimpl::WithPimpl(WithPimpl&&) noexcept = default;
WithPimpl& WithPimpl::operator=(WithPimpl&&) noexcept = default;
```

注意一个细节：声明了析构函数之后，移动函数就不再自动生成了
（见 [03-04 的生成规则](03-04_copy_control.md)）——
所以必须显式声明它们，否则 Pimpl 类会意外地变成不可移动的。

### 拷贝需要手写

`unique_ptr` 不可拷贝，所以默认的拷贝构造也不可用。
要支持拷贝就得手写深拷贝：

```cpp
WithPimpl::WithPimpl(const WithPimpl& rhs)
    : _impl(rhs._impl ? std::make_unique<Impl>(*rhs._impl) : nullptr) {}

WithPimpl& WithPimpl::operator=(const WithPimpl& rhs) {
    if (this != &rhs) {
        _impl = rhs._impl ? std::make_unique<Impl>(*rhs._impl) : nullptr;
    }
    return *this;
}
```

实测确认是深拷贝：

```
    拷贝（深拷贝 Impl）:
      a.option_count()=1 b.option_count()=2（互不影响，说明是深拷贝）
    移动（只转移指针）:
      移动后 c.option_count()=1
```

注意要处理 `_impl` 为空的情况 —— 对象被移动之后 `_impl` 就是空的。

## 五、变体与相关技术

### 用 shared_ptr 实现"共享实现"

```cpp
std::shared_ptr<Impl> _impl;      // 拷贝时共享实现
```

这样拷贝变成 O(1)，但多个对象共享状态 —— 语义从值类型变成了引用类型。
`shared_ptr` 的析构函数不要求完整类型（删除器在构造时就捕获了），
所以可以不声明析构函数。但大多数时候值语义才是你想要的。

### 反向 Pimpl：接口 + 抽象基类

另一种达到编译防火墙效果的办法是纯虚接口：

```cpp
// 头文件
class IService {
public:
    virtual ~IService() = default;
    virtual void work() = 0;
};
std::unique_ptr<IService> create_service();     // 工厂函数

// 源文件：具体实现类完全不可见
```

对比：

| | Pimpl | 抽象接口 |
|---|---|---|
| 对象语义 | 值类型（可拷贝、可栈上） | 必须通过指针 |
| 运行时开销 | 一次间接 | 虚调用 |
| 多实现 | 不支持 | 天然支持 |
| 适合 | 单一实现的具体类 | 需要多态/插件 |

### 标准库的做法

`std::pmr` 的某些实现、各种平台封装层（Qt 的 `Q_DECLARE_PRIVATE`
就是 Pimpl 的宏化版本）都在用这个惯用法。
Qt 之所以大量使用，正是因为它要维护跨版本的 ABI 兼容。

## 六、什么时候用

**适合：**

- 库的公开接口类（需要 ABI 稳定）
- 实现依赖重量级第三方头文件（减少依赖传递）
- 编译时间已经是问题的大类
- 实现细节经常变动的类

**不适合：**

- 小而热的值类型（`Point`、`Complex`、`Vector3`）——
  一次堆分配和一次间接寻址的代价相对太大
- 模板类 —— 模板的实现必须在头文件里，Pimpl 用不上
- 需要内联的性能关键类
- 只有少量简单成员、依赖也简单的类 —— 收益不足

实践判断：**先不用，等编译时间或 ABI 成为真问题时再引入。**
Pimpl 是解决具体问题的工具，不是默认的类设计方式。

## 七、小结

| 要点 | 说明 |
|---|---|
| 解决什么 | 头文件暴露私有成员 → 依赖传递、重编译传染、ABI 不稳定 |
| 做法 | 头文件里只有 `class Impl;` 前向声明 + `unique_ptr<Impl>` |
| 效果 | 对象永远是一个指针大小；实现改动只需重编一个 `.cpp` |
| **必须注意** | 析构函数、移动函数、拷贝函数都要**在源文件里定义** |
| 为什么 | `Impl` 在头文件里是不完整类型，`unique_ptr` 无法 delete |
| 错误信息 | `invalid application of 'sizeof' to incomplete type` |
| 连带影响 | 声明了析构函数 → 移动函数不再自动生成 → 必须显式声明 |
| 拷贝 | 必须手写深拷贝，并处理 `_impl` 为空的情况 |
| 代价 | 一次堆分配 + 一次间接寻址 + 无法内联 + 样板代码 |
| 不适合 | 小而热的值类型、模板类 |

> 下一章：[03-09 友元与运算符重载](03-09_operator_overload.md)
