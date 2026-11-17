# 07-06 正则与文件系统

> 标准：`regex` `[C++11]`；`filesystem` `[C++17]`
> demo：[`07_06_regex_filesystem.cpp`](../../demos/part7_stdlib/07_06_regex_filesystem.cpp)

## 一、regex：三个基本操作

C++11 把正则表达式加进标准库。三个核心函数：

| 函数 | 语义 |
|---|---|
| `regex_match` | **整个字符串**必须完全匹配 |
| `regex_search` | 查找**子串** |
| `regex_replace` | 替换 |

```cpp
std::regex date_re(R"(\d{4}-\d{2}-\d{2})");

std::regex_match("2024-01-15", date_re);          // true
std::regex_match("date: 2024-01-15", date_re);    // false！有多余内容

std::smatch m;
std::regex_search(text, m, date_re);               // 找子串
m[0];                                               // 匹配到的内容
m.position(0);                                      // 位置

std::regex_replace(text, date_re, "YYYY-MM-DD");
```

实测：

```
      regex_match（必须【完全】匹配）:
        "2024-01-15" -> 1
        "date: 2024-01-15" -> 0（有多余内容，不匹配）
      regex_search（查找【子串】）:
        找到: 2024-01-15 位置 6
      regex_replace（替换）:
        date: YYYY-MM-DD end
```

**`match` 和 `search` 的区别是最常见的困惑点** ——
想找子串用了 `regex_match` 就会一直返回 false。

注意正则用了**原始字符串字面量** `R"(...)"`
（见 [01-09](../part1_basics/01-09_literals.md)）：

```cpp
"\\d{4}-\\d{2}"        // 传统写法：双层转义
R"(\d{4}-\d{2})"       // 原始写法：和正则本身一致
```

写正则一律用原始字符串。

## 二、捕获组

```cpp
std::regex log_re(R"((\w+)\s+\[(\d{4}-\d{2}-\d{2})\]\s+(.+))");
std::string line = "ERROR [2024-01-15] disk full";

std::smatch m;
if (std::regex_match(line, m, log_re)) {
    m[0];      // 完整匹配
    m[1];      // 第一个捕获组
    m[2];
    m[3];
    m.size();  // 4（完整匹配 + 3 个组）
}
```

实测：

```
      完整匹配 m[0] = ERROR [2024-01-15] disk full
      级别  m[1] = ERROR
      日期  m[2] = 2024-01-15
      消息  m[3] = disk full
      组数 m.size() = 4
```

替换时用 `$N` 引用捕获组：

```cpp
std::regex_replace(line, log_re, "[$2] $1: $3");
// 输出: [2024-01-15] ERROR: disk full
```

实测确认重排成功。

`std::smatch` 是 `std::match_results<std::string::const_iterator>` 的别名。
注意它持有的是**迭代器**，所以：

```cpp
std::smatch m;
std::regex_search(get_temporary_string(), m, re);     // 危险！
m[0];                                                  // 悬垂：临时字符串已销毁
```

C++11 起这个重载被 `= delete` 禁止了（对右值 string），但要知道原因。

## 三、遍历所有匹配

### sregex_iterator：遍历每个匹配

```cpp
std::string text = "a=1, b=22, c=333";
std::regex pair_re(R"((\w+)=(\d+))");

auto begin = std::sregex_iterator(text.begin(), text.end(), pair_re);
auto end = std::sregex_iterator();
for (auto it = begin; it != end; ++it) {
    (*it)[1];      // 键
    (*it)[2];      // 值
}
```

实测：

```
        a => 1
        b => 22
        c => 333
```

### sregex_token_iterator：做分割

```cpp
std::regex comma(R"(\s*,\s*)");
std::sregex_token_iterator tok(text.begin(), text.end(), comma, -1);  // -1 = 取分隔符之间
std::sregex_token_iterator tok_end;
for (; tok != tok_end; ++tok) { use(*tok); }
```

实测分割出 `[a=1]`、`[b=22]`、`[c=333]`。

第四个参数的含义：
`-1` 取分隔符之间的部分（即分割），
`0` 取整个匹配，`N` 取第 N 个捕获组。

这是 C++ 里做"按正则分割字符串"的标准方式 ——
标准库没有 `split` 函数（C++20 的 ranges 才补上）。

## 四、语法与选项

### 六种正则语法

```cpp
std::regex re(pattern, std::regex::ECMAScript);   // 默认，功能最全
std::regex re(pattern, std::regex::basic);         // POSIX BRE
std::regex re(pattern, std::regex::extended);      // POSIX ERE
std::regex re(pattern, std::regex::awk);
std::regex re(pattern, std::regex::grep);
std::regex re(pattern, std::regex::egrep);
```

默认的 ECMAScript 支持最多特性（非贪婪、前向断言、`\b` 等），通常就用它。

### 常用标志

```cpp
std::regex ci(R"(hello)", std::regex::icase);          // 忽略大小写
std::regex ml(pattern, std::regex::multiline);          // [C++17] ^ $ 匹配行首行尾
std::regex opt(pattern, std::regex::optimize);          // 构造慢、匹配快
std::regex nosub(pattern, std::regex::nosubs);          // 不记录捕获组，更快
```

实测 `icase` 让 `"HELLO"` 匹配 `hello`。

`optimize` 适合"构造一次、匹配很多次"的场景。

## 五、regex 的性能问题（重要）

这是 `std::regex` 最需要注意的地方。实测 2 万次匹配：

```
        循环内构造 regex: 6169937 us      <- 6.17 秒！
        regex 提到循环外: 60319 us        <- 60 毫秒
        手写 find 解析:   154 us          <- 0.15 毫秒
```

两个结论：

### 结论一：regex 对象必须复用

循环内构造比提到外面慢 **102 倍**。

`std::regex` 的构造函数要**编译正则表达式**（构建状态机），
这个过程比匹配本身贵得多。

```cpp
// 错误
for (const auto& line : lines) {
    std::regex re(pattern);                 // 每次都编译！
    if (std::regex_search(line, re)) { }
}

// 正确
std::regex re(pattern);                      // 编译一次
for (const auto& line : lines) {
    if (std::regex_search(line, re)) { }
}

// 更好：如果正则是常量，用 static（注意线程安全）
static const std::regex re(pattern);
```

### 结论二：std::regex 本身不快

手写解析比复用的 regex 还快 **390 倍**。

`std::regex` 的实现以"功能完备和标准符合"为目标，
性能不是优先项。libstdc++ 的实现尤其慢（它用递归回溯，
而且 `std::regex` 的接口设计阻碍了优化）。

实践建议：

| 场景 | 做法 |
|---|---|
| 格式固定、性能敏感 | **手写解析**（`find`、`substr`、`from_chars`）|
| 配置文件、一次性脚本 | `std::regex` 够用 |
| 高性能正则需求 | 用 RE2、PCRE2、Hyperscan 等专门库 |
| 用户提供的模式 | `std::regex`（功能完备、安全）|

另外注意 `std::regex` 有**灾难性回溯**的风险 ——
形如 `(a+)+b` 的模式配上恶意输入会指数级爆炸。
处理不可信输入时要用 RE2（保证线性时间）。

## 六、filesystem：路径操作 `[C++17]`

C++17 之前，操作文件路径只能用平台 API（POSIX 的 `opendir`、
Windows 的 `FindFirstFile`）或 Boost.Filesystem。

### 兼容性处理

filesystem 在 GCC 8 及更早版本位于 `<experimental/filesystem>`
且需要链接 `-lstdc++fs`。用 `__has_include`
（见 [06-05](../part6_modern_core/06-05_cpp17.md)）处理：

```cpp
#if __has_include(<filesystem>)
#  include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#  include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
```

本书的 demo 就用了这个手法。

### 路径分解

```cpp
fs::path p = "/home/user/docs/report.tar.gz";

p.parent_path();      // "/home/user/docs"
p.filename();          // "report.tar.gz"
p.stem();              // "report.tar"     <- 只去掉最后一个扩展名
p.extension();         // ".gz"            <- 只取最后一个
p.root_path();         // "/"
p.is_absolute();       // true
```

实测：

```
        父目录 parent_path():    "/home/user/docs"
        文件名 filename():       "report.tar.gz"
        主干   stem():           "report.tar"
        扩展名 extension():      ".gz"（只取最后一个）
```

注意 `.tar.gz` 这类双扩展名 —— `extension()` 只返回 `.gz`。
要处理双扩展名得自己判断。

### 路径拼接：用 / 运算符

```cpp
fs::path dir = "/tmp";
fs::path file = dir / "subdir" / "file.txt";      // "/tmp/subdir/file.txt"
```

**自动使用平台的分隔符** —— 不用手工拼 `"/"` 或 `"\\"`，
也不用担心重复的分隔符。

这是 filesystem 最实用的功能之一。对比手工拼接：

```cpp
std::string path = dir + "/" + name;      // Windows 上也能用但不规范；
                                           // dir 末尾已有 "/" 就会出现 "//"
```

### 路径规范化

```cpp
fs::path messy = "/tmp/./subdir/../file.txt";
messy.lexically_normal();                   // "/tmp/file.txt"

fs::path target = "/tmp/a/c/d.txt";
fs::path base = "/tmp/a/b";
target.lexically_relative(base);            // "../c/d.txt"
```

实测两者都正确。

`lexically_*` 系列是**纯字符串操作**（不访问文件系统），
所以快而且不会失败。对应的 `fs::canonical`、`fs::relative`
会访问文件系统（能解析符号链接，但要求路径存在）。

## 七、filesystem：文件系统操作

### 查询

```cpp
fs::exists(p);
fs::is_directory(p);
fs::is_regular_file(p);
fs::is_symlink(p);
fs::file_size(p);                  // 注意：标了 [[nodiscard]]
fs::last_write_time(p);
fs::space(p);                       // 磁盘空间信息
```

实测：

```
        exists: 1
        is_directory: 1
        is_regular_file: 1
        file_size: 17 字节
        space().available: 354 GB
```

（写 demo 时碰到 `file_size` 的 `[[nodiscard]]` 警告 ——
标准库给它加了这个属性，忽略返回值会告警。
这正是 [01-09](../part1_basics/01-09_literals.md) 讲的那个特性的实际作用。）

### 遍历目录

```cpp
// 非递归
for (const auto& entry : fs::directory_iterator(dir)) {
    entry.path();
    entry.is_regular_file();        // 比 fs::is_regular_file(path) 快（复用了 stat 结果）
}

// 递归
for (const auto& entry : fs::recursive_directory_iterator(dir)) {
    if (entry.is_regular_file()) { /* ... */ }
}
```

实测：

```
      directory_iterator（非递归）:
        a.txt
        b.log
        c.txt
        sub1
      recursive_directory_iterator（递归）:
        a.txt
        b.log
        c.txt
        sub1/nested.txt
```

注意用 `entry.is_regular_file()` 而不是 `fs::is_regular_file(entry.path())` ——
`directory_entry` 缓存了文件状态，避免重复的系统调用。

按扩展名过滤：

```cpp
for (const auto& entry : fs::recursive_directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".txt") { /* ... */ }
}
```

### 创建、拷贝、删除

```cpp
fs::create_directory(p);                   // 单层
fs::create_directories(p);                  // 多层（像 mkdir -p）
fs::copy_file(from, to, fs::copy_options::overwrite_existing);
fs::copy(from, to, fs::copy_options::recursive);
fs::rename(from, to);
fs::remove(p);                              // 单个文件或空目录
fs::remove_all(p);                          // 递归删除，返回删除的条目数
```

实测 `remove_all` 删除了 8 个条目。

### 错误处理：两种重载

**大部分 filesystem 函数都有两个重载**：

```cpp
// 版本一：失败抛 fs::filesystem_error
try {
    auto sz = fs::file_size("/nonexistent");
} catch (const fs::filesystem_error& e) {
    e.code().message();        // "No such file or directory"
    e.path1();                  // 出错的路径
}

// 版本二：填 std::error_code，不抛异常
std::error_code ec;
auto sz = fs::file_size("/nonexistent", ec);
if (ec) { ec.message(); }
// 失败时 sz 是 static_cast<uintmax_t>(-1)
```

实测两种都正常：

```
        抛异常版本: No such file or directory
        error_code 版本: ec=No such file or directory 返回值=-1
```

**选择原则**：

- 文件操作失败是**预期情况**（文件可能不存在）→ 用 `error_code` 版本
- 失败是**异常情况**（配置文件必须存在）→ 用抛异常版本

这个双接口设计也出现在标准库的其他地方，是处理"可能失败但不算错误"
的推荐模式（见 [01-08](../part1_basics/01-08_exception.md)
关于"什么时候不该用异常"的讨论）。

### 并发注意

文件系统本质上是**共享可变状态** ——
`exists()` 返回 true 之后，文件可能立刻被别的进程删掉。

```cpp
// 有竞态
if (fs::exists(p)) {
    std::ifstream ifs(p);       // 这时文件可能已经没了
}

// 更好：直接尝试，失败再处理
std::ifstream ifs(p);
if (!ifs) { /* 处理失败 */ }
```

这叫 TOCTOU（Time-Of-Check to Time-Of-Use）问题，
是安全漏洞的常见来源。

## 八、小结

### regex

| 要点 | 说明 |
|---|---|
| 三个操作 | `regex_match`（**整串**）、`regex_search`（**子串**）、`regex_replace` |
| 最常见困惑 | 想找子串用了 `regex_match` → 永远 false |
| 捕获组 | `m[0]` 是完整匹配，`m[N]` 是第 N 组；替换用 `$N` |
| 遍历匹配 | `sregex_iterator`；分割用 `sregex_token_iterator(..., -1)` |
| 写法 | 一律用原始字符串 `R"(...)"` |
| **性能（实测）** | 循环内构造 regex 慢 **102 倍**；手写解析快 **390 倍** |
| 实践 | regex 对象必须复用；性能敏感场合手写解析或用 RE2 |
| 风险 | 灾难性回溯；处理不可信输入要用保证线性时间的库 |

### filesystem

| 要点 | 说明 |
|---|---|
| 兼容性 | GCC 8 及更早在 `<experimental/filesystem>` + `-lstdc++fs`；用 `__has_include` 处理 |
| 路径拼接 | **用 `/` 运算符**，自动处理平台分隔符 |
| 路径分解 | `parent_path`/`filename`/`stem`/`extension`；双扩展名要自己处理 |
| 规范化 | `lexically_normal`/`lexically_relative` 是纯字符串操作，不访问文件系统 |
| 遍历 | `directory_iterator` / `recursive_directory_iterator` |
| 性能提示 | 用 `entry.is_regular_file()` 而不是 `fs::is_regular_file(path)`（复用缓存）|
| **错误处理** | 每个函数有两个重载：抛异常 / 填 `error_code` |
| 选择原则 | 失败是预期情况 → `error_code`；失败是异常 → 抛异常 |
| 并发注意 | TOCTOU 问题：`exists()` 之后文件可能已消失，直接尝试操作更安全 |

> 下一章：[07-07 IO 流体系](07-07_iostream.md)
