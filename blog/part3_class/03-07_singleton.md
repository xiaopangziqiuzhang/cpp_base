# 03-07 单例模式的四次进化

> 标准：静态局部变量的线程安全初始化、`std::call_once` `[C++11]`
> demo：[`03_07_singleton.cpp`](../../demos/part3_class/03_07_singleton.cpp)

单例是 23 种 GOF 模式中最简单的创建型模式，目的是确保一个类只有一个实例。
它也是最能说明"C++11 如何改变了既有写法"的例子 ——
同一个需求，C++98 需要几十行并且很难写对，C++11 只需要三行。

## 一、版本一：裸指针懒汉（经典写法）

根据已学的知识，实现步骤是三步：

1. 将构造函数私有化
2. 在类中定义一个静态的指向本类型的指针变量
3. 定义一个返回类指针的静态成员函数

```cpp
class Singleton {
public:
    static Singleton* getInstance() {
        if (nullptr == _pInstance) {
            _pInstance = new Singleton();
        }
        return _pInstance;
    }
private:
    Singleton() {}
    ~Singleton() {}
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static Singleton* _pInstance;
};
Singleton* Singleton::_pInstance = nullptr;

Singleton* p1 = Singleton::getInstance();
Singleton* p2 = Singleton::getInstance();
assert(p1 == p2);
```

实测确认两次调用返回同一地址。为什么构造函数要私有化？
因为这样外部就无法 `new Singleton()` 或创建栈对象，
唯一的获取途径是 `getInstance()`。拷贝构造和赋值也要禁掉，
否则 `Singleton s = *p;` 就复制出第二个实例了。

### 参考文档留的思考题：这个实现还有什么问题？

**两个严重问题：**

**问题一：多线程不安全。** 两个线程同时首次调用 `getInstance()`：

```
线程 A: if (_pInstance == nullptr)  -> 真
线程 B: if (_pInstance == nullptr)  -> 也为真（A 还没赋值）
线程 A: _pInstance = new Singleton();
线程 B: _pInstance = new Singleton();   -> 创建了两个实例，前一个泄漏
```

**问题二：析构函数永远不会被调用。** `new` 出来的对象没人 `delete`。
实测输出里完全没有 `V1_Naive() 析构` 这一行 —— 资源不会被释放。

对单例来说，"进程退出时才释放"通常可以接受（操作系统会回收内存），
但如果单例持有的是文件句柄、数据库连接、需要 flush 的缓冲区，
不调用析构函数就是真问题。

## 二、版本二：加锁

最直接的修法 —— 给整个函数加锁：

```cpp
static V2_Locked* getInstance() {
    std::lock_guard<std::mutex> lk(_mtx);      // 每次调用都加锁
    if (nullptr == _pInstance) {
        _pInstance = new V2_Locked();
    }
    return _pInstance;
}
```

实测 8 个线程并发调用，拿到的指针全部相同 —— 线程安全问题解决了。

**但引入了性能问题：每次 `getInstance()` 都要加锁，而其实只有第一次需要。**
单例往往在热路径上被频繁调用，这个锁的开销不可忽视。

## 三、版本三：双检锁（著名的错误写法）

为了避免每次加锁，社区发明了**双检锁（Double-Checked Locking）**：

```cpp
// ⚠ 这个版本是错的
static Singleton* getInstance() {
    if (_pInstance == nullptr) {               // 第一次检查：无锁，快速路径
        std::lock_guard<std::mutex> lk(_mtx);
        if (_pInstance == nullptr) {           // 第二次检查：持锁
            _pInstance = new Singleton();
        }
    }
    return _pInstance;
}
```

这段代码在 2000 年代初广泛流传，但它**在 C++98 的内存模型下是错的**，
原因是：

```cpp
_pInstance = new Singleton();
```

这一行不是原子的，它包含三个步骤：

1. 分配内存
2. 在该内存上运行构造函数
3. 把地址赋给 `_pInstance`

**编译器和 CPU 可以重排 2 和 3。** 如果实际执行顺序是 1→3→2，
那么另一个线程在第一次检查时会看到 `_pInstance != nullptr`，
于是直接返回 —— 返回了一个**内存已分配但构造函数还没跑完**的对象。

这类 bug 极难复现和调试：它依赖于具体的编译器优化、CPU 架构、线程调度时机。

### 正确的双检锁需要原子操作和内存序

C++11 引入了内存模型之后，可以写对了：

```cpp
static V3_DoubleChecked* getInstance() {
    V3_DoubleChecked* tmp = _pInstance.load(std::memory_order_acquire);
    if (tmp == nullptr) {
        std::lock_guard<std::mutex> lk(_mtx);
        tmp = _pInstance.load(std::memory_order_relaxed);
        if (tmp == nullptr) {
            tmp = new V3_DoubleChecked();
            _pInstance.store(tmp, std::memory_order_release);   // release 保证
        }                                                        // 构造先于发布
    }
    return tmp;
}
private:
    static std::atomic<V3_DoubleChecked*> _pInstance;
```

`release`/`acquire` 配对保证了：
如果一个线程通过 `acquire` 看到了非空的 `_pInstance`，
那么构造函数的全部写入对它都是可见的。内存序的细节见
[08-04 原子操作与内存模型](../part8_concurrency/08-04_atomic.md)。

**但是：知道怎么写对之后，你会发现没必要写它。**

## 四、版本四：Meyers Singleton（推荐）

C++11 给了静态局部变量一条关键保证：

> **如果多个线程同时执行到一个静态局部变量的声明，
> 只有一个线程会执行初始化，其他线程阻塞等待初始化完成。**

于是整个单例可以简化成：

```cpp
class Singleton {
public:
    static Singleton& getInstance() {
        static Singleton instance;      // C++11 保证线程安全的初始化
        return instance;                // 返回引用，不是指针
    }
private:
    Singleton() = default;
    ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};
```

这叫 **Meyers Singleton**（Scott Meyers 在《Effective C++》里推广）。
实测 8 个线程并发调用，全部拿到同一实例。

它一次解决了前面所有问题：

| 问题 | 解决方式 |
|---|---|
| 线程安全 | C++11 的标准保证（编译器用内部的 guard 变量实现）|
| 析构 | 静态局部对象在程序结束时自动析构 |
| 懒加载 | 首次调用 `getInstance()` 时才构造 |
| 锁开销 | 初始化完成后，后续调用只有一次标志位检查，无锁 |
| 代码量 | 三行 |

实测输出确认了自动析构：

```
=== main 结束，下面是自动析构 ===
      V4_Meyers() 析构（程序结束时自动）
```

对比 V1/V2 完全没有析构输出。

### 为什么返回引用而不是指针

```cpp
static Singleton& getInstance();      // 推荐
static Singleton* getInstance();      // 不推荐
```

返回引用表达了"**这个对象一定存在**"，调用方不需要判空。
返回指针会让人以为可能返回 `nullptr`，或者以为自己该负责 `delete`。

用起来也更自然：

```cpp
Singleton::getInstance().do_work();
```

### 注意：销毁顺序问题

Meyers Singleton 的析构发生在程序结束时，
多个单例之间的**析构顺序是构造顺序的逆序**。
如果单例 A 的析构函数用到了单例 B，而 B 先被销毁，就是访问已销毁对象。

缓解办法：

- 让依赖关系中被依赖的一方先构造（在 A 的构造函数里先调一次 `B::getInstance()`）
- 或者接受"单例不释放"，用下面的 call_once + 裸指针版本

## 五、版本五：call_once（需要控制析构时机时）

`std::call_once` 保证某个函数在多线程下只执行一次：

```cpp
class V5_CallOnce {
public:
    static V5_CallOnce& getInstance() {
        std::call_once(_flag, [] { _pInstance.reset(new V5_CallOnce()); });
        return *_pInstance;
    }
private:
    V5_CallOnce() = default;
    ~V5_CallOnce() = default;

    // 析构函数是 private，unique_ptr 的默认删除器访问不到它，
    // 所以要提供一个有访问权限的删除器（嵌套类是本类成员，有权限）
    struct Deleter {
        void operator()(V5_CallOnce* p) const { delete p; }
    };

    static std::once_flag _flag;
    static std::unique_ptr<V5_CallOnce, Deleter> _pInstance;
};
```

写这个 demo 时踩到一个实际问题值得记录：
如果直接用 `std::unique_ptr<V5_CallOnce>`，会编译失败：

```
error: 'V5_CallOnce::~V5_CallOnce()' is private within this context
```

因为 `std::default_delete` 是外部类，访问不到 private 的析构函数。
解法是提供一个嵌套的自定义删除器（嵌套类是本类的成员，有访问权），
或者把 `std::default_delete<V5_CallOnce>` 声明为友元。

### 什么时候用 call_once 而不是 Meyers

- 需要**显式控制析构时机**（可以手工 `reset()`）
- 初始化逻辑复杂，需要传参数或做错误处理
- 需要"初始化失败可以重试"的语义
  （`call_once` 的回调抛异常时，标志位不会被设置，下次还会再试）

多数情况下 Meyers Singleton 就够了。

## 六、单例的真正问题

前面讨论的都是"怎么把单例写对"。但更值得问的是：**要不要用单例？**

单例的本质是**全局状态**，它带来几个结构性问题：

**一、隐藏依赖。** 一个函数用了单例，从签名上看不出来：

```cpp
void process() {
    Logger::getInstance().log("...");    // 依赖不可见
    Config::getInstance().get("key");
}
```

读 `process()` 的签名，你不知道它依赖 Logger 和 Config。
这让代码难以理解，也让重构变得危险。

**二、无法测试。** 单元测试想替换掉日志或配置，但单例不允许。
测试之间也会互相污染 —— 单例的状态在测试间共享。

**三、并发问题转移。** 单例本身线程安全了，但它持有的**状态**仍然需要保护。
`getInstance()` 线程安全不代表 `getInstance().modify()` 线程安全。

**四、生命周期不可控。** 构造时机取决于首次调用，析构顺序难以管理。

### 替代方案：依赖注入

```cpp
class Service {
public:
    explicit Service(Logger& logger) : _logger(logger) {}   // 依赖显式声明
    void work() { _logger.log("Service::work()"); }
private:
    Logger& _logger;
};

// 生产环境
Logger prod_logger("PROD");
Service s(prod_logger);

// 测试环境：可以注入另一个
Logger test_logger("TEST");
Service s2(test_logger);
```

实测输出：

```
      [PROD] Service::work()
      [TEST] Service::work()
      依赖注入让单元测试可以替换依赖，单例做不到
```

依赖写在构造函数签名里 —— 可见、可替换、可测试。

### 什么时候单例是合理的

不是说单例一律不能用。它合理的场合有共同特征：
**确实在物理上只有一个，而且没有状态或状态天然全局**。

- 日志系统（通常可以接受）
- 硬件资源的唯一句柄（一块 GPU、一个串口）
- 线程池、内存池这类基础设施
- 纯粹的常量表、注册表

不合理的场合：配置（测试要替换）、数据库连接（可能要多个）、
任何持有可变业务状态的东西。

实践建议：**默认用依赖注入，确实需要全局唯一时才用 Meyers Singleton。**
而且即使用了，也尽量让单例只是一个"获取默认实例"的便利函数，
而类本身仍然可以被正常构造：

```cpp
class Logger {
public:
    Logger(std::string prefix);                 // 仍然可以自己构造
    static Logger& default_instance();          // 便利函数
};
```

这样既有全局访问的便利，又保留了测试时替换的能力。

## 七、小结

五个版本对照：

| 版本 | 线程安全 | 自动析构 | 锁开销 | 代码量 |
|---|---|---|---|---|
| V1 裸指针懒汉 | ✗ | ✗ | 无 | 中 |
| V2 全程加锁 | ✓ | ✗ | **每次调用** | 中 |
| V3 双检锁（裸指针 + 普通 if） | **✗（错的）** | ✗ | 仅首次 | 多 |
| V3' 双检锁（atomic + 内存序） | ✓ | ✗ | 仅首次 | 多 |
| **V4 Meyers Singleton** | ✓ | ✓ | 可忽略 | **少** |
| V5 call_once | ✓ | ✓（可控）| 可忽略 | 中 |

要点：

- C++11 保证静态局部变量的初始化是线程安全的 —— 这是 Meyers Singleton 的基础
- **优先用 Meyers Singleton**：三行代码，线程安全，自动析构，懒加载
- 返回引用而不是指针，表达"一定存在"
- 需要控制析构时机或复杂初始化时用 `call_once`
- 双检锁不要手写；要写必须用 `atomic` + acquire/release
- 多个单例之间有析构顺序问题
- **单例的真正代价是全局状态**：隐藏依赖、无法测试。优先考虑依赖注入

> 下一章：[03-08 Pimpl 惯用法](03-08_pimpl.md)
