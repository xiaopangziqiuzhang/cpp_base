# 03-06 const 成员函数与 mutable

> demo：[`03_06_const_member.cpp`](../../demos/part3_class/03_06_const_member.cpp)

## 一、const 成员函数到底发生了什么

把 `const` 放在参数表和函数体之间，得到 const 成员函数：

```cpp
类型 函数名(参数列表) const { 函数体 }
```

它有两个约束：

1. 只能读取数据成员，不能修改
2. 只能调用其它 const 成员函数，不能调用非 const 成员函数

参考文档在这里留了个思考题：**定义一个 const 成员函数时，到底发生了什么？**

答案是：**`this` 指针的类型变了。**

| 成员函数 | `this` 的类型 |
|---|---|
| 普通成员函数 | `T* const this` |
| const 成员函数 | `const T* const this` |

实测验证：

```cpp
void non_const() {
    // decltype(this) 是 const Probe* 吗: 0
}
void is_const() const {
    // decltype(this) 是 const Probe* 吗: 1
}
```

输出：

```
      non_const(): this 是 Probe* const
        decltype(this) 是 const Probe* 吗: 0
      is_const() const: this 是 const Probe* const
        decltype(this) 是 const Probe* 吗: 1
```

所以上面那两条"约束"并不是什么额外规则，**就是 const 指针的常规行为**：

- 不能修改成员 → 因为 `this` 是 `const T*`，通过它访问的成员都是 const 的
- 不能调用非 const 成员函数 → 因为那些函数要求 `T* const this`，
  而你手里只有 `const T* const`，这个转换会丢掉 const，语言不允许

理解这一点之后，很多相关规则就不需要单独记了。
比如"静态成员函数不能是 const"（[03-05](03-05_special_members.md)）——
因为它没有 `this`，`const` 无从修饰。

## 二、const 对象

```cpp
const Computer cpc("ThinkPad", 9999);
cpc.print();            // ok：const 成员函数
// cpc.setPrice(1);     // 编译错误：passing 'const Computer' as 'this'
```

**能作用于 const 对象的成员函数，除了构造函数和析构函数，只有 const 成员函数。**
因为 const 对象只能被创建、只读访问、撤销，改写是不允许的。

错误信息很直白：`passing 'const Computer' as 'this' argument discards qualifiers`
—— 把 const 对象当 `this` 传进去会丢弃限定符。

这反过来推出一条重要的设计规则：

> **凡是不修改对象状态的成员函数，都应该声明为 const。**

否则你的类对 const 对象就是半残的 —— 别人拿到 `const T&` 后什么都做不了。
这个性质叫 **const 正确性（const correctness）**，
它会沿调用链传播：一个函数漏了 const，所有调用它的地方都没法是 const。

实践中应该**从一开始就写对**。给一个大型代码库补 const 是件痛苦的事 ——
改一个函数会牵出一长串需要同时修改的地方。

## 三、const 与非 const 的重载

const 可以参与重载（见 [01-05](../part1_basics/01-05_overload.md)：
底层 const 构成不同签名）：

```cpp
class Buffer {
public:
    // 非 const 版本：返回可写引用
    int& operator[](std::size_t i) { return _data[i]; }
    // const 版本：返回只读引用
    const int& operator[](std::size_t i) const { return _data[i]; }
};
```

调用哪个由对象的常量性决定：

```cpp
Buffer b(3);
b[0] = 42;              // 非 const 版本，返回 int&，可以赋值

const Buffer& cb = b;
int v = cb[0];          // const 版本，返回 const int&
// cb[0] = 1;           // 编译错误
```

实测：

```
      [非const operator[]] 写入 42
      [const operator[]] 读到 42
```

标准库到处是这个模式：`operator[]`、`at`、`front`、`back`、`data`、
`begin`/`end`（const 版本返回 `const_iterator`）。
这就是为什么容器有 `iterator` 和 `const_iterator` 两套类型。

### 避免两个版本的代码重复

两个版本的实现往往完全一样，只有返回类型不同。
重复的逻辑一多就容易不同步。标准惯用法是**非 const 版本转调 const 版本**：

```cpp
class Table {
public:
    int* find(const std::string& key) {
        // 1. 把 *this 转成 const 引用，调用 const 版本
        // 2. 把结果的 const 去掉
        return const_cast<int*>(static_cast<const Table&>(*this).find(key));
    }

    const int* find(const std::string& key) const {
        auto it = _data.find(key);
        return it == _data.end() ? nullptr : &it->second;
    }
};
```

实测两个版本都正常：

```
      非 const find 可以修改: 100
      const find 只能读: 100
```

**方向很重要：必须是非 const 版本调 const 版本，不能反过来。**

- 非 const → const：安全。对象本来就是非 const 的，
  调完之后用 `const_cast` 去掉 const 也合法（见 [01-04](../part1_basics/01-04_cast.md)）
- const → 非 const：**不安全**。需要 `const_cast` 掉 `this` 的 const，
  而对象可能真的是 const 对象，修改它是未定义行为

两步转换缺一不可：`static_cast<const Table&>(*this)` 选中 const 版本，
`const_cast<int*>` 还原可写性。少了第一步会无限递归。

（C++23 的"推导 this"特性可以用一个模板函数同时生成两个版本，
彻底消除这个样板代码。）

## 四、mutable：逻辑常量性

有时一个函数逻辑上不改变对象状态，但内部需要修改某些成员：
缓存、调用计数、延迟初始化的标志、互斥量。

`mutable` 成员可以在 const 成员函数中被修改：

```cpp
class CachedValue {
public:
    int compute() const {
        ++_call_count;                   // mutable，可以改
        if (!_cached) {
            _cache = _base * _base;      // mutable
            _cached = true;
        }
        return _cache;
    }
private:
    int _base;
    mutable int _cache = 0;
    mutable bool _cached = false;
    mutable int _call_count = 0;
};

const CachedValue c(12);     // 注意是 const 对象
c.compute();                  // 首次计算
c.compute();                  // 用缓存
```

实测：

```
compute()=      （首次计算）144
compute()=      （用缓存）144
      调用次数: 2
```

`mutable` 表达的是**逻辑上的常量性**：
对外看来对象没变（两次 `compute()` 返回同样的值），
变的只是内部实现细节。

### 最正当的用途：在 const 成员函数里加锁

```cpp
class ThreadSafeCounter {
public:
    int get() const {
        std::lock_guard<std::mutex> lk(_mtx);   // lock() 会修改 _mtx
        return _value;
    }
private:
    mutable std::mutex _mtx;      // 不是 mutable 的话，const 成员函数无法加锁
    int _value = 0;
};
```

`std::mutex::lock()` 是非 const 的成员函数（它确实改变了互斥量的状态），
所以在 const 成员函数里加锁，互斥量必须是 `mutable` 的。

这是 `mutable` 无可替代的场合 ——
没有它，线程安全的类就无法提供 const 的读接口。

### mutable 的风险

滥用 `mutable` 会破坏一个重要的直觉：**const 意味着可以并发读**。

C++11 之后，标准库对 const 的要求是"const 成员函数必须是线程安全的"
（因为标准库算法会并发调用它们）。如果你的 `mutable` 成员在 const 成员函数里
被无保护地修改，那么多线程并发调用这个 const 函数就是数据竞争：

```cpp
int compute() const {
    ++_call_count;      // 两个线程同时进来 -> 数据竞争
    // ...
}
```

上面的 `CachedValue` 就有这个问题。规则：

> **用了 `mutable`，就要么保证线程安全（加锁或用 atomic），
> 要么在文档里明确说明这个 const 函数不是线程安全的。**

```cpp
mutable std::atomic<int> _call_count{0};   // 这样就安全了
```

## 五、陷阱：const 成员函数不保护指针指向的内容

这是 const 正确性最容易漏的一点。

```cpp
class Leaky {
public:
    int* data() const { return _data; }     // 合法！但泄漏了可写访问
private:
    int* _data;
};

const Leaky obj;
*obj.data() = 42;        // 修改了"const 对象"的内容
```

为什么合法？因为 const 成员函数保证的是"不修改 `_data` 这个**指针本身**"，
而返回它的副本并不修改它。至于通过这个指针去改指向的内容 —— const 管不着。

这叫 **浅 const（shallow const）**：
C++ 的 const 只作用一层，不会自动传播到指针指向的对象。

正确做法是手工加上：

```cpp
class Correct {
public:
    const int* data() const { return _data; }   // const 版本返回 const 指针
    int* data() { return _data; }                // 非 const 版本返回可写指针
private:
    int* _data;
};
```

同样的问题也出现在返回引用、返回内部容器的迭代器上。
用 `std::unique_ptr` 成员时也要注意：

```cpp
class WithSmartPtr {
public:
    // unique_ptr 成员本身被保护了，但指向的对象没有
    T* get() const { return _p.get(); }           // 泄漏可写访问
    const T* get() const { return _p.get(); }     // 正确
private:
    std::unique_ptr<T> _p;
};
```

## 六、小结

| 要点 | 说明 |
|---|---|
| **本质** | const 成员函数把 `this` 从 `T* const` 变成 `const T* const` |
| 两条约束 | 不能改成员、不能调非 const 成员函数 —— 都是 const 指针的常规行为 |
| const 对象 | 只能调用 const 成员函数（构造/析构除外）|
| 设计规则 | **所有不修改状态的成员函数都应该是 const** |
| const 正确性 | 会沿调用链传播；一开始就写对，事后补很痛苦 |
| const 重载 | 常用于 `operator[]`/`begin`/`at`，分别返回可写和只读引用 |
| 避免重复 | 非 const 版本转调 const 版本 + `const_cast`；**不能反方向** |
| `mutable` | 表达逻辑常量性；最正当用途是在 const 成员函数里加锁 |
| `mutable` 的风险 | 可能破坏"const 可并发读"的约定，要自己保证线程安全 |
| **浅 const** | const 只作用一层；`T* data() const` 会泄漏可写访问 |

> 下一章：[03-07 单例模式的四次进化](03-07_singleton.md)
