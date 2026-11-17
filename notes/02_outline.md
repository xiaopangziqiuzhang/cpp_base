# 章节设计（Step 2 产物）

## 设计原则

1. **问题驱动**：每个特性先讲"没有它的时候代码怎么写、痛在哪"，再讲特性本身。
   这是 readme 里"新特性解决了什么问题"的硬要求。
2. **代码可运行**：每章的代码片段都对应 `demos/` 下的真实可编译文件，
   由 `scripts/build_all.sh` 全量编译验证，博客里不出现跑不起来的伪代码。
3. **标准标注**：每个特性标注 `[C++11]` / `[C++14]` / `[C++17]`，
   方便读者按自己的编译器版本取用。
4. **新旧对比**：凡是现代写法替代了旧写法的，并排给出 Before / After。

## 目录结构

```
cpp_1111/
├── readme.md              # 任务说明（只读）
├── notes/                 # 过程产物
│   ├── 01_knowledge_tree.md
│   ├── 02_outline.md
│   └── 03_progress.md     # 进度跟踪，支持断点续写
├── blog/                  # 博客正文
│   ├── README.md          # 总目录
│   ├── part0_overview/
│   ├── part1_basics/
│   ├── part2_memory/
│   ├── part3_class/
│   ├── part4_oop/
│   ├── part5_template/
│   ├── part6_modern_core/
│   ├── part7_stdlib/
│   ├── part8_concurrency/
│   └── part9_appendix/
├── demos/                 # 可编译 demo，按 part 分目录
└── scripts/               # 构建与校验脚本
```

---

## Part 0 导读

| 章 | 标题 | 要点 |
|---|---|---|
| 00-01 | C++ 的三个时代与标准演进 | 三阶段、ISO 标准表、各标准核心主题、编译器支持现状 |
| 00-02 | 本书怎么读 / 环境准备 | g++ 版本与 -std 选项、编译命令、demo 组织方式 |

## Part 1 基础语法：C++ 在 C 之上加了什么

| 章 | 标题 | 关键特性 |
|---|---|---|
| 01-01 | 命名空间 | namespace 三种用法、匿名空间、嵌套命名空间简化 `[C++17]`、inline namespace `[C++11]` |
| 01-02 | const 与 constexpr | const 修饰变量/指针/成员函数/对象、const vs 宏、constexpr `[C++11/14]`、inline 变量 `[C++17]` |
| 01-03 | 引用 | 左值引用本质、做参数、做返回值、引用 vs 指针、const 引用延长临时量寿命 |
| 01-04 | 类型转换 | 4 种 cast、为什么不用 C 风格、dynamic_cast 与 RTTI |
| 01-05 | 函数重载与默认参数 | name mangling、extern "C"、默认参数规则与二义性 |
| 01-06 | bool / nullptr / enum class | bool、NULL 的三宗罪与 nullptr `[C++11]`、enum class `[C++11]` |
| 01-07 | inline 函数 | inline vs 宏、放头文件的原因、现代语义（链接而非展开） |
| 01-08 | 异常与异常安全 | try/catch/throw、标准异常体系、noexcept `[C++11]`、异常安全三级保证、RAII |
| 01-09 | 字面量与新语法糖 | 原始字符串 `[C++11]`、用户自定义字面量 `[C++11]`、二进制字面量+数字分隔符 `[C++14]`、属性 `[[...]]` |
| 01-10 | 类型推导：auto 与 decltype | auto 推导规则与陷阱、decltype、尾置返回、decltype(auto) `[C++14]` |
| 01-11 | 统一初始化 | {} 初始化、initializer_list、最烦人的解析、窄化转换、聚合初始化 |
| 01-12 | 范围 for 与结构化绑定 | 范围 for `[C++11]`、自定义类型适配、结构化绑定 `[C++17]`、if/switch 初始化语句 `[C++17]` |

## Part 2 内存模型与资源管理

| 章 | 标题 | 关键特性 |
|---|---|---|
| 02-01 | 程序内存布局 | 五大区、实测各区地址、栈 vs 堆对比 |
| 02-02 | new/delete 表达式 | new 三步、delete 两步、operator new/delete 重载、placement new、只能建栈/堆对象 |
| 02-03 | 右值引用与移动语义 | 左值/右值/将亡值、移动构造与移动赋值、std::move、Rule of Five、noexcept 对容器的影响 |
| 02-04 | 完美转发 | 万能引用、引用折叠、std::forward、emplace_back 原理 |
| 02-05 | 智能指针 | unique_ptr、shared_ptr、weak_ptr、make_unique `[C++14]`、循环引用、自定义删除器、enable_shared_from_this |
| 02-06 | RAII 与资源管理惯用法 | RAII 原理、lock_guard 式封装、scope guard、为什么不再手写 delete |

## Part 3 类与对象

| 章 | 标题 | 关键特性 |
|---|---|---|
| 03-01 | 类的定义与 this | class vs struct、成员函数的本质、this、空指针调用成员函数 |
| 03-02 | 构造函数 | 重载、初始化列表与初始化顺序、委托构造 `[C++11]`、继承构造 `[C++11]`、类内成员初始值 `[C++11]`、explicit |
| 03-03 | 析构函数与对象生命周期 | 析构调用时机、析构顺序、虚析构预告 |
| 03-04 | 拷贝控制与 Rule of Five | 拷贝构造/赋值、深浅拷贝与 double free、=default / =delete `[C++11]`、五大函数的生成规则 |
| 03-05 | 特殊成员 | const 成员、引用成员、类对象成员、static 成员、static 成员的 inline 初始化 `[C++17]` |
| 03-06 | const 成员函数与 mutable | const 成员函数的本质、重载决议、mutable |
| 03-07 | 单例模式的四次进化 | 懒汉裸指针 → 加锁 → Meyers Singleton `[C++11]` → call_once |
| 03-08 | Pimpl 惯用法 | 编译期解耦、与 unique_ptr 配合、析构函数不完整类型问题 |
| 03-09 | 友元与运算符重载 | 友元函数/类、重载规则、三种形式、特殊运算符（`++`/`=`/`()`/`[]`/`->`/`<<`）|
| 03-10 | 类型转换 | 转换构造函数、类型转换运算符、explicit、二义性、只走一步 |

## Part 4 面向对象：继承与多态

| 章 | 标题 | 关键特性 |
|---|---|---|
| 04-01 | 继承基础 | 三种派生方式的权限、继承的局限、using 改变访问权限 |
| 04-02 | 派生类的构造与销毁 | 构造/析构顺序、多基继承、基类与派生类的转换 |
| 04-03 | 名字冲突与菱形继承 | 二义性、virtual 继承、虚基类对象布局 |
| 04-04 | 虚函数与多态 | 动态绑定、虚表虚指针、实测虚表、哪些不能是虚函数、构造析构中的虚调用 |
| 04-05 | override / final 与三种"同名" | 重载 vs 隐藏 vs 覆盖、override `[C++11]`、final `[C++11]` |
| 04-06 | 纯虚函数、抽象类、接口 | 纯虚函数、抽象类、虚析构函数的必要性 |
| 04-07 | 多态的代价与 CRTP | 虚调用开销、内联障碍、CRTP 静态多态 |

## Part 5 模板与泛型编程

| 章 | 标题 | 关键特性 |
|---|---|---|
| 05-01 | 函数模板 | 定义、实例化、推导、与重载的关系、非类型参数、auto 非类型参数 `[C++17]` |
| 05-02 | 类模板 | 定义、成员函数定义、默认模板参数、CTAD `[C++17]`、变量模板 `[C++14]` |
| 05-03 | 特化与偏特化 | 全特化、偏特化、特化的常见坑 |
| 05-04 | 可变模板参数 | 参数包、sizeof...、递归展开、逗号表达式展开、包扩展、折叠表达式 `[C++17]` |
| 05-05 | 编译期计算与 type_traits | static_assert `[C++11]`、type_traits、SFINAE、enable_if、if constexpr `[C++17]` |
| 05-06 | 模板的工程问题 | 两阶段查找、typename/template 消歧、显式实例化、编译期开销 |

## Part 6 C++11/14/17 核心语言特性专题

| 章 | 标题 | 关键特性 |
|---|---|---|
| 06-01 | lambda 表达式全解 | 闭包本质、捕获方式、mutable、泛型 lambda `[C++14]`、初始化捕获 `[C++14]`、constexpr lambda / *this 捕获 `[C++17]` |
| 06-02 | 可调用对象统一视图 | 函数指针/函数对象/lambda/成员函数指针、std::function、std::bind、std::ref、std::invoke `[C++17]` |
| 06-03 | 类型别名与 using | typedef vs using `[C++11]`、别名模板、_t/_v 后缀惯例 `[C++14]` |
| 06-04 | C++14 特性汇总 | 泛型 lambda、auto 返回推导、make_unique、变量模板、constexpr 放宽、[[deprecated]] |
| 06-05 | C++17 语言特性汇总 | 结构化绑定、if constexpr、折叠表达式、CTAD、内联变量、嵌套命名空间、保证的拷贝消除 |

## Part 7 标准库

| 章 | 标题 | 关键特性 |
|---|---|---|
| 07-01 | string 与 string_view | string 接口、底层实现（Eager/COW/SSO/fbstring）、string_view `[C++17]` 及其悬垂陷阱 |
| 07-02 | 容器全景 | 顺序/关联/无序容器、array `[C++11]`、forward_list `[C++11]`、unordered_* `[C++11]`、emplace 系列、容器选择决策表 |
| 07-03 | 迭代器与算法 | 迭代器分类、常用算法、lambda 配合、并行算法 `[C++17]` |
| 07-04 | tuple 与和类型 | pair/tuple、tie、apply `[C++17]`、optional / variant / any `[C++17]` |
| 07-05 | chrono 与 random | 时间点/时长/时钟、计时惯用法、随机数引擎与分布（为什么不用 rand） |
| 07-06 | 正则与文件系统 | regex `[C++11]`、filesystem `[C++17]` |
| 07-07 | IO 流体系 | 流类型关系、流状态、缓冲区、标准 IO、文件 IO、stringstream 格式转换、操纵符 |

## Part 8 并发

| 章 | 标题 | 关键特性 |
|---|---|---|
| 08-01 | 线程 | thread、join/detach、传参陷阱、hardware_concurrency、thread_local |
| 08-02 | 互斥与锁 | mutex 家族、lock_guard / unique_lock / scoped_lock `[C++17]`、死锁与 std::lock |
| 08-03 | 条件变量与同步 | condition_variable、虚假唤醒、生产者消费者队列实现 |
| 08-04 | 原子操作与内存模型 | atomic、CAS、六种内存序、无锁栈示例 |
| 08-05 | 异步任务 | future / promise / packaged_task / async、线程池实现 |

## Part 9 附录

| 章 | 标题 | 内容 |
|---|---|---|
| 09-01 | 特性速查表 | C++11/14/17 全特性一览 + 所在章节 |
| 09-02 | 高频面试题 | 知识树第四节的考点逐个解答 |
| 09-03 | 现代 C++ 编码清单 | 该用什么不该用什么、Core Guidelines 摘要 |
| 09-04 | demo 索引 | 所有 demo 文件与对应章节、编译方式 |

---

## 规模估算

- 章节数：64 章（10 个 part）
- demo 文件：62 个
- 执行顺序：Part 0 → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9
  （按依赖顺序，先基础后进阶；Part 6 的 lambda 等在 Part 1/2 已被引用处用"详见 06-01"交叉引用）
