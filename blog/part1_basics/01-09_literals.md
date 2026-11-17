# 01-09 字面量与语法糖

> 标准：原始字符串、二进制字面量、自定义字面量、`[[noreturn]]` `[C++11]`；
> 数字分隔符、`[[deprecated]]`、`s`/`h`/`ms` 后缀 `[C++14]`；
> `[[nodiscard]]`、`[[maybe_unused]]`、`[[fallthrough]]` `[C++17]`
> demo：[`01_09_literals.cpp`](../../demos/part1_basics/01_09_literals.cpp)

本章是一组小特性的集合。它们单个都不复杂，但都在消除一类具体的书写噪声或遗漏风险。

## 一、原始字符串字面量 `[C++11]`

### 解决什么问题

字符串里的反斜杠要转义，而反斜杠在两个场合天然高频：Windows 路径和正则表达式。

```cpp
// 传统写法
const char* path = "C:\\Users\\name\\Documents\\file.txt";
const char* re = "\\\\d+";      // 四个反斜杠，才表示正则里的 \\d
```

正则那行需要数一下才能确认写对了：想匹配字面的 `\d`，
正则层面要写 `\\d`，C++ 字符串层面每个 `\` 再翻倍，于是是四个。
这种"双层转义"是 bug 高发区。

### 写法

`R"(...)"` 之间的内容**原样保留**，不做任何转义处理：

```cpp
const char* path = R"(C:\Users\name\Documents\file.txt)";
const char* re = R"(\\d+)";
```

实测输出证明两种写法结果一致：

```
  传统: C:\Users\name\Documents\file.txt
  原始: C:\Users\name\Documents\file.txt
  正则(传统): \\d+   正则(原始): \\d+
```

### 内容里含 `)"` 怎么办

用自定义分隔符 `R"delim(...)delim"`，`delim` 可以是任意不超过 16 字符的序列：

```cpp
const char* tricky = R"json({"key": "value with )\" inside"})json";
```

实测输出：

```
  自定义分隔符: {"key": "value with )\" inside"}
```

嵌入 JSON、SQL、HTML 片段时，这个写法很实用 —— 顺带还给编辑器提供了语法高亮的线索。

### 可以跨行

原始字符串里的换行被原样保留：

```cpp
const char* multi = R"(第一行
第二行
第三行)";
```

这替代了 C++98 里用行尾反斜杠拼接多行字符串的写法。

组合前缀也可以：`LR"(...)"`（wchar_t）、`u8R"(...)"`（UTF-8）。

## 二、数字字面量的可读性

### 二进制字面量 `[C++11]`

```cpp
int binary = 0b10101010;      // 170
```

C++98 只有十进制、八进制（`0` 前缀）、十六进制（`0x` 前缀）。
写位掩码时只能手工换算成十六进制，或者用一串 `1 << n`。

### 数字分隔符 `[C++14]`

单引号作为分隔符，编译器完全忽略它：

```cpp
int million = 1'000'000;
long long big = 9'223'372'036'854'775'807LL;
int binary = 0b1010'1010;
int hex = 0xFF'FF;
constexpr unsigned kMask = 0b1111'0000'1111'0000;
```

实测输出：

```
  1'000'000 = 1000000
  0b1010'1010 = 170
  0xFF'FF = 65535  0755 = 493
  kMask = 61680 (0b1111000011110000)
```

分隔位置随意，不要求每三位一组 —— 按语义分组才有价值：
金额按三位、二进制按四位（对应一个十六进制位）、
电话号码或 IP 按自己的结构。

顺便提醒 `0755` 这个坑：以 `0` 开头是**八进制**，值是 493 不是 755。
文件权限用这个写法是对的，但不小心给十进制数补了前导零就会出错。

## 三、标准库提供的字面量后缀

### `s` 后缀：直接得到 `std::string` `[C++14]`

```cpp
auto cstr = "hello";     // const char*
auto sstr = "hello"s;    // std::string，需要 using namespace std::string_literals;

("hello"s + ", world");  // 直接相加，不需要先构造 string
```

不加 `s` 时字面量是 `const char*`，没有成员函数、不能直接相加
（`"a" + "b"` 是指针相加，编译错误）。`s` 后缀省掉了显式构造。

### chrono 后缀：时间单位写进代码 `[C++14]`

```cpp
auto timeout = 500ms;
auto hour = 1h;
```

这是个被低估的改进。对比一下：

```cpp
// 旧写法：500 是什么单位？要查文档
sleep(500);
wait_for(std::chrono::milliseconds(500));    // 明确但啰嗦

// 新写法
wait_for(500ms);
```

单位成了类型的一部分，传错单位是**编译错误**而不是运行时的诡异行为。
而且不同单位可以直接运算，编译器负责换算：

```cpp
auto total = 1h + 30min + 45s;    // 合法，自动统一单位
```

实测输出：

```
  500ms = 500 ms
  1h 换算成分钟 = 60
```

详见 [07-05 chrono](../part7_stdlib/07-05_chrono_random.md)。

### 其他标准后缀

- `1.0i` —— 复数 `[C++14]`，`std::complex_literals`
- `"abc"sv` —— `string_view` `[C++17]`，`std::string_view_literals`

这些后缀都在各自的 inline 命名空间里，需要 `using namespace std::xxx_literals;`。
标准规定**不带下划线的后缀保留给标准库**，用户自定义的必须以下划线开头。

## 四、用户自定义字面量 `[C++11]`

### 解决什么问题

数值在代码里经常是"带单位"的，但类型系统看不到单位：

```cpp
void set_timeout(int ms);
set_timeout(5);        // 5 毫秒还是 5 秒？
set_buffer(4096);      // 4096 字节？4096 KB？
```

单位混用是真实事故的常见原因（火星气候探测者号就毁于磅力秒与牛顿秒混用）。
用户自定义字面量让单位出现在字面量上，并且在编译期完成换算。

### 三种形式

```cpp
// 浮点字面量：参数必须是 long double
constexpr long double operator""_km(long double v) { return v * 1000.0L; }
constexpr long double operator""_m(long double v)  { return v; }
constexpr long double operator""_cm(long double v) { return v / 100.0L; }

// 整型字面量：参数必须是 unsigned long long
constexpr unsigned long long operator""_KB(unsigned long long v) { return v * 1024; }
constexpr unsigned long long operator""_MB(unsigned long long v) { return v * 1024 * 1024; }

// 字符串字面量：参数是 (const char*, size_t)
std::string operator""_upper(const char* s, std::size_t len) { /* ... */ }
```

用起来：

```cpp
constexpr long double distance = 1.5_km + 300.0_m + 50.0_cm;   // 1800.5 米
constexpr auto buffer_size = 4_KB;                              // 4096
"hello"_upper;                                                  // "HELLO"
```

实测输出：

```
  1.5_km + 300.0_m + 50.0_cm = 1800.5 米
  4_KB = 4096 字节, 16_MB = 16777216 字节
  "hello"_upper = HELLO
```

### 配合强类型，彻底消除单位错误

上面的 `_km` 返回 `long double`，单位信息在换算后就丢了。
更彻底的做法是返回一个专门的类型：

```cpp
class Temperature {
public:
    constexpr explicit Temperature(double celsius) : _celsius(celsius) {}
    constexpr double celsius() const { return _celsius; }
    constexpr double fahrenheit() const { return _celsius * 9.0 / 5.0 + 32.0; }
private:
    double _celsius;
};

constexpr Temperature operator""_degC(long double v) {
    return Temperature(static_cast<double>(v));
}

constexpr auto body_temp = 36.5_degC;
body_temp.fahrenheit();     // 97.7
```

这样 `Temperature` 和 `double` 不能互相赋值，单位混用在编译期就被拦住。
`std::chrono` 就是这个思路的标准库版本。

因为全部是 `constexpr`，这些换算在编译期完成，运行时零开销：

```cpp
static_assert(4_KB == 4096, "");
static_assert(body_temp.celsius() == 36.5, "");
```

### 约束

- 后缀**必须以下划线开头**（不带下划线的保留给标准库）
- 参数类型只能是上面列出的几种固定形式
- 原始形式 `operator""_x(const char*)` 可以拿到未解析的字面量文本，
  用于实现任意精度整数等特殊需求

## 五、属性（attributes）

C++11 引入了统一的属性语法 `[[...]]`，取代各编译器五花八门的扩展
（GCC 的 `__attribute__((...))`、MSVC 的 `__declspec(...)`）。
属性是给编译器的额外信息，通常用于**启用更多检查**。

### `[[noreturn]]` `[C++11]`

声明函数不会返回：

```cpp
[[noreturn]] void fail(const std::string& msg) {
    log(msg);
    std::exit(1);     // 必须真的不返回，否则是未定义行为
}
```

好处：编译器知道调用它之后的代码不可达，于是不会报"函数缺少返回值"
这类误报，也能做更好的优化。`std::terminate`、`std::exit`、`std::abort`
都标了这个属性。

### `[[nodiscard]]` `[C++17]`

返回值被忽略时产生警告。这个属性价值很高 ——
它把"必须检查返回值"从注释变成了编译器检查：

```cpp
[[nodiscard]] int must_check() { return 42; }

int value = must_check();     // ok
must_check();                 // 警告
```

实测警告（我单独验证过）：

```
warning: ignoring return value of 'int f()', declared with attribute 'nodiscard' [-Wunused-result]
```

也可以标在**类型**上，这样所有返回该类型的函数都受保护：

```cpp
struct [[nodiscard]] ErrorCode { int value; };    // 注意属性在 struct 之后
ErrorCode do_work();

do_work();      // 警告：ignoring returned value of type 'ErrorCode'
```

注意位置：属性必须**跟在 `struct`/`class` 关键字之后**。
写成 `[[nodiscard]] struct E {...}` 会被静默忽略 ——
GCC 给出的提示是 `attribute for 'struct E' must follow the 'struct' keyword`。
这个错误很隐蔽：代码能编过，属性却不生效。

该标的地方：

- 返回错误码的函数（忽略错误码是经典 bug，见 [01-08](01-08_exception.md)）
- 纯查询函数，如 `empty()`、`size()`
  —— 调用它却不看结果，说明作者想调的是 `clear()`
- 工厂函数、`async` 返回的 future

标准库从 C++20 起给大量函数加上了它，包括 `std::vector::empty`。

### `[[deprecated]]` `[C++14]`

```cpp
[[deprecated("请改用 new_api()")]] void old_api();

old_api();     // warning: 'void old_api()' is deprecated: 请改用 new_api()
```

比注释里写"已废弃"有效得多 —— 使用者在编译时就会看到，
而且带上了替代方案的提示。库演进时的标准做法。

### `[[maybe_unused]]` `[C++17]`

抑制"未使用"警告，用于**确实可能不用**的实体：

```cpp
void f([[maybe_unused]] int debug_only_param) {
    assert(debug_only_param > 0);     // Release 下 assert 消失，参数就没用了
}
```

比 `(void)param;` 这个传统技巧更直白。常见场合：
只在 `assert` 里用到的参数、只在某些 `#ifdef` 分支里用到的变量、
结构化绑定里不关心的那部分。

### `[[fallthrough]]` `[C++17]`

`switch` 里故意不写 `break` 时，显式说明这是有意的：

```cpp
switch (x) {
    case 1:
        result += 1;
        [[fallthrough]];     // 明确声明"故意落穿"
    case 2:
        result += 10;
        break;
}
```

实测：`fallthrough_demo(1)` 返回 11（1 + 10），`fallthrough_demo(2)` 返回 10。

意义在于区分"忘写 break"和"故意不写 break"。
开了 `-Wimplicit-fallthrough` 后，前者会被警告，后者因为有这个标记而不会。
忘写 break 是 C 语言最经典的 bug 之一，这个属性让编译器能帮上忙。

## 六、小结

| 特性 | 标准 | 解决的问题 |
|---|---|---|
| `R"(...)"` | C++11 | 路径和正则的双层转义 |
| `0b1010` | C++11 | 位掩码只能写十六进制 |
| `1'000'000` | C++14 | 长数字难以看清位数 |
| `"abc"s` | C++14 | 字面量不是 `string`，不能直接相加 |
| `500ms` / `1h` | C++14 | 时间单位不在类型里，传错不报错 |
| `operator""_x` | C++11 | 自定义单位的编译期换算与强类型 |
| `[[noreturn]]` | C++11 | 编译器不知道函数不返回 |
| `[[nodiscard]]` | C++17 | 返回值被静默忽略 |
| `[[deprecated]]` | C++14 | 废弃信息只在注释里 |
| `[[maybe_unused]]` | C++17 | 条件编译导致的未使用警告 |
| `[[fallthrough]]` | C++17 | 无法区分故意落穿和忘写 break |

实践建议：

- 正则和 Windows 路径一律用原始字符串
- 位掩码用 `0b` + 四位一组的分隔符
- 时间参数用 chrono 字面量，不要用裸 `int`
- 返回错误码或纯查询的函数标 `[[nodiscard]]`
- 自定义字面量后缀必须以 `_` 开头
- 属性标在类型上时，位置是 `struct [[attr]] Name`，不是 `[[attr]] struct Name`

> 下一章：[01-10 类型推导：auto 与 decltype](01-10_auto_decltype.md)
