# 02-04 完美转发

> 标准：`[C++11]`
> demo：[`02_04_perfect_forwarding.cpp`](../../demos/part2_memory/02_04_perfect_forwarding.cpp)

## 一、解决了什么问题

写一个"包装函数"：接收参数，原样传给另一个函数。
听起来简单，C++98 下做不到。

假设目标函数有三个重载，分别处理非 const 左值、const 左值、右值：

```cpp
void take(Probe&);        // 非const左值
void take(const Probe&);  // const左值
void take(Probe&&);       // 右值
```

要写一个包装函数 `wrapper`，使得 `wrapper(x)` 的效果和直接调 `take(x)` 完全一样。

**尝试一：按值传参**

```cpp
void wrapper(Probe p) { take(p); }
```

两个问题：多一次拷贝；而且 `p` 是有名字的变量 ——
**它是左值**，所以永远只会调 `take(Probe&)`。实测：

```
    按值传参（注意多了一次拷贝，且总是非const左值）:
      Probe(const Probe&)  拷贝
      take(Probe&)        非const左值
```

**尝试二：按 const 引用传参**

```cpp
void wrapper(const Probe& p) { take(p); }
```

不拷贝了，但 `p` 是 const 左值，永远调 `take(const Probe&)`，
丢失了可写性和右值信息：

```
    按 const& 传参（总是 const 左值）:
      take(const Probe&)  const左值
```

**尝试三：手写所有重载**

```cpp
void wrapper(Probe& p)       { take(p); }
void wrapper(const Probe& p) { take(p); }
void wrapper(Probe&& p)      { take(std::move(p)); }
```

这次行为正确了：

```
    手写三个重载（正确，但组合爆炸）:
      take(Probe&)        非const左值
      take(const Probe&)  const左值
      take(Probe&&)       右值
```

但代价是**组合爆炸**：一个参数要 3 个重载，两个参数要 9 个，
三个参数要 27 个。而且这还只是针对 `Probe` 一种类型 ——
想支持任意类型，再乘上模板。

容器的 `emplace_back` 要支持任意个数、任意类型的参数，按这个路子根本写不出来。

C++11 的解法由两部分组成：**万能引用**（接收任何东西并记住它是什么）
和 **`std::forward`**（把记住的信息还原出去）。

## 二、万能引用（forwarding reference）

```cpp
template <class T>
void wrapper(T&& p) {              // 这里的 T&& 不是右值引用！
    take(std::forward<T>(p));
}
```

当 `T&&` 中的 `T` 是**需要推导的模板参数**时，`T&&` 不是右值引用，
而是**万能引用**（标准术语是 forwarding reference，Scott Meyers 称之为 universal reference）。
它能绑定任何东西：左值、右值、const、非 const。

实测结果与手写三个重载完全一致：

```
    转发非const左值:
      take(Probe&)        非const左值
    转发const左值:
      take(const Probe&)  const左值
    转发右值:
      take(Probe&&)       右值
```

一个模板函数替代了所有重载。

### 怎么区分万能引用和右值引用

只有**形如 `T&&` 且 `T` 由推导得来**时才是万能引用：

```cpp
template <class T>
void universal(T&& t);              // 万能引用：T 需要推导

void rvalue_only(Probe&& p);        // 右值引用：类型已确定，只能绑右值

template <class T>
void not_universal(const T&& t);    // 不是：有 const 限定

template <class T>
void not_universal2(std::vector<T>&& v);   // 不是：不是裸的 T&&

template <class T>
class Widget {
    void method(T&& t);             // 不是！T 由类模板确定，调用成员函数时不推导
};
```

`auto&&` 也是万能引用（`auto` 的推导规则和模板一致）：

```cpp
auto&& r1 = x;          // x 是左值 -> r1 是 Probe&
auto&& r2 = Probe{};    // 右值     -> r2 是 Probe&&

for (auto&& item : container) { /* 对任何容器都安全 */ }
```

实测验证：

```
    万能引用能接左值和右值；真正的右值引用只能接左值之外的右值
```

## 三、引用折叠：万能引用为什么能工作

万能引用的机制建立在两条规则上。

### 规则一：特殊的推导规则

模板参数是 `T&&` 时：

- 传**左值**，`T` 被推导为 `T&`（左值引用）
- 传**右值**，`T` 被推导为 `T`（非引用）

实测：

```
    传左值 x:
      T 是左值引用吗: 1, T&& 是左值引用吗: 1      <- T = int&
    传右值 10:
      T 是左值引用吗: 0, T&& 是左值引用吗: 0      <- T = int
```

### 规则二：引用折叠

把推导出的 `T` 代回 `T&&`，就出现了"引用的引用"。
C++ 不允许直接写引用的引用，但在模板替换、typedef、`auto`、`decltype`
中间接产生时，会按下面的规则**折叠**：

| 组合 | 折叠结果 |
|---|---|
| `T& &` | `T&` |
| `T& &&` | `T&` |
| `T&& &` | `T&` |
| `T&& &&` | `T&&` |

一句话记法：**只有"右值引用的右值引用"折叠成右值引用，其余三种都是左值引用**。
（像乘法：左值引用是"负号"，一旦出现就是左值引用。）

实测验证四条规则：

```
      (int&)&   -> 左值引用: 1
      (int&)&& -> 左值引用: 1
      (int&&)& -> 左值引用: 1
      (int&&)&&-> 右值引用: 1
```

注意这个验证必须经由类型别名才能写出来。直接写 `int& &` 是语法错误：

```
error: cannot declare reference to 'int&', which is not a typedef or
       a template type argument
```

### 两条规则合起来看

```cpp
template <class T> void wrapper(T&& p);

Probe lv;
wrapper(lv);        // T = Probe&   -> 参数类型 Probe& && -> 折叠成 Probe&
wrapper(Probe{});   // T = Probe    -> 参数类型 Probe&&   -> 保持 Probe&&
```

传左值得到左值引用参数，传右值得到右值引用参数 ——
**原始的值类别被编码进了 `T` 里**。接下来要做的就是把它还原出来。

## 四、std::forward：把值类别还原

问题是：在函数体内，参数 `p` **有名字，所以它是左值**，
哪怕它的类型是 `Probe&&`：

```cpp
template <class T>
void wrapper(T&& p) {
    take(p);        // 永远调 take(Probe&)！因为 p 是左值
}
```

需要的是"如果 `T` 是左值引用，就当左值传；否则当右值传"。
这正是 `std::forward` 做的事：

```cpp
take(std::forward<T>(p));
```

它的实现大致是：

```cpp
template <class T>
constexpr T&& forward(std::remove_reference_t<T>& t) noexcept {
    return static_cast<T&&>(t);
}
```

关键在 `static_cast<T&&>`：

- `T = Probe&` 时，`T&&` 折叠成 `Probe&` → 转成左值引用 → 调 `take(Probe&)`
- `T = Probe` 时，`T&&` 就是 `Probe&&` → 转成右值引用 → 调 `take(Probe&&)`

和 `std::move` 一样，`std::forward` 也只是一个 `static_cast`，**没有运行时开销**。

注意 `std::forward` **必须显式指定模板参数** `std::forward<T>(p)` ——
因为它要靠 `T` 来判断该转成什么。写成 `std::forward(p)` 是错的。

## 五、std::move 和 std::forward 的分工

两者都是 `static_cast`，差别在于：

| | 行为 | 用在哪 |
|---|---|---|
| `std::move(x)` | **无条件**转成右值 | 确定不再需要 `x` 时 |
| `std::forward<T>(x)` | **有条件**转成右值（取决于 `T`）| 转发函数里，保留调用方的值类别 |

实测对比：

```cpp
template <class T> void always_moves(T&& t) { take(std::move(t)); }
template <class T> void forwards(T&& t)     { take(std::forward<T>(t)); }

Probe lv;
always_moves(lv);   // -> take(Probe&&)   把调用方的左值当成了右值！
forwards(lv);       // -> take(Probe&)    正确
```

输出：

```
    std::move 不管来源，总是转成右值（可能误偷调用方的对象）:
      take(Probe&&)       右值
    std::forward 保留原来的值类别:
      take(Probe&)        非const左值
```

第一种写法很危险 —— 调用方传了个还要继续用的左值，
却被包装函数当成右值移走了资源，而且不会有任何警告。

规则：**万能引用参数一律用 `std::forward`，右值引用参数用 `std::move`**。

```cpp
template <class T>
void f(T&& t) { g(std::forward<T>(t)); }   // 万能引用 -> forward

void h(Probe&& p) { g(std::move(p)); }      // 右值引用 -> move
```

## 六、实际应用

### make_unique / make_shared

```cpp
template <class T, class... Args>
std::unique_ptr<T> my_make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

auto p = my_make_unique<Probe>(1, "x");
```

实测输出证明参数被原样转发给了构造函数：

```
    my_make_unique<Probe>(1, "x") 转发两个参数:
      Probe(int, const char*)
```

注意 `Args&&... args` 和 `std::forward<Args>(args)...` 的配合 ——
这是**可变参数模板 + 完美转发**的标准组合，见
[05-04 可变模板参数](../part5_template/05-04_variadic.md)。

### emplace 系列

这是完美转发最常见的用途。对比 `push_back` 和 `emplace_back`：

```
      push_back(Probe{}):
      Probe()                    <- 先构造临时对象
      Probe(Probe&&)       移动   <- 再移动进容器
      emplace_back(1):
      Probe(int)                 <- 直接在容器内构造，就一步
```

`emplace_back(1)` 把实参 `1` 完美转发到容器内存上的构造函数调用，
**没有临时对象，也没有移动**。这是 `emplace` 系列相对 `push`/`insert` 的全部优势。

```cpp
// 简化的实现思路
template <class... Args>
void emplace_back(Args&&... args) {
    // 在 end() 位置用 placement new 构造（见 02-02）
    new (end_ptr) T(std::forward<Args>(args)...);
    ++size;
}
```

什么时候用 `emplace`？

- 参数需要用来**构造**新元素时，用 `emplace_back(args...)`
- 已经有一个现成的对象要**放进去**时，`push_back(std::move(obj))` 同样好

### 其他

完美转发在标准库里到处都是：
`std::thread` 的构造函数（转发可调用对象和参数）、
`std::function`、`std::bind`、`std::invoke`、
`std::optional::emplace`、`std::vector::resize`。

## 七、三个常见错误

### 错误一：同一个参数转发两次

```cpp
template <class T>
void bad(T&& t) {
    use(std::forward<T>(t));
    use(std::forward<T>(t));   // 第一次已经可能把资源移走了
}
```

如果 `T` 推导为非引用（传了右值），第一次 `forward` 就把资源交出去了，
第二次拿到的是被掏空的对象。

规则：**一个参数只转发一次，而且要在最后一次使用的位置**。

### 错误二：万能引用吃掉所有重载

万能引用在重载决议中是**精确匹配**，优先级极高：

```cpp
void f(int);

template <class T>
void f(T&&);           // 万能引用

short s = 1;
f(s);                  // 调的是模板！因为 short -> int 需要提升，
                       // 而模板可以精确匹配 short&
```

这个坑在构造函数上尤其危险：

```cpp
class Widget {
public:
    template <class T>
    explicit Widget(T&& name);        // 想接受各种字符串

    Widget(const Widget& rhs);        // 拷贝构造
};

Widget w1("hello");
Widget w2(w1);        // 调的是模板构造函数！不是拷贝构造！
                      // 因为 w1 是非 const 左值，模板能精确匹配 Widget&
```

解决办法：用 `enable_if` 排除掉本类型，或者 C++20 的 concepts：

```cpp
template <class T,
          class = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Widget>>>
explicit Widget(T&& name);
```

详见 [05-05 type_traits](../part5_template/05-05_type_traits.md)。
实践上，**优先考虑按值传参 + `std::move`**，能避开这整类问题：

```cpp
class Widget {
public:
    explicit Widget(std::string name) : _name(std::move(name)) {}   // 简单且够好
};
```

### 错误三：对 `const` 万能引用抱有期待

```cpp
template <class T>
void f(const T&& t);     // 这不是万能引用，只接受 const 右值
```

加了 `const` 就退化成普通的右值引用，几乎没有用途。

## 八、小结

| 概念 | 要点 |
|---|---|
| 解决什么 | 写包装函数时无法保留参数的值类别；手写重载会组合爆炸 |
| 万能引用 | 形如 `T&&` 且 `T` 由推导得来；`auto&&` 也是 |
| 不是万能引用 | `const T&&`、`vector<T>&&`、类模板成员函数的 `T&&` |
| 推导规则 | 传左值 → `T = U&`；传右值 → `T = U` |
| 引用折叠 | 只有 `&& &&` → `&&`，其余三种 → `&` |
| 为什么需要 forward | 函数体内的参数**有名字就是左值**，直接传会丢失右值性 |
| `std::forward<T>(x)` | 有条件转右值；必须显式写 `<T>`；零运行时开销 |
| `move` vs `forward` | 右值引用用 `move`，万能引用用 `forward` |
| 典型应用 | `emplace_back`、`make_unique`、`thread` 构造函数 |
| 禁忌 | 同一参数转发两次；万能引用构造函数会抢走拷贝构造 |

> 下一章：[02-05 智能指针](02-05_smart_pointers.md)
