# 07-05 chrono 与 random

> 标准：`[C++11]`；字面量后缀 `[C++14]`
> demo：[`07_05_chrono_random.cpp`](../../demos/part7_stdlib/07_05_chrono_random.cpp)

两个库，同一个设计思路：**把隐含的约定搬进类型系统**。

## 一、C 时间 API 的问题

```c
clock();              /* 单位是 CLOCKS_PER_SEC，平台相关 */
time(nullptr);         /* 只有秒级精度 */
gettimeofday(&tv, 0);  /* 微秒，但 POSIX 专有 */
```

实测本机 `CLOCKS_PER_SEC = 1000000`，但在其他平台可能是 1000 或 100。

核心问题是 **时间单位不在类型里**：

```cpp
void sleep_for(int duration);      // 毫秒？秒？微秒？
sleep_for(1000);                    // 1 秒还是 1 毫秒？
```

传错单位不会报错，只是行为不对 —— 而且往往是"慢了 1000 倍"这种
需要调试才能发现的问题。

## 二、chrono 的三个核心概念

### 概念一：duration（时长）

表示"一段时间"，**单位编码在类型里**：

```cpp
std::chrono::milliseconds ms(500);
std::chrono::seconds s(2);
auto total = ms + s;              // 自动统一到毫秒
total.count();                     // 2500
```

实测 `500ms + 2s = 2500 ms` —— 不同单位可以直接相加，编译器负责换算。

底层是模板：

```cpp
template <class Rep, class Period = std::ratio<1>>
class duration;

using seconds      = duration<long long, ratio<1>>;
using milliseconds = duration<long long, ratio<1, 1000>>;
using microseconds = duration<long long, ratio<1, 1000000>>;
```

`Rep` 是存储类型，`Period` 是单位（用 `std::ratio` 表示的分数）。
所以可以自定义任意单位：

```cpp
using third_second = std::chrono::duration<double, std::ratio<1, 3>>;   // 1/3 秒
using frames = std::chrono::duration<int, std::ratio<1, 60>>;            // 1/60 秒（帧）
```

### 概念二：time_point（时间点）

表示"某个时刻"：

```cpp
auto now = std::chrono::steady_clock::now();
auto later = now + std::chrono::seconds(1);
auto diff = later - now;                      // time_point 相减得到 duration
```

实测 `diff` 是 1000 ms。

规则：

- `time_point - time_point` → `duration`
- `time_point + duration` → `time_point`
- `time_point + time_point` → **非法**（两个时刻相加没有意义）

这种代数关系也是类型系统强制的。

### 概念三：clock（时钟）

提供 `now()` 和时间点的类型：

| 时钟 | 特点 | 用途 |
|---|---|---|
| `system_clock` | 墙上时钟，**可被调整**，能转 `time_t` | 日历时间、时间戳 |
| `steady_clock` | **单调递增**，不会回退 | **计时专用** |
| `high_resolution_clock` | 精度最高，通常是上面之一的别名 | 不推荐直接用 |

实测：

```
        steady_clock 是单调的吗: 1
        system_clock 是单调的吗: 0
```

## 三、单位换算：无损隐式，有损显式

```cpp
std::chrono::milliseconds from_sec = 2s;           // 秒 -> 毫秒，无损，隐式转换
// std::chrono::seconds from_ms = 1500ms;          // 编译错误！有损
auto to_sec = std::chrono::duration_cast<std::chrono::seconds>(1500ms);  // 显式，截断
```

实测：

```
        2s 隐式转毫秒 = 2000 ms
        1500ms 显式转秒 = 1 s（截断了 500ms）
```

**这个设计防止了意外的精度丢失** ——
想丢精度必须显式写 `duration_cast`，等于签字确认。

### 整数除法会截断

```cpp
auto d = 1h + 30min + 45s;
duration_cast<minutes>(d).count();        // 90，不是 90.75！
```

实测：

```
        = 5445 秒
        = 90 分（整数除法会截断）
        = 90.75 分（浮点，精确）
```

需要精确值要用浮点 `Rep`：

```cpp
using fmin = std::chrono::duration<double, std::ratio<60>>;
duration_cast<fmin>(d).count();           // 90.75
```

## 四、计时的正确做法

```cpp
auto t1 = std::chrono::steady_clock::now();
do_work();
auto t2 = std::chrono::steady_clock::now();

auto us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
auto ms = std::chrono::duration<double, std::milli>(t2 - t1).count();   // 浮点毫秒
```

实测：

```
      耗时: 2288 us = 2.28835 ms
```

### 为什么必须用 steady_clock

`system_clock` 可能被 **NTP 校正**、被用户手工修改、有**夏令时跳变**。
如果在计时期间发生了调整，差值就是错的 —— 甚至可能是**负数**。

`steady_clock` 保证单调递增，只能用于测量时间间隔
（它的时间点没有日历含义，不能转 `time_t`）。

**规则：计时用 `steady_clock`，显示时间用 `system_clock`。**

### 一个可复用的计时器

```cpp
class Timer {
public:
    Timer() : _start(std::chrono::steady_clock::now()) {}
    double elapsed_ms() const {
        return std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - _start).count();
    }
    void reset() { _start = std::chrono::steady_clock::now(); }
private:
    std::chrono::steady_clock::time_point _start;
};
```

实测测量 `sleep_for(10ms)` 得到 10.0585 ms —— 误差来自调度延迟。

（本书所有 demo 里的性能测量都用这个模式。）

## 五、日历时间

只有 `system_clock` 能和 `time_t` 互转：

```cpp
auto now = std::chrono::system_clock::now();
std::time_t t = std::chrono::system_clock::to_time_t(now);
std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");

// Unix 时间戳
auto since_epoch = now.time_since_epoch();
std::chrono::duration_cast<std::chrono::seconds>(since_epoch).count();
std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch).count();
```

实测：

```
      当前时间: 2026-11-14 01:51:34
      Unix 时间戳(秒): 1794592294
      Unix 时间戳(毫秒): 1794592294537
```

C++17 的日历支持很弱 —— 格式化要靠 C 的 `strftime`/`localtime`
（而且 `localtime` 不是线程安全的）。
**C++20 才有完整的日历和时区**：

```cpp
// C++20
using namespace std::chrono;
auto today = year_month_day{floor<days>(system_clock::now())};
zoned_time zt{"Asia/Shanghai", system_clock::now()};
std::format("{:%Y-%m-%d}", today);
```

C++17 下处理日历，实践上还是用第三方库（date.h、Boost.DateTime）或 C API。

## 六、为什么不用 rand()

`rand()` 有五个问题：

1. **质量差** —— 低位的随机性特别糟。老教材教你用
   `rand() % 2` 判奇偶，而很多实现的最低位几乎是交替的
2. **`RAND_MAX` 可能很小** —— 标准只保证 ≥ 32767。
   实测本机是 2147483647，但不能依赖
3. **`rand() % n` 有取模偏差**
4. **全局状态，不是线程安全的**
5. **不可重现** —— 同样的种子在不同实现上给出不同序列

### 取模偏差

用 `RAND_MAX = 32767` 举例最清楚：

```
要生成 [0, 10000)，rand() 的值域是 [0, 32767]，共 32768 个值
32768 = 3 × 10000 + 2768

所以 0~2767 各有 4 个原值映射到它
     2768~9999 各只有 3 个
于是小的数出现概率高约 33%
```

实测本机的情况：

```
      本机 RAND_MAX = 2147483647，偏差很小，但原理相同且无法消除
      本机取模 3 的理论偏差: 有偏差
```

`RAND_MAX` 大时偏差小到可以忽略，但**原理上无法消除** ——
只要值域不能被 n 整除就有偏差。
而标准库的 `uniform_int_distribution` 用拒绝采样消除了它。

## 七、C++11 的设计：引擎 + 分布

核心思路：**把"产生随机位"和"映射到分布"分开**。

### 引擎：产生均匀分布的随机位

```cpp
std::mt19937 gen(42);            // 固定种子，可重现
gen();                            // 返回一个 uint32_t
```

实测 `mt19937(42)` 的前三个值：

```
      引擎 mt19937(42) 前三个值: 1608637542 3421126067 4083286876
```

**同样的种子在任何平台给出同样的序列** —— 这是标准保证的，
所以测试可以重现。

常见引擎：

| 引擎 | 特点 |
|---|---|
| `mt19937` | 梅森旋转，质量好，状态 2.5KB，**默认选它** |
| `mt19937_64` | 64 位版本 |
| `minstd_rand` | 线性同余，快但质量一般 |
| `ranlux48` | 质量最好但最慢 |
| `default_random_engine` | **实现定义**，不保证跨平台一致，不推荐 |

### 种子：random_device

```cpp
std::random_device rd;
std::mt19937 gen(rd());          // 用真随机数做种子
```

`random_device` 尽可能提供真随机（Linux 上通常读 `/dev/urandom`）。
实测 `rd.entropy() = 32`。

它可能很慢，所以**只用它做种子，不要用它产生大量随机数**。

### 分布：映射到想要的分布

```cpp
std::uniform_int_distribution<int> dice(1, 6);         // 均匀整数
std::uniform_real_distribution<double> unit(0.0, 1.0);  // 均匀实数
std::normal_distribution<double> normal(100.0, 15.0);   // 正态（均值，标准差）
std::bernoulli_distribution coin(0.5);                   // 伯努利（抛硬币）
std::poisson_distribution<int> poisson(4.0);             // 泊松
std::exponential_distribution<double> exp(1.0);          // 指数
std::discrete_distribution<int> weighted({10, 30, 60});  // 离散加权
```

实测：

```
      均匀整数分布 [1,6] 掷骰子 10 次: 3 5 6 2 5 5 4 4 1 3
      均匀实数分布 [0,1): 0.1000 0.4592 0.3337
      正态分布 N(100,15): 105.8887 86.0622 101.1975
      伯努利分布（抛硬币）: THTTTHHTHH
```

### 验证分布的正确性

掷 60 万次骰子：

```
        1: 100293 (16.72%)
        2: 99684 (16.61%)
        3: 99972 (16.66%)
        4: 100258 (16.71%)
        5: 100139 (16.69%)
        6: 99654 (16.61%)
```

各面都接近理论值 16.67%，**没有取模偏差**。

## 八、实用模式

### 模式一：thread_local 引擎

引擎有状态（`mt19937` 的状态是 2.5KB），所以：

- 不能多线程共享（数据竞争）
- 不应每次调用都构造（开销大，而且会重复序列）

```cpp
std::mt19937& thread_local_engine() {
    static thread_local std::mt19937 gen(std::random_device{}());
    return gen;
}

int random_int(int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(thread_local_engine());
}
```

`thread_local` 让每个线程有自己的引擎 ——
既线程安全，又避免了重复构造。见
[08-01](../part8_concurrency/08-01_thread.md)。

### 模式二：测试用固定种子，生产用 random_device

```cpp
#ifdef TESTING
    std::mt19937 gen(42);                      // 可重现
#else
    std::mt19937 gen(std::random_device{}());  // 真随机
#endif
```

### 模式三：分布对象可以复用

```cpp
// 不好：循环里反复构造
for (int i = 0; i < n; ++i) {
    std::uniform_int_distribution<int> dist(1, 6);     // 每次都构造
    use(dist(gen));
}

// 好：构造一次
std::uniform_int_distribution<int> dist(1, 6);
for (int i = 0; i < n; ++i) use(dist(gen));
```

分布对象可能有内部状态（比如 `normal_distribution`
用 Box-Muller 一次生成两个值，缓存一个），反复构造会浪费。

### 模式四：洗牌用 shuffle 而不是手写

```cpp
std::shuffle(v.begin(), v.end(), gen);        // C++11
// std::random_shuffle(v.begin(), v.end());   // C++14 废弃，C++17 移除
```

手写洗牌极易写错（经典错误是每次都从整个范围取随机位置，
导致分布不均）。

## 九、小结

### chrono

| 概念 | 要点 |
|---|---|
| 解决什么 | C 的时间 API 单位不在类型里，传错不报错 |
| `duration` | 时长；单位编码在类型里；不同单位可直接相加 |
| `time_point` | 时刻；相减得 duration；不能相加 |
| 单位换算 | **无损隐式，有损要 `duration_cast`** |
| 整数截断 | `duration_cast<minutes>(90.75min)` 得 90；要精确用浮点 `Rep` |
| `steady_clock` | **计时专用**，单调递增 |
| `system_clock` | 日历时间，可被调整，能转 `time_t` |
| C++17 的不足 | 日历和时区支持弱，要靠 C API 或第三方库（C++20 补全）|

### random

| 概念 | 要点 |
|---|---|
| `rand()` 的五个问题 | 质量差、`RAND_MAX` 可能小、**取模偏差**、非线程安全、不可重现 |
| 设计 | **引擎**（产生随机位）+ **分布**（映射到分布）分离 |
| 引擎选择 | 默认 `mt19937`；避免 `default_random_engine`（实现定义）|
| 种子 | `random_device` 做种子；它本身慢，不用来产生大量数 |
| 可重现性 | 固定种子在任何平台给出相同序列（标准保证）|
| 实用模式 | `thread_local` 引擎、复用分布对象、`std::shuffle` |

> 下一章：[07-06 正则与文件系统](07-06_regex_filesystem.md)
