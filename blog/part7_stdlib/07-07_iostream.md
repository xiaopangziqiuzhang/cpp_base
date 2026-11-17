# 07-07 IO 流体系

> demo：[`07_07_iostream.cpp`](../../demos/part7_stdlib/07_07_iostream.cpp)

## 一、"流"的概念

输入输出的本质是在**内存**和**外部设备**之间传输数据。
C++ 把这个过程抽象成"流"：把 IO 看成字节的有序流动。

- **输入**：从设备流向内存
- **输出**：从内存流向设备

这个抽象的价值是**统一接口** ——
控制台、文件、字符串、网络（通过自定义 streambuf）都用同一套语法：

```cpp
std::cout << value;          // 控制台
ofs << value;                 // 文件
oss << value;                 // 字符串
```

对比 C 的做法（`printf` / `fprintf` / `sprintf` 三套函数），
流的接口统一性和可扩展性都更好 —— 自定义类型只要重载 `operator<<`
就能用于所有流。

## 二、类型体系

```
                    ios_base
                       |
                  basic_ios
                  /        \
        basic_istream    basic_ostream
          /      \        /       \
    ifstream  istringstream  ofstream  ostringstream
                  \        /
                basic_iostream
                  /        \
             fstream    stringstream
```

**`basic_iostream` 是虚继承自 `basic_ios` 的** ——
这是标准库里最著名的菱形继承（见 [04-03](../part4_oop/04-03_diamond.md)）。
如果不用虚继承，`fstream` 就会有两份 `basic_ios` 状态
（一份给输入、一份给输出），无法共享文件位置和错误标志。

三类流：

| 类别 | 头文件 | 类型 |
|---|---|---|
| 标准流 | `<iostream>` | `cin`、`cout`、`cerr`、`clog` |
| 文件流 | `<fstream>` | `ifstream`、`ofstream`、`fstream` |
| 串流 | `<sstream>` | `istringstream`、`ostringstream`、`stringstream` |

所有这些都是 `basic_xxx<char>` 的别名。宽字符版本是 `wcin`、`wifstream` 等。

## 三、流的状态

四个状态位：

| 状态位 | 含义 | 能否恢复 |
|---|---|---|
| `goodbit` | 一切正常（值为 0）| — |
| `eofbit` | 到达文件尾 | 可以 clear |
| `failbit` | 操作失败（如格式错误）| 可以恢复 |
| `badbit` | 严重错误（底层设备故障）| 通常无法恢复 |

实测状态变化：

```cpp
std::istringstream iss("42 abc");
int n = 0;
iss >> n;        // 成功
```

```
      读入 42 后: good=1 eof=0 fail=0 bad=0
```

```cpp
iss >> n;        // 尝试把 "abc" 读成 int
```

```
      读 "abc" 失败后: good=0 eof=0 fail=1 bad=0
      n 的值 = 0（C++11 起失败时被置为 0）
```

注意最后一点：**C++11 起，格式化输入失败时会把目标变量置为 0**
（C++98 是保持原值不变）。这个改动让失败后的状态更可预测。

### 状态查询与恢复

```cpp
s.good();     s.eof();     s.fail();     s.bad();
s.rdstate();                  // 返回状态位的组合
s.clear();                     // 清除所有状态位
s.clear(std::ios::eofbit);     // 设置为指定状态
s.setstate(std::ios::failbit); // 添加状态位
```

正确的恢复流程是 **clear + 跳过坏数据**：

```cpp
std::istringstream iss("abc 42");
int v = 0;
if (!(iss >> v)) {
    iss.clear();                  // 1. 清除失败状态
    std::string skipped;
    iss >> skipped;                // 2. 跳过坏数据
    iss >> v;                      // 3. 重试
}
```

实测成功读到 42。只 `clear()` 不跳过坏数据的话，下次读取还会失败
（而且会无限循环）—— 这是个常见 bug。

### 流转 bool

```cpp
if (s) { }                     // 等价于 !s.fail()
while (iss >> value) { }        // 最常用的惯用法
while (std::getline(ifs, line)) { }
```

这个转换是 `explicit operator bool()`（C++11 起），
所以不会意外参与算术运算。

**不要用 `while (!ifs.eof())`** —— 经典错误：

```cpp
// 错误：最后一行会被处理两次
while (!ifs.eof()) {
    std::getline(ifs, line);
    process(line);              // 读失败时 line 是空的，但还是被处理了
}

// 正确
while (std::getline(ifs, line)) { process(line); }
```

原因：`eofbit` 是在**读取失败之后**才被设置的，
所以最后一次读取失败时循环条件还是 true。

## 四、缓冲区

### 为什么要缓冲区

1. **系统调用很贵** —— 每个字符调一次 `write` 是灾难
2. **块设备按块读写** —— 攒够一块再操作效率高
3. **速度不匹配** —— CPU 快、磁盘慢，缓冲区解耦两者

### 三种缓冲模式

| 模式 | 刷新时机 | 默认用于 |
|---|---|---|
| 全缓冲（`_IOFBF`）| 缓冲区满 | 文件 |
| 行缓冲（`_IOLBF`）| 遇到换行 | 终端的 stdout |
| 无缓冲（`_IONBF`）| 立即 | stderr |

### 实测：endl 的代价

```cpp
// 写 20 万行
for (int i = 0; i < kLines; ++i) ofs << "line " << i << "\n";        // 不刷新
for (int i = 0; i < kLines; ++i) ofs << "line " << i << std::endl;    // 每行刷新
```

实测：

```
      写 200000 行:
        用 "\n"（不刷新）:   12 ms
        用 std::endl（每行刷）: 180 ms
```

**慢 15 倍。**

`std::endl` 等价于 `"\n" << std::flush`。
`flush` 强制把缓冲区内容写到设备，意味着一次系统调用。

**规则：换行用 `"\n"`，只在确实需要立即可见时用 `std::endl`。**

需要立即可见的场合：

- 交互式提示（但 `cin`/`cout` 的绑定已经处理了，见下节）
- 崩溃前的日志（更好的办法是用 `cerr`）
- 进度条

### 刷新的其他方式

```cpp
std::cout << std::flush;              // 显式刷新
std::cout.flush();
std::cout << std::unitbuf;            // 之后每次输出都刷新
```

## 五、cout / cerr / clog 的区别

| 流 | 目标 | 缓冲 |
|---|---|---|
| `cout` | stdout | 全缓冲（重定向时）或行缓冲（终端）|
| `cerr` | stderr | **无缓冲**，立即输出 |
| `clog` | stderr | **有缓冲** |

**为什么 `cerr` 无缓冲**：程序崩溃时，缓冲区里的内容会丢失。
错误信息必须保证输出，所以不缓冲。

**为什么还要有 `clog`**：大量日志时需要缓冲的性能。
它和 `cerr` 一样写到 stderr（所以能和 stdout 分开重定向），
但有缓冲。

实践：

```cpp
std::cout << "正常输出\n";           // 程序的输出
std::cerr << "错误: ...\n";          // 错误信息，保证可见
std::clog << "调试日志...\n";        // 大量日志
```

### cin 和 cout 的绑定

```cpp
std::cin.tie();        // 返回 &std::cout
```

实测确认 `cin.tie()` 是 `cout`。

含义：**任何从 `cin` 读取之前，`cout` 会自动 flush**。
这保证了提示能先显示出来：

```cpp
std::cout << "请输入: ";     // 没有 endl，但因为绑定，读取前会自动刷新
int n;
std::cin >> n;
```

没有这个绑定的话，提示会卡在缓冲区里，用户看到的是空白屏幕。

### 竞赛常用的加速技巧

```cpp
std::ios::sync_with_stdio(false);     // 解除与 C stdio 的同步
std::cin.tie(nullptr);                 // 解除 cin/cout 绑定
```

`sync_with_stdio(false)` 的效果：C++ 流不再和 `printf`/`scanf`
共用缓冲区，于是可以用自己的高效缓冲。通常能快好几倍。

**代价**：之后不能再混用 `printf`/`scanf`（输出顺序会乱）。

## 六、操纵符

```cpp
// 进制
std::cout << std::hex << 255;                    // ff
std::cout << std::oct << 255;                     // 377
std::cout << std::dec << 255;                     // 255
std::cout << std::showbase << std::hex << 255;    // 0xff

// 宽度与对齐
std::cout << std::setw(10) << 42;                             // "        42"
std::cout << std::left << std::setw(10) << 42;                 // "42        "
std::cout << std::setfill('0') << std::setw(10) << 42;         // "0000000042"

// 浮点
std::cout << std::setprecision(3) << pi;                        // 3.14
std::cout << std::fixed << std::setprecision(4) << pi;          // 3.1416
std::cout << std::scientific << pi;                              // 3.1416e+00
std::cout << std::defaultfloat;                                  // 恢复默认

// 布尔
std::cout << std::boolalpha << true;                             // true
```

实测全部正确：

```
      进制: ff(hex) 377(oct) 255(dec)
      带前缀: 0xff
        [        42]（右对齐，默认）
        [42        ]（左对齐）
        [0000000042]（填 0）
        默认:      3.14159
        precision(3): 3.14
        fixed:     3.1416
        scientific: 3.1416e+00
        boolalpha: true / false
```

### 最容易踩的坑：持久性不一致

**`setw` 只影响下一次输出，其他大多是持久的。**

```cpp
std::cout << std::setw(10) << 1 << 2;      // "         12" —— setw 只管第一个
std::cout << std::hex << 255 << 256;        // "ff100" —— hex 持久生效
```

所以设了 `hex` 忘了改回 `dec`，后面所有数字都是十六进制 ——
这个 bug 很隐蔽，因为它影响的是后续**无关的**代码。

**稳妥做法：用完就恢复**，或者用 RAII 保存/恢复流状态：

```cpp
class StateGuard {
public:
    explicit StateGuard(std::ios& s) : _s(s), _flags(s.flags()), _prec(s.precision()) {}
    ~StateGuard() { _s.flags(_flags); _s.precision(_prec); }
private:
    std::ios& _s;
    std::ios_base::fmtflags _flags;
    std::streamsize _prec;
};
```

（C++20 的 `std::format` 彻底避开了这个问题 ——
格式是参数而不是流的状态。）

## 七、文件 IO

### 文件模式

| 模式 | 含义 |
|---|---|
| `ios::in` | 读 |
| `ios::out` | 写（默认截断）|
| `ios::app` | 追加（每次写都到末尾）|
| `ios::ate` | 打开后定位到末尾（但可以 seek 回去）|
| `ios::trunc` | 截断已有内容 |
| `ios::binary` | 二进制模式（不转换换行符）|

可以用 `|` 组合。注意 `app` 和 `ate` 的区别：
`app` 是**每次写入**都到末尾（即使你 seek 了），
`ate` 只是**打开时**定位到末尾。

### RAII 自动关闭

```cpp
{
    std::ofstream ofs(path);
    if (!ofs) { /* 打开失败 */ }
    ofs << "data";
}   // 析构自动关闭（见 02-06）
```

不需要手工 `close()`。需要检查关闭是否成功（flush 可能失败）时才显式调用。

### 逐行读取：标准写法

```cpp
std::ifstream ifs(path);
std::string line;
while (std::getline(ifs, line)) {
    process(line);
}
```

实测正确读出三行。

`std::getline` 的第三个参数可以指定分隔符：

```cpp
std::getline(ifs, field, ',');      // 按逗号分割
```

### 一次读入整个文件

两种常用写法：

```cpp
// 写法一：先获取大小再 read（最快）
std::ifstream ifs(path, std::ios::binary | std::ios::ate);
auto size = ifs.tellg();
ifs.seekg(0);
std::string content(static_cast<std::size_t>(size), '\0');
ifs.read(content.data(), size);

// 写法二：istreambuf_iterator（最简洁）
std::ifstream ifs(path);
std::string content((std::istreambuf_iterator<char>(ifs)),
                    std::istreambuf_iterator<char>());
```

实测两种都正常。第二种写法注意那对多余的括号 ——
防止 most vexing parse（见 [01-11](../part1_basics/01-11_uniform_init.md)）。

### 二进制读写

```cpp
std::vector<int> data{1, 2, 3, 4, 5};
std::ofstream ofs(path, std::ios::binary);
ofs.write(reinterpret_cast<const char*>(data.data()),
          static_cast<std::streamsize>(data.size() * sizeof(int)));

std::ifstream ifs(path, std::ios::binary);
std::vector<int> loaded(5);
ifs.read(reinterpret_cast<char*>(loaded.data()),
         static_cast<std::streamsize>(loaded.size() * sizeof(int)));
```

实测正确读回 `1 2 3 4 5`。

**注意：二进制格式不跨平台** —— 字节序、结构体对齐、
类型大小都可能不同。跨平台的数据交换要用明确定义的格式
（protobuf、msgpack、或手工处理字节序）。

## 八、stringstream：格式转换

### 数值 ↔ 字符串

```cpp
// 数值 -> 字符串
std::ostringstream oss;
oss << "id=" << 42 << " pi=" << std::fixed << std::setprecision(2) << 3.14159;
oss.str();                      // "id=42 pi=3.14"

// 字符串 -> 数值
std::istringstream iss("42 3.14 hello");
int i; double d; std::string s;
iss >> i >> d >> s;
```

实测都正确。

### 分割字符串

标准库没有 `split` 函数（C++20 的 ranges 才补上），
用 `getline` + `stringstream` 是标准做法：

```cpp
std::string csv = "alice,30,engineer";
std::istringstream line_ss(csv);
std::string field;
while (std::getline(line_ss, field, ',')) {
    use(field);
}
```

实测分割出 `[alice]`、`[30]`、`[engineer]`。

### 复用 stringstream 要调两个函数

```cpp
std::ostringstream oss;
oss << "first";

oss.str("");        // 1. 清空内容
oss.clear();         // 2. 清除状态位
oss << "second";
```

实测两步都做了才正常。只调 `str("")` 不清状态的话，
如果之前有过失败，后续操作会一直失败 —— 这是常见 bug。

### 性能：简单转换不要用 stringstream

```cpp
// 20 万次 int 转字符串
std::ostringstream o; o << k; o.str();      // 每次构造一个 stringstream
std::to_string(k);
```

实测：

```
        ostringstream: 26 ms
        to_string:     5 ms
```

**`to_string` 快 5 倍。** `stringstream` 要维护格式状态、
locale、缓冲区，开销不小。

性能排序（见 [07-01](07-01_string.md)）：

```
std::to_chars (C++17)  >  std::to_string  >  ostringstream
```

`stringstream` 的价值在于**复杂格式化**和**混合类型解析**，
简单转换用专门的函数。

## 九、小结

| 要点 | 说明 |
|---|---|
| 流的抽象 | 统一了控制台/文件/字符串的接口；自定义类型重载 `operator<<` 即可接入 |
| 类型体系 | `basic_iostream` **虚继承**自 `basic_ios`，是标准库最著名的菱形继承 |
| 四个状态位 | `goodbit`/`eofbit`/`failbit`/`badbit` |
| 失败后的值 | C++11 起格式化输入失败会把目标置 0 |
| 恢复流程 | `clear()` + **跳过坏数据**，缺一步会无限循环 |
| 不要用 `while(!eof())` | `eofbit` 在读取失败后才设置，最后一行会处理两次 |
| 缓冲的三种模式 | 全缓冲（文件）、行缓冲（终端 stdout）、无缓冲（stderr）|
| **`std::endl` 的代价** | 等于 `"\n"` + flush；实测循环里用它**慢 15 倍** |
| `cerr` vs `clog` | 都写 stderr；`cerr` 无缓冲（崩溃时不丢），`clog` 有缓冲 |
| cin/cout 绑定 | 读 `cin` 前自动 flush `cout`，保证提示可见 |
| 加速技巧 | `sync_with_stdio(false)` + `cin.tie(nullptr)`，之后不能混用 printf |
| 操纵符的坑 | **`setw` 只管下一次，其他是持久的**；用完要恢复 |
| 文件模式 | `app`（每次写都到末尾）vs `ate`（只是打开时定位）|
| 逐行读 | `while (std::getline(ifs, line))` |
| 读全文件 | `tellg` + `read`（快）或 `istreambuf_iterator`（简洁）|
| 二进制 | 不跨平台（字节序、对齐、类型大小）|
| stringstream 复用 | **`str("")` + `clear()` 都要调** |
| 性能 | 简单转换用 `to_chars` / `to_string`，**不要用 stringstream**（慢 5 倍）|

Part 7 到此结束。

> 下一章：[08-01 线程](../part8_concurrency/08-01_thread.md)
