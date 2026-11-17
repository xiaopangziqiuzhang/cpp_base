# 07-03 迭代器与算法

> 标准：C++98；并行算法、`clamp`/`gcd`/`reduce`/`sample` `[C++17]`
> demo：[`07_03_algorithms.cpp`](../../demos/part7_stdlib/07_03_algorithms.cpp)

## 一、迭代器：算法与容器的粘合剂

STL 的核心设计是**算法与容器分离**：算法不知道容器的内部结构，
只通过迭代器访问元素。这样 M 个算法 + N 个容器只需要 M + N 份代码，
而不是 M × N 份。

代价是算法无法改变容器大小（只有迭代器，没有容器），
这导致了 erase-remove 惯用法（见第三节）。

### 五种迭代器分类

能力递增：

| 分类 | 能力 | 典型容器 |
|---|---|---|
| **输入**（input）| 单向、只读、只能遍历一次 | `istream_iterator` |
| **输出**（output）| 单向、只写、只能遍历一次 | `ostream_iterator`、`back_inserter` |
| **前向**（forward）| 单向、可多次读写 | `forward_list` |
| **双向**（bidirectional）| 可 `--` | `list`、`set`、`map` |
| **随机访问**（random access）| 可 `+n`、可比较大小 | `vector`、`deque`、裸指针 |

实测各容器的迭代器类别：

```
        vector       : 随机访问
        deque        : 随机访问
        list         : 双向
        forward_list : 前向
        set          : 双向
        int*（裸指针）: 随机访问
```

**算法要求的最低类别决定了它能用于哪些容器。** 例如：

```cpp
std::sort(v.begin(), v.end());        // 要求随机访问
// std::sort(l.begin(), l.end());     // 编译错误：list 的迭代器是双向的
l.sort();                              // list 有自己的成员函数版本
```

`std::sort` 用的是快排/堆排的混合（introsort），需要随机访问来做分区。
`list::sort` 用归并排序，不需要随机访问，但要移动节点指针。

裸指针是随机访问迭代器 —— 这就是 STL 算法能直接用于数组的原因
（靠 `iterator_traits` 的指针偏特化，见
[05-03](../part5_template/05-03_specialization.md)）。

## 二、常用算法

### 非修改式查询

```cpp
std::count_if(v.begin(), v.end(), pred);         // 满足条件的个数
std::find(v.begin(), v.end(), value);             // 查找，返回迭代器
std::find_if(v.begin(), v.end(), pred);
std::all_of / any_of / none_of(first, last, pred);
std::max_element / min_element(first, last);
std::minmax_element(first, last);                 // 一次遍历得到两者
std::equal(first1, last1, first2);
std::mismatch(first1, last1, first2);             // 第一个不同的位置
```

实测：

```
        count_if(偶数) = 2
        find(8) 位置 = 2
        all_of(都>0) = 1
        max_element = 9, min_element = 1
        minmax_element = (1, 9)
```

### 修改式操作

```cpp
std::transform(src_first, src_last, dst_first, func);    // map
std::copy / copy_if / copy_n(first, last, dst);
std::fill(first, last, value);
std::iota(first, last, start_value);                      // 填充递增序列
std::generate(first, last, gen);                          // 用函数生成
std::replace(first, last, old, new);
std::reverse / rotate(first, last);
std::shuffle(first, last, rng);                           // C++11，取代 random_shuffle
```

实测：

```
        transform(x*2): 10 4 16 2 18 6 14
        iota(从10递增): 10 11 12 13 14
```

### 排序与选择

```cpp
std::sort(first, last);                       // O(n log n)，不稳定
std::stable_sort(first, last);                 // 稳定，但需要额外内存
std::partial_sort(first, middle, last);        // 只排前 k 个
std::nth_element(first, nth, last);            // O(n) 找第 k 小
std::partition / stable_partition(first, last, pred);
std::is_sorted(first, last);
```

实测：

```
        sort: 1 2 3 5 7 8 9
        partial_sort(前3小): 1 2 3 8 9 5 7 （只保证前 3 个有序，比全排序快）
        nth_element(第4小) = 5（O(n) 找第 k 小）
```

**`partial_sort` 和 `nth_element` 经常被忽略，但很有用**：

- "取 Top 10" → `partial_sort`，不需要排全部
- "找中位数" → `nth_element`，O(n) 而不是 O(n log n)

### 数值算法（`<numeric>`）

```cpp
std::accumulate(first, last, init);                       // 求和
std::accumulate(first, last, init, binary_op);             // 任意二元操作
std::inner_product(first1, last1, first2, init);           // 点积
std::partial_sum(first, last, dst);                        // 前缀和
std::adjacent_difference(first, last, dst);                // 相邻差
std::reduce(first, last);                                  // [C++17] 并行版 accumulate
std::transform_reduce(...)                                 // [C++17] map + reduce
```

实测：

```
        accumulate(求和) = 35
        accumulate(求积) = 15120
        inner_product = 233
```

### 二分查找（要求已排序）

```cpp
std::binary_search(first, last, value);       // 只返回 bool
std::lower_bound(first, last, value);          // 第一个 >= value 的位置
std::upper_bound(first, last, value);          // 第一个 > value 的位置
std::equal_range(first, last, value);          // 两者的 pair
```

实测 `lower_bound(5)` 返回位置 3。

注意这些算法**要求区间已排序**，否则是未定义行为（不会报错，只会给错结果）。

## 三、erase-remove 惯用法

算法不能改变容器大小，所以"删除元素"要分两步。

```cpp
std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};
auto new_end = std::remove_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
```

实测 `remove_if` 之后的状态：

```
      remove_if 之后（size 没变）: 1 3 5 7 5 6 7 8  size=8
      新的逻辑末尾在位置 4
```

`remove_if` 把要保留的元素**移到前面**，返回新的逻辑末尾。
后面的元素处于"未指定状态"（这里恰好是原值的残留，但不要依赖）。

第二步真正删除：

```cpp
v.erase(new_end, v.end());
```

```
      erase 之后: 1 3 5 7  size=4
```

合起来写成一行：

```cpp
v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
```

### unique 也是同样的模式

```cpp
std::vector<int> dup{1, 1, 2, 2, 2, 3, 3};
dup.erase(std::unique(dup.begin(), dup.end()), dup.end());    // 1 2 3
```

**注意 `unique` 只去除相邻的重复元素**，所以通常要先 `sort`：

```cpp
std::sort(v.begin(), v.end());
v.erase(std::unique(v.begin(), v.end()), v.end());
```

### C++20 的简化

```cpp
std::erase_if(v, pred);        // C++20，一步完成
std::erase(v, value);
```

## 四、迭代器适配器

### back_inserter / inserter

把算法的"写入"变成容器的插入操作：

```cpp
std::vector<int> dst;                                      // 空容器
std::copy(src.begin(), src.end(), std::back_inserter(dst)); // 自动 push_back

std::set<int> s;
std::copy(src.begin(), src.end(), std::inserter(s, s.begin()));
```

不用适配器就要先保证目标有足够空间：

```cpp
std::vector<int> dst2(src.size());                  // 必须先 resize
std::copy(src.begin(), src.end(), dst2.begin());
```

### reverse_iterator

```cpp
for (auto it = v.rbegin(); it != v.rend(); ++it) { /* 反向遍历 */ }
```

实测输出 `3 2 1`。

### ostream_iterator / istream_iterator

```cpp
std::copy(v.begin(), v.end(), std::ostream_iterator<int>(std::cout, " "));

// 从输入流读取所有 int
std::vector<int> nums(std::istream_iterator<int>(std::cin),
                      std::istream_iterator<int>());
```

（第二行要注意 most vexing parse，见
[01-11](../part1_basics/01-11_uniform_init.md)。）

### move_iterator `[C++11]`

```cpp
std::vector<std::string> src{"a", "b", "c"};
std::vector<std::string> dst(std::make_move_iterator(src.begin()),
                             std::make_move_iterator(src.end()));
// src 的元素被移动而非拷贝
```

## 五、并行算法 `[C++17]`

给算法加一个**执行策略**参数：

```cpp
#include <execution>

std::sort(std::execution::par, v.begin(), v.end());
std::transform(std::execution::par, src.begin(), src.end(), dst.begin(), f);
```

四种策略：

| 策略 | 含义 |
|---|---|
| `seq` | 串行，和不带策略一样 |
| `par` | 多线程并行 |
| `par_unseq` | 多线程 + 向量化（SIMD）|
| `unseq` | 仅向量化 `[C++20]` |

### 重要的实测发现：可能静默退化为串行

我在本机实测 1000 万个 double 的排序：

```
      10000000 个 double 排序:
        串行: 4224 ms
        并行: 4233 ms            <- 完全没有加速！
```

原因是 **libstdc++ 的并行算法依赖 Intel TBB**，而本机没有安装：

```bash
$ ldconfig -p | grep -i tbb
（空 = 未安装）

$ g++ -std=c++17 par.cpp -ltbb -o par
/usr/bin/ld: cannot find -ltbb: No such file or directory
```

**没有 TBB 时的行为是静默退化为串行** ——
不报错、不警告，代码照样编译运行，只是完全没有并行。
这是个很容易踩的坑：你以为用上了并行，实际上什么也没发生。

为了确认不是机器本身不能并行，我加了个对照组 ——
手工用两个线程做归并排序：

```
        手工 2 线程归并: 1996 ms（对照组，证明机器能并行加速）
        结果与串行一致: 1
```

**手工并行快 2.1 倍**，证明问题在 TBB 缺失，不在硬件。

### 正确的使用方式

```bash
# Ubuntu/Debian
apt install libtbb-dev
g++ -std=c++17 -O2 main.cpp -ltbb
```

各实现的情况不同：

| 实现 | 并行算法支持 |
|---|---|
| **libstdc++**（GCC） | 需要 TBB，否则静默串行 |
| **libc++**（Clang） | 较长时间内未实现（直接报错或退化）|
| **MSVC** | 原生支持，无额外依赖 |

所以**跨平台项目不要依赖并行算法的实际并行性**。
需要确定的并行，用 `std::thread`、线程池（见
[Part 8](../part8_concurrency/)）或 OpenMP。

### 并行算法的使用约束

即使 TBB 可用，也有几条限制：

```cpp
// 1. 传入的函数不能抛异常（抛了就 terminate，不是传播）
std::for_each(std::execution::par, v.begin(), v.end(),
              [](int& x) { x = may_throw(x); });      // 危险

// 2. 不能有数据竞争
int sum = 0;
std::for_each(std::execution::par, v.begin(), v.end(),
              [&sum](int x) { sum += x; });            // 数据竞争！
// 正确做法：用 reduce
auto sum2 = std::reduce(std::execution::par, v.begin(), v.end());

// 3. 元素数量要足够大，否则线程开销超过收益
```

## 六、C++17 新增的小工具

```cpp
std::clamp(15, 1, 10);        // 10，把值限制在区间内
std::clamp(-5, 1, 10);        // 1

std::gcd(12, 18);              // 6，最大公约数
std::lcm(4, 6);                // 12，最小公倍数

std::reduce(v.begin(), v.end());                      // accumulate 的并行版
std::transform_reduce(v.begin(), v.end(), 0,
                      std::plus<>{}, [](int x) { return x * x; });   // 平方和

std::sample(pool.begin(), pool.end(), out, 5, rng);   // 随机抽 5 个
```

实测：

```
      std::clamp(15, 1, 10) = 10
      std::gcd(12, 18) = 6
      std::reduce = 15
      std::transform_reduce = 55（平方和）
      std::sample（随机抽样）: 6 7 16 17 19
```

### reduce 和 accumulate 的区别

```cpp
std::accumulate(first, last, init);     // 保证从左到右依次累加
std::reduce(first, last, init);          // 不保证顺序，可以并行
```

**对浮点数，两者结果可能不同** —— 浮点加法不满足结合律：

```cpp
(0.1 + 0.2) + 0.3  !=  0.1 + (0.2 + 0.3)     // 可能不等
```

需要确定结果时用 `accumulate`，需要并行时用 `reduce`。

`std::clamp` 有个坑：**参数是引用，返回也是引用**：

```cpp
const int& r = std::clamp(get_value(), 1, 10);     // 可能悬垂！
int v = std::clamp(get_value(), 1, 10);             // 安全
```

## 七、什么时候该用算法而不是手写循环

**该用算法：**

- 操作有标准名字（`sort`、`find`、`transform`）——
  读代码的人一眼知道意图
- 算法的实现比你手写的更优（`sort` 是 introsort、
  `copy` 对 trivially copyable 类型用 `memmove`）
- 需要并行（加执行策略就行）

**手写循环：**

- 逻辑复杂，用算法要套好几层 lambda
- 需要同时操作多个容器且索引关系复杂
- 需要提前退出且退出条件复杂
- 范围 for 更清楚时（遍历 + 简单操作）

```cpp
// 用算法更好
auto it = std::find_if(v.begin(), v.end(), pred);

// 范围 for 更好
for (const auto& x : v) { process(x); }

// 手写循环更好（多容器 + 复杂索引）
for (std::size_t i = 0; i < a.size(); ++i) {
    if (i > 0 && a[i] > a[i-1]) b[i] = c[i-1] + d[i];
}
```

C++20 的 ranges 让算法的组合变得容易得多（`v | filter | transform`），
在 C++17 下算法的链式组合仍然笨重。

## 八、小结

| 要点 | 说明 |
|---|---|
| STL 的核心设计 | 算法与容器分离，通过迭代器粘合；M+N 而非 M×N |
| 代价 | 算法无法改变容器大小 → erase-remove 惯用法 |
| 五种迭代器 | 输入/输出 < 前向 < 双向 < 随机访问 |
| 实测类别 | `vector`/`deque`/裸指针随机访问；`list`/`set` 双向；`forward_list` 前向 |
| 被忽略的好算法 | `partial_sort`（Top K）、`nth_element`（O(n) 第 k 小）、`minmax_element` |
| erase-remove | `v.erase(std::remove_if(...), v.end())`；`unique` 只去相邻重复 |
| 迭代器适配器 | `back_inserter`、`inserter`、`ostream_iterator`、`move_iterator` |
| 并行算法 `[C++17]` | 加 `std::execution::par` 参数 |
| **实测的坑** | **libstdc++ 没有 TBB 时静默退化为串行**（实测无加速，对照组手工并行快 2.1 倍）|
| 并行约束 | 不能抛异常、不能有数据竞争、元素数要足够大 |
| `reduce` vs `accumulate` | 前者可并行但不保证顺序，浮点结果可能不同 |
| 算法 vs 手写循环 | 有标准名字的操作用算法；复杂索引关系手写 |

> 下一章：[07-04 tuple 与和类型](07-04_tuple_variant.md)
