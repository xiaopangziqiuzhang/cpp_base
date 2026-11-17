# 01-12 范围 for 与结构化绑定

> 标准：范围 for `[C++11]`、结构化绑定与 if/switch 初始化语句 `[C++17]`
> demo：[`01_12_range_for.cpp`](../../demos/part1_basics/01_12_range_for.cpp)

## 一、范围 for 解决了什么问题

C++98 遍历容器有两种写法，各有各的毛病：

```cpp
// 下标：只适用于随机访问容器，且 i 的类型容易写错
for (std::size_t i = 0; i < v.size(); ++i) { use(v[i]); }

// 迭代器：通用，但类型名冗长，三段式里藏着好几个出错点
for (std::vector<int>::const_iterator it = v.begin(); it != v.end(); ++it) {
    use(*it);
}
```

迭代器写法的具体问题：

1. **类型名太长** —— `auto` 能缓解（见 [01-10](01-10_auto_decltype.md)），但还是啰嗦
2. **`end()` 被反复调用** —— 每次循环都调一次，虽然通常会被优化掉
3. **容易写错** —— `it != end` 写成 `it < end`（关联容器不支持）、
   用了 `i` 却访问 `v[j]`、循环变量复用导致的笔误
4. **必须解引用** —— `*it` 这层间接妨碍阅读

C++11 借鉴 Java 的 for-each，加入了范围 for：

```cpp
for (int x : v) { use(x); }
```

把"怎么遍历"交给编译器，代码里只剩"对每个元素做什么"。

## 二、三种形式，按用途选

```cpp
for (auto x : items)        { x.id = 1; }   // 拷贝：改不到原对象
for (auto& x : items)       { x.id = 2; }   // 引用：能改
for (const auto& x : items) { sum += x.id; } // 只读引用：默认首选
```

实测输出：

```
  for (auto x : c)       -> items[0].id = 0     <- 修改丢失了
  for (auto& x : c)      -> items[0].id = 2
  for (const auto& x : c) -> sum = 6
```

| 写法 | 语义 | 何时用 |
|---|---|---|
| `for (const auto& x : c)` | 只读引用 | **默认选择**，不拷贝也不会误改 |
| `for (auto& x : c)` | 可写引用 | 需要修改元素 |
| `for (auto x : c)` | 拷贝 | 元素是基本类型，或确实需要副本 |
| `for (auto&& x : c)` | 万能引用 | 泛型代码、或容器返回代理对象时 |

`auto x` 的拷贝陷阱在 [01-10 的陷阱三](01-10_auto_decltype.md) 讲过 ——
遍历 `vector<Heavy>` 时每轮都拷贝一个重对象，而且对元素的修改会丢失。

最后一行的 `auto&&` 值得说明：当容器的 `operator*` 返回**代理对象**（右值）时，
`auto&` 绑不上，必须用 `auto&&`。写泛型代码时 `auto&&` 是最安全的选择：

```cpp
for (auto&& x : get_any_container()) { /* 对任何容器都能工作 */ }
```

## 三、它是怎么工作的

范围 for 是纯语法糖。`for (auto x : v) { body }` 大致展开为：

```cpp
{
    auto&& range = v;              // 绑定到容器，右值也能延长寿命
    auto it = std::begin(range);
    auto end = std::end(range);    // 注意：只求值一次
    for (; it != end; ++it) {
        auto x = *it;
        body
    }
}
```

实测手工展开的结果与范围 for 一致。

有两个推论：

**推论一：`end()` 只在循环开始时求值一次。** 这既是优化，也是约束 ——
循环体内改变容器大小，`end` 不会更新。

**推论二：临时容器是安全的。** `auto&& range = v` 能延长临时对象的寿命：

```cpp
for (const auto& x : make_vector()) { /* 安全，临时 vector 活到循环结束 */ }
```

但有个著名的例外 —— 如果遍历的是**临时对象的成员或子对象**，就会悬垂：

```cpp
// 危险：get_struct() 返回的临时对象在 range 初始化后就销毁了，
//       而 range 绑定的是它的成员
for (char c : get_struct().name) { /* C++20 之前是未定义行为 */ }
```

C++23 修正了这个缺陷（P2644）。C++17 环境下的写法是先存下来：

```cpp
auto tmp = get_struct();
for (char c : tmp.name) { /* 安全 */ }
```

## 四、陷阱：循环中修改容器

```cpp
for (int x : v) {
    if (x % 2 == 1) v.push_back(x * 10);   // 未定义行为！
}
```

`push_back` 可能导致 `vector` 重新分配内存，于是 `it` 和 `end` 全部失效。
这类 bug 通常表现为"大部分时候能跑，数据量一大就崩"。

正确做法是**先收集，再修改**：

```cpp
std::vector<int> to_add;
for (int x : v) {
    if (x % 2 == 1) to_add.push_back(x * 10);
}
v.insert(v.end(), to_add.begin(), to_add.end());
```

实测输出：

```
  先收集再插入: 1 2 3 10 30
```

需要边遍历边删除时，用传统迭代器循环配合 `erase` 的返回值，
或者用 erase-remove 惯用法（见 [07-03](../part7_stdlib/07-03_algorithms.md)）。

## 五、让自定义类型支持范围 for

要求很低：**提供 `begin()` 和 `end()`**（成员函数，或 ADL 能找到的自由函数），
返回的对象支持 `*`、`++`、`!=` 三个操作。

```cpp
class IntRange {
public:
    IntRange(int from, int to) : _from(from), _to(to) {}

    class Iterator {
    public:
        explicit Iterator(int v) : _v(v) {}
        int operator*() const                      { return _v; }
        Iterator& operator++()                     { ++_v; return *this; }
        bool operator!=(const Iterator& rhs) const { return _v != rhs._v; }
    private:
        int _v;
    };

    Iterator begin() const { return Iterator(_from); }
    Iterator end() const   { return Iterator(_to); }
private:
    int _from, _to;
};

for (int x : IntRange(1, 6)) { /* 1 2 3 4 5 */ }
```

注意 `begin()` 和 `end()` **不必返回同一类型** —— 这是 C++17 放宽的
（为哨兵迭代器铺路，C++20 ranges 大量使用这一点）。
这让"无限序列"、"读到文件尾才停"这类惰性序列成为可能。

如果无法修改目标类（比如第三方类型），可以在**它所在的命名空间**里
提供自由函数 `begin`/`end`，ADL 会找到它们（见 [01-01](01-01_namespace.md)）。

## 六、结构化绑定 `[C++17]`

### 解决什么问题

遍历 `map` 时，元素是 `std::pair`，于是代码里全是 `.first` 和 `.second`：

```cpp
for (const auto& pair : scores) {
    std::cout << pair.first << " = " << pair.second << "\n";
}
```

`.first` 是什么？得知道这是个 map 才能猜出是 key。
嵌套一层就更难读了：`it->second.first`。

另一个场合是返回多个值的函数：

```cpp
// C++11：要么先声明再 tie，要么忍受 .first/.second
std::pair<iterator, bool> result = m.insert({"key", 1});
if (result.second) { use(result.first); }

iterator it; bool ok;
std::tie(it, ok) = m.insert({"key", 1});   // tie 要求变量先声明，且不能用 auto
```

### 写法

结构化绑定给组成部分**起名字**：

```cpp
for (const auto& [name, score] : scores) {
    std::cout << name << " = " << score << "\n";
}

auto [iter, inserted] = m.insert({"key", 1});
if (inserted) { /* ... */ }
```

实测输出：

```
  map::insert 返回 inserted=1 key=key
  重复插入 inserted=0（已存在，值仍是 1）
```

可读性的提升是实质性的 —— `inserted` 比 `result.second` 清楚得多。

### 三种可绑定的来源

**一、数组**

```cpp
int arr[3]{10, 20, 30};
auto [a, b, c] = arr;      // 元素个数必须完全匹配
```

**二、tuple-like 类型** —— 任何支持 `std::tuple_size`、
`std::tuple_element`、`get<I>` 的类型：

```cpp
auto [num, text, flag] = get_tuple();     // std::tuple
auto [key, value] = *m.begin();           // std::pair
```

自定义类型也可以通过特化这三个设施来支持绑定。

**三、所有非静态成员都是 public 的类**（聚合类型）：

```cpp
struct Record { int id; std::string name; double value; };

Record r{7, "widget", 3.14};
auto [id, name, value] = r;
```

注意是**全部成员按声明顺序**绑定，不能只绑一部分，也不能跳过。
有私有成员的类不能这样绑定。

### 绑定到引用可以写回

```cpp
auto& [rid, rname, rvalue] = r;
rid = 8;                  // 修改了 r.id
```

实测 `r.id` 变成了 8。遍历容器时常用这个形式修改元素：

```cpp
for (auto& [name, score] : scores) { score += 1; }
```

### 几个细节

- **名字不能省略，也不能用 `_` 占位** ——
  不关心的部分仍要起名，可以用 `[[maybe_unused]]` 抑制警告
  （C++26 才引入 `_` 占位符）
- **不能嵌套** —— `auto [[a, b], c] = ...;` 不合法
- `auto`/`auto&`/`const auto&`/`auto&&` 的语义与范围 for 中一致
- 绑定的名字**不是变量**，而是对隐藏对象成员的别名。
  这导致它们不能被 lambda 捕获（C++20 才允许）

## 七、if / switch 初始化语句 `[C++17]`

### 解决什么问题

C++11 的 `find`-then-check 模式有个作用域缺陷：

```cpp
auto it = config.find("timeout");
if (it != config.end()) {
    use(it->second);
}
// it 在这里依然可见，但已经没有意义了
// 后面不小心用到它，或者和下一个 find 的结果搞混，都是常见 bug
```

想限制作用域就得额外加一层花括号，很笨。

### 写法

把初始化语句放进 `if`，变量的作用域限制在 `if`/`else` 内：

```cpp
if (auto it = config.find("timeout"); it != config.end()) {
    use(it->second);
} else {
    // it 在这里也可见（因为它属于整个 if 语句）
}
// it 在这里不可见
```

分号前是初始化语句，分号后是条件。这和 `for` 的第一个分号是同一个思路。

### 与结构化绑定组合

这两个特性配合起来，构成了现代 C++ 最常见的一种写法：

```cpp
if (auto [iter, ok] = config.insert({"retries", 3}); ok) {
    std::cout << "插入成功: " << iter->first << " = " << iter->second << "\n";
}
```

一行完成"调用、解构返回值、检查状态"，且所有变量不泄漏到外层。

实测输出：

```
  C++17: timeout = 30
  插入成功: retries = 3
```

### switch 同样支持

```cpp
switch (int n = get_count(); n) {
    case 1:  /* ... */ break;
    case 2:  /* ... */ break;
    default: use(n);        // n 在所有分支里可用
}
```

### 典型应用场景

```cpp
// 1. 加锁 + 条件判断
if (std::lock_guard lk(_mtx); _queue.empty()) { return; }

// 2. 返回错误码的 API
if (auto err = do_work(); err != 0) { log(err); return err; }

// 3. dynamic_cast 检查
if (auto* d = dynamic_cast<Derived*>(base); d != nullptr) { d->special(); }

// 4. 正则匹配
if (std::smatch m; std::regex_match(line, m, re)) { use(m[1]); }
```

## 八、小结

| 特性 | 标准 | 解决的问题 |
|---|---|---|
| 范围 for | C++11 | 迭代器三段式冗长、易错、需解引用 |
| 结构化绑定 | C++17 | `.first`/`.second` 无语义、`std::tie` 需预先声明变量 |
| if/switch 初始化语句 | C++17 | 临时变量泄漏到外层作用域 |

要点：

- 范围 for 默认用 `const auto&`，要改元素用 `auto&`，泛型代码用 `auto&&`
- `end()` 只求值一次；循环体内不要改变容器大小
- 自定义类型只需提供 `begin()`/`end()` 即可支持范围 for
- 结构化绑定支持数组、tuple-like、全 public 成员的聚合类
- 绑定的名字是别名不是变量，C++17 下不能被 lambda 捕获
- `if (init; cond)` + 结构化绑定是现代 C++ 处理"返回值+状态"的标准写法

Part 1 到此结束。

> 下一章：[02-01 程序内存布局](../part2_memory/02-01_memory_layout.md)
