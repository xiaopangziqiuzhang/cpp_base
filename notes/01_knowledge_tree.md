# 参考文档知识树（Step 1 产物）

对两份参考文档（wangdao_pairt_1 / wangdao_pairt_2，共 6568 行）的结构化归纳。
目的：搞清楚"已有什么"，从而确定"博客要补什么"。

---

## 一、参考文档实际覆盖的内容

### Part 1（3707 行）

| 大块 | 具体知识点 |
|---|---|
| C++ 历史发展 | 三阶段演进、Bjarne Stroustrup、历史事件年表、ISO 标准列表（C++98→C++17）、C++11 的影响、推荐书籍 |
| C++ vs C | 命名空间（3 种使用方式/匿名/嵌套）、const（修饰变量/指针/成员函数/对象）、new/delete vs malloc/free、引用（本质/做参数/做返回值）、4 种强制转换、函数重载与 name mangling、默认参数、bool、inline 函数、异常 try/catch/throw |
| 字符串 | C 风格字符串及其隐患、std::string 构造/转换/遍历/容量/追加/子串/删除/比较/查找 |
| 程序内存分配 | 虚拟内存布局（栈/堆/全局静态区/常量区/代码区）、栈 vs 堆六大区别 |
| 类和对象 | 面向过程 vs 面向对象的哲学讨论、类的定义、class vs struct、构造函数、初始化列表（初始化顺序陷阱）、析构函数、拷贝构造（深浅拷贝/double free）、this 指针、赋值运算符函数、特殊数据成员（const/引用/类对象/static）、静态成员函数、const 成员函数、对象的组织（const 对象/指针/数组/堆对象）、单例模式 |
| new/delete 表达式 | new 的 3 步、delete 的 2 步、operator new/delete 重载、只能建栈对象、只能建堆对象 |
| IO 流 | 流的概念、流类型体系、流状态（badbit/failbit/eofbit/goodbit）、缓冲区原理与类型、标准 IO（cin/cout/cerr/clog）、操纵符、文件 IO 与文件模式、字符串 IO（stringstream 做格式转换） |

> 注：Part 1 的 2337-2995 行与 2995-3707 行是**重复内容**（IO 流一章出现了两遍）。

### Part 2（2861 行）

| 大块 | 具体知识点 |
|---|---|
| 友元 | 友元函数（全局/成员）、友元类、友元的破坏封装性问题 |
| 运算符重载 | 为什么需要、重载规则、3 种形式（普通函数/成员函数/友元函数）、特殊运算符（自增自减/赋值/函数调用/下标/成员访问/输入输出流） |
| 类型转换 | 其他类型→自定义类型（转换构造函数）、自定义类型→其他类型（类型转换运算符） |
| 类作用域 | 全局/类/块作用域、Pimpl 设计模式、单例的自动释放 |
| string 底层实现 | Eager Copy、COW（写时复制+引用计数+原子操作）、SSO（短字符串优化）、folly::fbstring 混合策略、线程安全性讨论 |
| 继承 | 继承的哲学意义、继承定义、继承的局限、3 种派生方式的访问权限、派生类对象的构造/销毁顺序、多基继承、成员名冲突二义性、菱形继承、基类派生类相互转换、派生类的复制控制 |
| 多态 | 为什么要多态、虚函数定义、虚函数实现机制（虚表/虚指针）、不能做虚函数的函数、虚函数的 5 种访问方式、构造析构中调用虚函数、纯虚函数、抽象类、虚析构函数、重载/隐藏/覆盖的区别、测试虚表存在、带虚函数的多基派生、虚拟继承、菱形继承解决、效率分析 |
| 模板 | 为什么要模板、模板定义、函数模板、模板与重载、非类型参数、模板特化、成员函数模板、**可变模板参数**（sizeof.../递归展开/逗号表达式展开/包扩展）、类模板 |

---

## 二、参考文档的 C++11/14/17 覆盖度评估

参考文档的定位是**面向对象 + 泛型基础**（王道 C++ 培训风格），标准上大体停留在 C++98/03，
只零星提到 C++11：

- 提到了：nullptr（仅在代码里用）、可变模板参数（唯一成体系讲的 C++11 特性）、
  COW→SSO 的演进（涉及 C++11 对 string 的要求）、`data()` 在 C++11 后的变化
- **完全没有覆盖**（而 readme 任务明确要求的部分）：

| 缺口 | 具体内容 |
|---|---|
| 类型推导 | auto、decltype、decltype(auto)（C++14）、尾置返回类型 |
| 初始化 | 列表初始化 / 统一初始化、initializer_list、聚合初始化、类内成员初始值 |
| 移动语义 | 右值引用、移动构造/移动赋值、std::move、返回值优化、Rule of Five |
| 完美转发 | 万能引用、引用折叠、std::forward |
| lambda | 闭包、捕获方式、泛型 lambda（C++14）、初始化捕获（C++14）、constexpr lambda（C++17）、*this 捕获（C++17） |
| 智能指针 | unique_ptr / shared_ptr / weak_ptr、make_unique（C++14）、循环引用、自定义删除器、RAII |
| 类的新语法 | =default、=delete、override、final、委托构造、继承构造、explicit 转换运算符 |
| 枚举与常量 | enum class、constexpr（C++11/14 放宽/C++17 if constexpr）、inline 变量（C++17） |
| 编译期 | static_assert、type_traits、SFINAE、enable_if、if constexpr、折叠表达式（C++17） |
| 字面量 | 原始字符串、用户自定义字面量、二进制字面量与数字分隔符（C++14） |
| 标准库容器 | array、forward_list、unordered_* 系列、tuple、emplace 系列接口 |
| 可调用对象 | std::function、std::bind、std::ref |
| 并发 | thread、mutex 家族、lock_guard/unique_lock/scoped_lock、condition_variable、atomic、内存序、future/promise/packaged_task/async、call_once、thread_local |
| 新工具库 | chrono、random、regex、ratio |
| C++14 | 泛型 lambda、auto 返回类型推导、make_unique、变量模板、[[deprecated]] |
| C++17 | 结构化绑定、if/switch 初始化语句、折叠表达式、CTAD、optional/variant/any、string_view、filesystem、并行算法、内联变量、嵌套命名空间 |
| 属性 | [[noreturn]]、[[nodiscard]]、[[maybe_unused]]、[[fallthrough]] |

---

## 三、结论：博客的内容来源划分

1. **可直接吸收改写**（参考文档已有，需补上现代 C++ 的写法对比）
   - 基础语法、C++ vs C、类与对象、运算符重载、继承多态、模板基础、IO 流、string
2. **参考文档有但要大幅现代化**
   - 单例模式（→ Meyers Singleton / call_once）
   - 拷贝控制（→ Rule of Five / 移动语义）
   - 裸指针资源管理（→ 智能指针 RAII）
   - 可变模板参数（→ 折叠表达式）
3. **完全新增**（本博客的主体价值）
   - C++11/14/17 全部新特性，按"解决了什么问题"组织

---

## 四、文档中值得保留的"考点"与"思考题"

参考文档里埋了不少面试常考点和未解答的思考题，博客中应当明确回答：

- const 常量 vs 宏定义的区别
- new/delete vs malloc/free 的区别
- 引用的本质（是否占内存）
- 为什么初始化列表的顺序由声明顺序决定
- 拷贝构造函数的参数形式能否改变（为什么必须是引用）
- 赋值运算符函数的返回值类型能否改变
- const 成员函数到底发生了什么（this 指针的类型变化）
- 通过空指针调用成员函数会发生什么（NullPointCall 例子）
- 单例模式的现有实现有什么问题、怎么改进（线程安全 + 释放）
- COW 的引用计数为什么不和 string 的数据成员放在一起
- 为什么多核时代编译器厂商都选择了 SSO
- 哪些函数不能是虚函数
- 构造/析构函数中调用虚函数会发生什么
