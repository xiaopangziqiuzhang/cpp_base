# 07-02 容器全景

> 标准：`array`、`forward_list`、`unordered_*`、emplace 系列 `[C++11]`；
> `try_emplace`、`insert_or_assign` `[C++17]`
> demo：[`07_02_containers.cpp`](../../demos/part7_stdlib/07_02_containers.cpp)

## 一、容器全景表

| 容器 | 底层结构 | 随机访问 | 插入/删除 | 标准 |
|---|---|---|---|---|
| `array` | 固定大小数组（栈上）| O(1) | 不支持 | `[C++11]` |
| `vector` | 动态数组 | O(1) | 尾部 O(1) 均摊，中间 O(n) | C++98 |
| `deque` | 分段数组 | O(1) | 两端 O(1)，中间 O(n) | C++98 |
| `list` | 双向链表 | O(n) | 任意位置 O(1)（已有迭代器）| C++98 |
| `forward_list` | 单向链表 | O(n) | 同上 | `[C++11]` |
| `set` / `map` | 红黑树 | — | O(log n) | C++98 |
| `multiset` / `multimap` | 红黑树 | — | O(log n) | C++98 |
| `unordered_set` / `unordered_map` | 哈希表 | — | 平均 O(1)，最坏 O(n) | `[C++11]` |
| `stack` / `queue` / `priority_queue` | 适配器 | — | 取决于底层容器 | C++98 |

## 二、std::array：替代内置数组 `[C++11]`

内置数组有两个硬伤：**不知道自己的大小**、**作为参数会退化成指针**。

```cpp
void c_array_param(int a[3]) { a[0] = 999; }     // 实际签名是 int*
int ca[3]{1, 2, 3};
c_array_param(ca);                                // ca[0] 变成 999！
```

实测确认：

```
      array 按值传递是真拷贝: a[0] = 1（没被改）
      内置数组"按值"其实是传指针: ca[0] = 999（被改了！）
```

这是 C 继承下来的规则，极容易误判。`std::array` 修正了它：

```cpp
std::array<int, 5> arr{1, 2, 3, 4, 5};

arr.size();              // 5，编译期常量
arr.at(10);              // 抛 out_of_range
arr.front(); arr.back();
for (int x : arr) {}      // 支持范围 for
a == b;                   // 支持比较运算符
```

而且是**零开销**的：

```
      sizeof(array<int,5>) = 20 == sizeof(int[5]) = 20（零开销）
```

它就是一个包了一层的内置数组，没有任何额外存储。

**规则：需要固定大小数组时一律用 `std::array`，不要用内置数组。**

（需要动态大小用 `vector`，需要 C 接口时用 `arr.data()`。）

## 三、forward_list：单向链表 `[C++11]`

比 `list` 省内存 —— 每个节点只有一个指针而不是两个。

```cpp
std::forward_list<int> fl{1, 2, 3};
fl.push_front(0);
auto it = fl.begin();
fl.insert_after(it, 99);       // 注意是 insert_after，不是 insert
```

它的接口有意做得"残缺"：

- **没有 `size()`** —— 要算就得 O(n) 遍历，所以干脆不提供（避免你误以为是 O(1)）
- **没有 `push_back`** —— 没有尾指针
- **只有 `insert_after`/`erase_after`** —— 单向链表无法回头找前驱

实测：

```
      forward_list: 0 1 2 3
      insert_after 之后: 0 99 1 2 3
```

实践中用得很少 —— 只在内存极度受限、且确实需要链表语义时才值得。
大多数场合 `vector` 比任何链表都快（缓存局部性）。

## 四、unordered_* ：哈希容器 `[C++11]`

C++98 只有基于红黑树的有序容器。C++11 补上了哈希表。

```cpp
std::map<std::string, int> ordered;              // 红黑树，有序，O(log n)
std::unordered_map<std::string, int> hashed;     // 哈希表，无序，平均 O(1)
```

实测遍历顺序：

```
      map（红黑树，有序）:      apple=1 banana=2 cherry=3
      unordered_map（哈希，无序）: cherry=3 apple=1 banana=2
```

### 性能实测

20 万个 `int` 键：

```
      200000 次插入: map 99639 us, unordered_map 34708 us
      200000 次查找: map 52250 us, unordered_map 9458 us
```

查找快 **5.5 倍**，插入快 **2.9 倍**。

### 怎么选

| 需求 | 选择 |
|---|---|
| 需要**有序遍历** | `map` / `set` |
| 需要范围查询（`lower_bound`）| `map` / `set` |
| 只需要按键查找 | **`unordered_map` / `unordered_set`** |
| 键的哈希函数难写或很慢 | `map` / `set` |
| 需要稳定的最坏情况性能 | `map` / `set`（哈希最坏退化到 O(n)）|

默认选 `unordered_*`，除非需要有序。

### 哈希表的内部参数

```cpp
hashed.bucket_count();        // 桶的数量
hashed.load_factor();          // 负载因子 = size / bucket_count
hashed.max_load_factor();      // 超过它就 rehash（默认 1.0）
hashed.reserve(n);             // 预留，避免反复 rehash
```

实测：

```
      哈希表内部信息: bucket_count=13 load_factor=0.230769 max_load_factor=1
```

**插入大量元素前要 `reserve`** —— rehash 要重新计算所有元素的桶位置，很贵。

### 自定义类型作键

`map` 需要 `operator<`（或自定义比较器），`unordered_map` 需要**哈希函数**：

```cpp
struct Point { int x, y; };

// map：提供比较
struct PointLess {
    bool operator()(const Point& a, const Point& b) const {
        return a.x != b.x ? a.x < b.x : a.y < b.y;
    }
};
std::map<Point, int, PointLess> m;

// unordered_map：提供哈希 + 相等比较
struct PointHash {
    std::size_t operator()(const Point& p) const {
        return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
    }
};
struct PointEq {
    bool operator()(const Point& a, const Point& b) const {
        return a.x == b.x && a.y == b.y;
    }
};
std::unordered_map<Point, int, PointHash, PointEq> um;
```

写哈希函数比写比较函数难得多（要保证分布均匀），这是 `map` 的一个优势。

## 五、emplace 系列

```cpp
v.push_back(Tracer("a"));      // 构造临时对象 -> 移动进容器
v.emplace_back("b");            // 直接在容器内构造
```

实测输出：

```
        push_back(Tracer("a")):
          构造 a
          移动                    <- 多一次移动
        emplace_back("b"):
          构造 b                  <- 就一步
```

原理是完美转发（见 [02-04](../part2_memory/02-04_perfect_forwarding.md)）：
参数被原样转发给元素的构造函数。

### 什么时候用哪个

```cpp
// 用参数构造新元素 -> emplace
v.emplace_back(arg1, arg2);

// 已有现成对象要放进去 -> push_back + move（一样好）
v.push_back(std::move(existing));

// 已有对象且还要继续用 -> push_back（必须拷贝）
v.push_back(existing);
```

注意 `emplace` 不总是更好：

```cpp
v.emplace_back(Tracer("a"));    // 和 push_back 完全一样，没有收益
```

### map 的特殊情况 `[C++17]`

```cpp
std::map<int, std::string> m;
m.emplace(1, "one");
m.try_emplace(2, "two");         // [C++17] 键已存在时不构造值
m.insert_or_assign(1, "ONE");    // [C++17] 存在就赋值
```

实测：

```
        1 => ONE
        2 => two
```

`try_emplace` 解决了 `emplace` 的一个实际问题：
`emplace` 即使插入失败（键已存在），**也可能已经构造了值对象**。
如果值的构造很贵（或者有副作用，比如移动了一个 `unique_ptr`），这就是浪费甚至 bug：

```cpp
std::map<int, std::unique_ptr<Widget>> m;
m.emplace(1, std::move(ptr));        // 失败时 ptr 可能已经被移走了！
m.try_emplace(1, std::move(ptr));    // 失败时保证不动 ptr
```

`insert_or_assign` 解决的是 `operator[]` 的问题 ——
`m[key] = value` 要求值类型可默认构造，而且会先构造一个默认值再赋值。

## 六、迭代器失效规则

这是容器最容易出错的地方。**每种容器的规则不同**。

### vector

```cpp
std::vector<int> v{1, 2, 3};      // capacity = 3
auto* old_data = v.data();
v.push_back(4);                    // 扩容！
```

实测：

```
        capacity=3
        push_back 后 capacity=6 数据地址已变（全部迭代器失效）
```

| 操作 | 失效范围 |
|---|---|
| 扩容（`push_back` 导致 size > capacity）| **全部**迭代器、引用、指针 |
| `insert` | 插入点及之后；若扩容则全部 |
| `erase` | 被删位置及之后 |
| `reserve` / `shrink_to_fit` | 全部 |
| `clear` | 全部 |

### deque

任何插入都**可能**使迭代器失效（因为可能分配新的段），
但**引用和指针**在两端插入时保持有效。

### list / forward_list

```cpp
std::list<int> l{1, 2, 3};
auto it = std::next(l.begin());
l.push_front(0);
l.push_back(4);
*it;                               // 仍然有效
```

实测仍指向 2。链表是节点式存储，插入不影响已有节点。

| 操作 | 失效范围 |
|---|---|
| `insert` / `push_*` | **不失效** |
| `erase` | 只有被删元素的迭代器失效 |

### map / set（红黑树）

```cpp
std::map<int, int> m{{1,1}, {2,2}};
auto mit = m.find(1);
m[3] = 3;
mit->first;                        // 仍然有效
```

实测仍指向 key=1。和链表一样是节点式存储。

### unordered_*（哈希表）

| 操作 | 失效范围 |
|---|---|
| rehash（负载因子超限）| **迭代器全部失效**，但**引用和指针保持有效** |
| `erase` | 只有被删元素 |

这个区别很微妙：rehash 重新组织桶结构，所以迭代器（要遍历桶）失效，
但元素节点本身没动，所以指向元素的引用和指针仍然有效。

## 七、边遍历边删除的正确写法

```cpp
// 错误：erase 之后 it 失效
for (auto it = v.begin(); it != v.end(); ++it)
    if (*it % 2 == 0) v.erase(it);        // 未定义行为
```

### 写法一：用 erase 的返回值

```cpp
for (auto it = v.begin(); it != v.end(); ) {
    if (*it % 2 == 0) it = v.erase(it);   // erase 返回下一个有效迭代器
    else ++it;
}
```

注意循环头里**没有 `++it`** —— 由分支决定是否前进。

关联容器从 C++11 起 `erase` 也返回下一个迭代器（C++98 返回 `void`）。

### 写法二：erase-remove 惯用法（vector/deque 推荐）

```cpp
v.erase(std::remove_if(v.begin(), v.end(),
                       [](int x) { return x % 2 == 0; }),
        v.end());
```

实测两种写法结果相同：

```
      手工循环 + erase 返回值: 1 3 5
      erase-remove 惯用法: 1 3 5
```

`std::remove_if` 不真正删除元素，而是把"要保留的"移到前面，
返回新的逻辑末尾。再用 `erase` 删掉后面的部分。

为什么要两步？因为算法只有迭代器，无法改变容器大小 ——
这是 STL"算法与容器分离"设计的代价。

**C++20 提供了 `std::erase_if` 统一这些写法**：

```cpp
std::erase_if(v, [](int x) { return x % 2 == 0; });     // C++20，一行
```

## 八、vector 的内存增长

实测容量增长序列：

```
        size=1 capacity=1
        size=2 capacity=2
        size=3 capacity=4
        size=5 capacity=8
        size=9 capacity=16
        size=17 capacity=32
        size=33 capacity=64
        size=65 capacity=128
```

libstdc++ 用 **2 倍增长**（MSVC 用 1.5 倍）。
2 倍增长保证 `push_back` 的**均摊复杂度是 O(1)**。

### reserve 的收益

100 万次 `push_back`：

```
        不 reserve: 11992 us
        先 reserve: 8385 us
```

快约 **1.4 倍**。知道大小时一定要 `reserve`
—— 不只是省时间，还避免了扩容时的元素搬迁（见
[02-03](../part2_memory/02-03_move_semantics.md)）。

### clear 不释放内存

```cpp
std::vector<int> big(1000000);
big.clear();
big.capacity();                 // 仍然是 1000000！
big.shrink_to_fit();            // 这才释放
```

实测：

```
      clear() 后 size=0 capacity=1000000（内存没还）
      shrink_to_fit() 后 capacity=0
```

这是有意的设计 —— 容器假定你可能还要往里放东西。
真要还内存，用 `shrink_to_fit()`（C++11）或者老办法：

```cpp
std::vector<int>().swap(big);     // C++98 的办法：和空 vector 交换
```

## 九、容器选择决策表

```
需要存一组元素？
├── 固定大小，编译期已知 -> std::array
├── 需要按键查找
│   ├── 需要有序 -> std::map / std::set
│   └── 不需要有序 -> std::unordered_map / std::unordered_set   ← 默认
├── 需要序列
│   ├── 主要在尾部增删 + 随机访问 -> std::vector               ← 默认
│   ├── 两端都要高效增删 -> std::deque
│   ├── 中间频繁增删且已有迭代器 -> std::list
│   └── 内存极度受限的单向链表 -> std::forward_list
└── 只需要栈/队列语义 -> std::stack / queue / priority_queue
```

**`std::vector` 应该是默认选择。** 理由：

- 连续内存 → 缓存局部性最好
- 没有每元素的额外开销（链表每个节点要存指针）
- 随机访问 O(1)
- 即使中间插入是 O(n)，在元素数不大时仍然比链表快
  （memmove 极快，而链表要跳指针）

Bjarne Stroustrup 做过实测：即使是"在随机位置插入"这种 `list`
理论占优的场景，`vector` 在几十万元素以内都更快。
**只在确实有大量中间插入删除、而且元素很大、而且已经有迭代器时，才考虑 `list`。**

## 十、小结

| 要点 | 说明 |
|---|---|
| `std::array` `[C++11]` | 零开销替代内置数组；知道大小、不退化成指针、支持比较 |
| `forward_list` `[C++11]` | 省一个指针；故意没有 `size()` 和 `push_back` |
| `unordered_*` `[C++11]` | 实测查找快 5.5 倍、插入快 2.9 倍；默认选它除非需要有序 |
| 哈希容器要点 | 插入前 `reserve`；自定义类型要提供哈希 + 相等比较 |
| emplace | 完美转发，省掉临时对象；但已有对象时 `push_back + move` 一样好 |
| `try_emplace` `[C++17]` | 键已存在时**不构造值**，避免 `move` 被浪费 |
| `insert_or_assign` `[C++17]` | 不要求值可默认构造 |
| vector 失效 | 扩容 → 全部失效；`erase` → 该位置及之后 |
| list/map 失效 | 插入**不失效**；`erase` 只影响被删元素 |
| unordered 失效 | rehash → 迭代器失效，但**引用和指针有效** |
| 边遍历边删 | 用 `it = c.erase(it)`，或 erase-remove 惯用法 |
| vector 增长 | libstdc++ 2 倍；`reserve` 快 1.4 倍 |
| `clear` 不还内存 | 要 `shrink_to_fit()` |
| **默认选择** | **`vector`** —— 缓存局部性胜过算法复杂度 |

> 下一章：[07-03 迭代器与算法](07-03_algorithms.md)
