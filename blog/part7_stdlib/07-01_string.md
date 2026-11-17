# 07-01 string 与 string_view

> 标准：`std::string` C++98；`string_view`、`from_chars`/`to_chars` `[C++17]`
> demo：[`07_01_string.cpp`](../../demos/part7_stdlib/07_01_string.cpp)

## 一、C 风格字符串的问题

C 风格字符串是以 `'\0'` 结尾的字符数组，操作函数在 `<string.h>` / `<cstring>`：

```cpp
size_t strlen(const char* str);
int strcmp(const char* lhs, const char* rhs);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
char* strstr(const char* str, const char* substr);
```

用它做一次拼接：

```cpp
char str[] = "hello";
const char* pstr = "world";

char* ptmp = (char*)malloc(strlen(str) + strlen(pstr) + 1);   // 手工算大小
strcpy(ptmp, str);
strcat(ptmp, pstr);
// 用完要记得 free
free(ptmp);
```

问题清单：

1. **长度是 O(n)** —— 每次 `strlen` 都要扫描到 `'\0'`
2. **内存要手工管理** —— `malloc`/`free` 配对，容易泄漏
3. **缓冲区大小要手工算** —— `strcpy`/`strcat` 是**缓冲区溢出的头号来源**
   （算错一个字节就越界写）
4. **不能用 `==` 比较** —— 比的是指针，必须用 `strcmp`
5. **结尾必须有 `'\0'`** —— 忘了就一路读到越界
6. **不能放进容器、不能安全返回**

> 尽管 C++ 支持 C 风格字符串，但在 C++ 程序中最好还是不要使用它们。
> C 风格字符串不仅使用起来不方便，而且极易引发程序漏洞，是诸多安全问题的根本原因。

## 二、std::string

`std::string` 定义在 `<string>` 中（注意和 `<cstring>` 区分，
后者是对 C 的 `<string.h>` 的封装）。
它本质上是 `basic_string` 模板关于 `char` 的实例化。

完全可以把 `string` 当成 C++ 的内置类型，和 `int`、`double` 同等看待。

### 常用接口

```cpp
// 构造
std::string s1 = "hello";
std::string s2("world");
std::string s3(5, 'x');                        // "xxxxx"
std::string s4(s1.begin(), s1.begin() + 3);    // "hel"

// 长度与容量
s1.size();        // 5，O(1)
s1.length();      // 等价于 size()
s1.capacity();    // 当前分配的容量
s1.empty();

// 拼接
s1 + "," + s2;
s += ", ";  s.append(s2);

// 查找：找不到返回 npos
s.find("world");                       // 7
s.find("xyz") == std::string::npos;    // true
s.rfind(...);  s.find_first_of(...);

// 子串
s.substr(7);        // 从 7 到末尾
s.substr(7, 3);     // 从 7 取 3 个字符

// 比较：可以直接用运算符
s1 == "hello";      // true

// 元素访问
s1[0];              // 不检查越界，越界是未定义行为
s1.at(0);           // 检查越界，越界抛 std::out_of_range

// 转 C 风格
s1.c_str();         // const char*，保证以 '\0' 结尾
s1.data();          // C++11 起与 c_str() 等价
```

实测全部正常，其中：

```
      at(100) 抛出 out_of_range
      （operator[] 越界是未定义行为，不会抛异常）
```

**`[]` 和 `at()` 的区别要记住**：前者快但不检查，后者安全但有检查开销。
循环内部用 `[]`，边界不确定时用 `at()`。

## 三、底层实现的三种策略

参考文档里提了一个好问题：`sizeof(std::string)` 是多少？
答案在不同编译器、甚至同一编译器的不同版本上都不同 ——
因为实现策略不同。

历史上有三种实现，每种都要保存三类信息：字符串大小、容量、内容本身。

### 策略一：Eager Copy（深拷贝）

最简单：任何情况都拷贝字符串内容。

```cpp
class string {
    Allocator _allocator;
    size_t _size;
    size_t _capacity;
    char* _pointer;        // 独立的堆内存
};
```

缺点：频繁复制而不改内容时效率低下。

### 策略二：COW（Copy-On-Write，写时复制）

两个 string 复制构造或赋值时，**不复制内容，只增加引用计数**，
指针做浅拷贝，执行效率 O(1)。只有需要修改其中一个时，才执行真正的复制。

GNU 4.8.4 采用的实现只有一个数据成员：

```cpp
class string {
    char* _pointer;        // 指向一块内存，引用计数在字符串数据【之前】
};
```

参考文档留了个问题：**引用计数为什么不和 string 的数据成员放在一起？**

因为引用计数必须被**所有共享该字符串的 string 对象**看到。
如果放在 string 对象内部，每个对象有自己的一份，就无法共享了。
所以它被放在堆上字符串数据的前面（一块内存同时存计数、长度、容量、内容）。

这个设计让 `sizeof(string)` 等于一个指针的大小。实测验证：

```bash
$ g++ -std=c++17 -D_GLIBCXX_USE_CXX11_ABI=0 cow.cpp   # 切到旧 ABI
sizeof(string) = 8
```

而默认的新 ABI：

```
sizeof(string) = 32
```

8 字节正是"只有一个指针"的 COW 布局。
（注：我在本机用旧 ABI 实测时，拷贝后两个 string 的 `data()`
地址并不相同 —— 说明这个 GCC 版本的旧 ABI 实现并未真正共享缓冲区。
所以这里只能确认布局是单指针的，不能确认共享行为。）

**COW 的线程安全问题**：涉及修改引用计数的操作在多线程下是否安全？
GNU 4.8.4 的实现用了**原子操作**来保证。

### 策略三：SSO（Short String Optimization，短字符串优化）

目前 VC++、GNU 5.x+、Clang++ 都采用 SSO。

核心思想：**程序里用到的字符串大部分都很短**，
而 64 位机器上一个 `char*` 指针就占 8 字节 ——
既然拷贝时反正要复制一个指针，对小字符串为什么不直接复制整个内容？

```cpp
class string {
    union Buffer {
        char* _pointer;        // 长字符串：指向堆
        char _local[16];       // 短字符串：直接存在对象里
    };
    Buffer _buffer;
    size_t _size;
    size_t _capacity;
};
```

### SSO 实测

我用"比较字符串数据地址与对象自身地址"的方法探测阈值：

```cpp
auto uses_sso = [](const std::string& s) {
    auto obj_addr = reinterpret_cast<const char*>(&s);
    auto data_addr = s.data();
    return data_addr >= obj_addr && data_addr < obj_addr + sizeof(std::string);
};
```

实测结果：

```
      sizeof(std::string) = 32（libstdc++ 的 SSO 实现）
        长度 0: SSO（数据在对象内）, capacity=15
        长度 5: SSO（数据在对象内）, capacity=15
        长度 15: SSO（数据在对象内）, capacity=15
        长度 16: 堆分配, capacity=16
        长度 20: 堆分配, capacity=20
        长度 100: 堆分配, capacity=100
```

**libstdc++ 的阈值精确地是 15 个字符**（16 字节含 `'\0'`）。
长度 15 时 `capacity` 仍是 15（栈内缓冲），16 时就变成了堆分配。

### SSO 的性能意义

实测 200 万次字符串拷贝：

```
        短字符串(10 字符，走 SSO) : 19284 us
        长字符串(100 字符，走堆)  : 41351 us
```

短字符串快**一倍多**，因为完全不需要堆分配 ——
回想 [02-01](../part2_memory/02-01_memory_layout.md) 的实测：
堆分配比栈分配慢约 26 倍。

### 最佳策略：folly::fbstring

三种方式各有所长也各有缺陷。Facebook 的 folly 库实现了 `fbstring`，
**根据字符串长度使用不同策略**，每个对象固定 24 字节：

| 长度 | 策略 | 布局 |
|---|---|---|
| 0~22 | SSO | 23 字节存内容（含 `'\0'`），1 字节存长度 |
| 23~255 | Eager Copy | 8 字节指针 + 8 字节 size + 8 字节 capacity |
| >255 | COW | 8 字节指针（字符串和引用计数）+ size + capacity |

### 线程安全性

两个线程同时操作**同一个** string 不可能线程安全 ——
出于性能考虑，C++ 没有为 string 实现线程安全（毕竟不是所有程序都用多线程）。

但两个线程操作**各自独立的** string 必须是安全的。
COW 通过原子操作维护引用计数来实现这一点。

而 CPU 的原子操作虽然比互斥锁好得多，仍然有性能损失：

- 阻止了 CPU 的乱序执行
- 两个 CPU 对同一地址做原子操作会导致 cache 失效，要重新从内存读
- 系统通常会锁住比目标地址更大的一片区域，影响逻辑上不相关的地址访问

**这正是多核时代各大编译器厂商都转向 SSO 的原因** ——
SSO 完全没有共享状态，天然没有这些问题。

## 四、常见性能陷阱

### 陷阱一：循环中反复拼接不预留容量

```cpp
std::string s;
for (int i = 0; i < 100000; ++i) s += 'x';      // 多次重新分配

std::string s2;
s2.reserve(100000);                              // 预留
for (int i = 0; i < 100000; ++i) s2 += 'x';
```

实测：

```
        不 reserve: 349 us
        先 reserve: 268 us
```

差距不算大（因为容量是按倍数增长的，重分配次数是 O(log n)），
但在热路径上仍然值得 `reserve`。

### 陷阱二：substr 会拷贝

```cpp
std::string sub = long_str.substr(10, 50);      // 分配 + 拷贝 50 字节
```

C++17 可以用 `string_view` 避免（见下节）。

### 陷阱三：参数传递方式

```cpp
void f(std::string s);          // 总是拷贝（除非调用方 move）
void f(const std::string& s);   // 不拷贝，但 const char* 实参要构造临时 string
void f(std::string_view s);     // 不拷贝，接受所有形式  <- C++17 首选
```

规则：

- **只读**：用 `std::string_view`（C++17）或 `const std::string&`
- **要存起来**：按值传 + `std::move`

```cpp
class Widget {
public:
    explicit Widget(std::string name) : _name(std::move(name)) {}   // 按值 + move
private:
    std::string _name;
};
```

## 五、string_view `[C++17]`

### 解决什么问题

一个只读字符串参数，调用方可能传 `std::string`、`const char*`、
字面量、或者某个子串。C++17 之前的选择都不好：

```cpp
void f(const std::string& s);
f("literal");                   // 构造一个临时 string！有分配和拷贝
f(long_str.substr(0, 5));       // 又一次分配和拷贝
```

`string_view` 是一个**非拥有的字符串视图** ——
只有一个指针和一个长度，不管理内存：

```cpp
std::size_t count_chars(std::string_view sv, char c);

count_chars(str, 'l');                                // std::string
count_chars(cstr, 'l');                               // const char*
count_chars("hello world", 'l');                      // 字面量
count_chars(std::string_view(str).substr(0, 5), 'l'); // 子串，零拷贝
```

实测一个函数接受了全部四种形式。

```
      sizeof(string_view) = 16（一个指针 + 一个长度）
```

### 零拷贝的子串操作

```cpp
std::string_view sv = str;
sv.substr(6);               // 不分配内存，只是调整指针和长度
sv.remove_prefix(6);        // 原地调整
sv.remove_suffix(3);
```

实测性能对比（100 万次取子串）：

```
        string::substr:      37911 us
        string_view::substr: 11829 us
```

**快约 3.2 倍**，因为完全没有内存分配。

### 三个陷阱

**陷阱一：指向临时对象**

```cpp
std::string_view sv = get_string();      // 临时 string 立刻销毁 -> 悬垂
std::string_view sv2 = s1 + s2;          // 同样危险
```

这是最常见的错误。`string_view` 不延长被指向对象的寿命
（和 `const T&` 不同，见 [01-03](../part1_basics/01-03_reference.md)）。

**陷阱二：返回局部 string 的 view**

```cpp
std::string_view bad() {
    std::string local = "temporary";
    return local;            // 返回后 local 销毁
}
```

**陷阱三：不保证以 `'\0'` 结尾**

```cpp
std::string s = "hello world";
std::string_view sv(s.data(), 5);        // "hello"，但第 6 个字符是 ' ' 不是 '\0'

std::cout << sv;                          // ok，ostream 知道长度
// printf("%s", sv.data());               // 错误！会一路打印到 "hello world"
```

实测确认打印正常（因为 `operator<<` 用长度），但 `data()` 不能当 C 字符串用。

注意 `string_view` **故意没有 `c_str()`** —— 这是设计决定，
提醒你它不保证终止符。需要 C 字符串时必须构造 `std::string`：

```cpp
std::string tmp(sv);
some_c_api(tmp.c_str());
```

### 使用规则

1. **只做函数参数**，不做类成员、不做返回值
2. 被指向的字符串必须比 view 活得久
3. 需要 C 字符串时先构造 `std::string`
4. 不要存储 `string_view`（除非你能确保生命周期）

```cpp
// 好
void parse(std::string_view input);

// 危险
class Config {
    std::string_view _name;      // 谁保证被指向的字符串还活着？
};
```

## 六、字符串与数值的转换

### C++11：to_string / stoi 系列

```cpp
std::string s = std::to_string(42);
int n = std::stoi("123");
double d = std::stod("3.14");
```

问题：失败时**抛异常**（`invalid_argument` / `out_of_range`），
而且会分配内存、受 locale 影响。

实测 `stoi("not a number")` 抛出 `invalid_argument`。

### C++17：from_chars / to_chars

```cpp
const char* text = "12345abc";
int value = 0;
auto [ptr, ec] = std::from_chars(text, text + strlen(text), value);
// value = 12345, ptr 指向 "abc", ec == std::errc{}

char buf[32];
auto [out_ptr, out_ec] = std::to_chars(buf, buf + sizeof(buf), 54321);
```

实测：

```
      from_chars: value=12345 剩余="abc" 成功=1
      to_chars: "54321" 成功=1
```

特点：

- **无异常** —— 通过返回的 `errc` 报告错误
- **无内存分配**
- **不受 locale 影响**
- 返回解析停止的位置，方便继续解析

**这是性能最好的转换方式**，适合解析大量数据（日志、CSV、JSON）。
代价是接口比 `stoi` 啰嗦。

## 七、小结

| 要点 | 说明 |
|---|---|
| C 风格字符串的问题 | O(n) 长度、手工内存管理、缓冲区溢出、不能用 `==` |
| `[]` vs `at()` | 前者不检查（UB），后者抛 `out_of_range` |
| 三种底层实现 | Eager Copy、COW（引用计数+原子操作）、**SSO** |
| 为什么转向 SSO | COW 的原子操作在多核下有 cache 失效等代价 |
| SSO 实测 | libstdc++ 阈值是 **15 字符**，`sizeof(string)` 为 32 |
| SSO 的收益 | 短字符串拷贝快一倍多（无堆分配）|
| COW 的布局 | 单指针（实测旧 ABI `sizeof` 为 8），引用计数在数据前面 |
| folly::fbstring | 按长度混合三种策略，固定 24 字节 |
| 性能实践 | 循环拼接前 `reserve`；只读参数用 `string_view` |
| `string_view` `[C++17]` | 指针 + 长度，零拷贝；子串操作快 3.2 倍 |
| **string_view 的三个陷阱** | 指向临时对象、返回局部 view、**不保证 `'\0'` 结尾** |
| string_view 的规则 | **只做函数参数**，不做成员和返回值 |
| 数值转换 | `stoi` 抛异常；`from_chars`/`to_chars` `[C++17]` 无异常无分配，最快 |

> 下一章：[07-02 容器全景](07-02_containers.md)
