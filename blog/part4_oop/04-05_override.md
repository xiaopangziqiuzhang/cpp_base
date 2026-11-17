# 04-05 override/final 与三种"同名"

> 标准：`override`、`final` `[C++11]`
> demo：[`04_05_override.cpp`](../../demos/part4_oop/04_05_override.cpp)

## 一、三种"同名"必须分清

C++ 里有三种不同的"函数同名"现象，初学者常把它们混为一谈。
它们的触发条件和行为完全不同。

| | 条件 | 作用域 | 调用解析 |
|---|---|---|---|
| **重载**（overload）| **同一作用域**，参数列表不同 | 同一个类内 | 编译期按参数选 |
| **隐藏**（hiding）| 派生类有同名成员（任何签名）| 跨类 | 派生类的名字遮蔽基类的 |
| **覆盖**（override）| 虚函数 + 签名**完全相同** | 跨类 | **运行期按对象实际类型** |

```cpp
class Base {
public:
    virtual void virtual_func() const;          // 用于演示覆盖
    void normal_func() const;                    // 用于演示隐藏
    void overloaded(int) const;                  // 这两个是重载
    void overloaded(double) const;
};

class Derived : public Base {
public:
    void virtual_func() const override;   // 覆盖
    void normal_func() const;             // 隐藏（非虚函数同名）
};
```

实测行为差异：

```
      覆盖（virtual + 签名相同）:
        通过基类指针: Derived::virtual_func()     <- 动态绑定
        通过对象:     Derived::virtual_func()
      隐藏（非虚函数同名）:
        通过基类指针: Base::normal_func()         <- 静态绑定！
        通过对象:     Derived::normal_func()
```

**覆盖**是多态的实现方式，调用目标取决于对象的实际类型。
**隐藏**只是名字查找的结果，调用目标取决于表达式的静态类型 ——
它几乎总是设计失误（见 [04-01 的名字隐藏](04-01_inheritance.md)）。

## 二、C++11 之前的问题：覆盖失败是静默的

要正确覆盖一个虚函数，签名必须**完全一致**：函数名、参数列表、
const 限定、引用限定（C++11）全都要对。差一点就不是覆盖，
而是**定义了一个新的虚函数并隐藏了基类的那个**。

而编译器**不会报错，也不会警告**：

```cpp
class Base {
public:
    virtual void f(int x) const;
    virtual void g() const;
    virtual void h(int x);
};

class DerivedBuggy : public Base {
public:
    void f(double x) const;    // 参数类型写错 -> 没覆盖
    void g();                  // 漏了 const  -> 没覆盖
    void h(int x) const;       // 多了 const  -> 没覆盖
};
```

实测三次调用全部落到基类：

```
      有 bug 的派生类（三个函数都没覆盖成功）:
        p->f(1):  Base::f(int)
        p->g():   Base::g() const
        p->h(1):  Base::h(int)
      ^ 三次都调用了基类版本，覆盖全部失败，而且编译器不报错
```

我单独验证过编译器的沉默程度：

```
=== 不加 override 时：完全没有警告 ===
（空 = 无警告，bug 被静默接受）
```

即使开了 `-Wall -Wextra` 也没有任何提示。

这类 bug 的特点是**难以察觉**：代码编译通过、能跑、
只是多态行为不对。往往要到运行时发现"怎么调的是基类版本"才会去查，
而排查时又很难注意到那个多出来或少掉的 `const`。

## 三、override：让覆盖失败变成编译错误

```cpp
class DerivedChecked : public Base {
public:
    void f(int x) const override;   // 显式声明"我要覆盖基类的虚函数"
    void g() const override;
    void h(int x) override;
};
```

签名写错时，编译器直接报错。实测四类错误：

```
error: 'void D1::f(double) const' marked 'override', but does not override
error: 'void D2::g()' marked 'override', but does not override
error: 'void D3::not_virtual()' marked 'override', but does not override
error: 'void D4::typo()' marked 'override', but does not override
```

对应的四类问题：

1. **参数列表不匹配**（`f(double)` vs `f(int)`）
2. **const / 引用限定不匹配**（漏写或多写 `const`）
3. **基类对应函数不是虚函数**（`not_virtual`）
4. **函数名拼写错误**（`typo`）

**代价是零** —— `override` 是纯编译期检查，不产生任何运行时代码，
不影响对象布局。

### 它是"上下文关键字"

`override` 不是保留字，而是只在特定位置有特殊含义的标识符。
所以老代码里如果有变量叫 `override`，不会冲突：

```cpp
int override = 1;      // 合法，仍然可以这么用
```

同理 `final` 也是。这是 C++11 为了兼容性做的设计。

### 实践规则

**所有覆盖虚函数的地方都写 `override`。** 这条规则没有例外，
成本是零，收益是消除一整类 bug。

配套的编译选项：

```bash
-Wsuggest-override        # GCC/Clang：覆盖了虚函数但没写 override 时警告
-Winconsistent-missing-override   # Clang
```

把它们加进项目的编译选项，就能保证新代码不会漏写。

另外注意：**写了 `override` 就不需要再写 `virtual`**。

```cpp
virtual void f() const override;   // 冗余
void f() const override;           // 推荐
```

派生类的覆盖版本自动是虚函数，`override` 已经表明了这是覆盖关系。

## 四、final：禁止进一步覆盖或继承

`final` 有两种用法。

### 用法一：修饰虚函数 —— 禁止派生类再覆盖

```cpp
class Middle : public Base {
public:
    void can_override() override { /* 派生类还能继续覆盖 */ }
    void cannot_override() final { /* 到此为止 */ }
};

class Leaf : public Middle {
public:
    void can_override() override { /* ok */ }
    // void cannot_override() override {}
    // error: virtual function 'virtual void Leaf::cannot_override()'
    //        overriding final function
};
```

实测 `p->cannot_override()` 调用的是 `Middle` 的版本，无法被 `Leaf` 改变。

### 用法二：修饰类 —— 禁止被继承

```cpp
class NoInherit final {
public:
    void f();
};

// class Try : public NoInherit {};
// error: cannot derive from 'final' base 'NoInherit'
```

### 什么时候用 final

**用于表达设计意图**：

- 这个类不是为继承设计的（没有虚析构函数、没有 protected 接口）
- 这个虚函数的行为是契约的一部分，不允许派生类改变
  （比如实现了某个必须保证的不变量）

**用于优化**（下一节）。

但不要滥用。过度使用 `final` 会让类无法被测试替身（mock）继承，
也堵死了未来扩展的可能。一个经验做法是：
**要么把类设计成可继承的（虚析构函数 + 文档说明扩展点），
要么标记为 `final`** —— 不要留下"看起来能继承但其实不行"的中间状态。

## 五、final 帮助编译器去虚化

虚调用的开销来自"运行期才知道调哪个函数"。
如果编译器能确定实际类型，就可以把虚调用变成直接调用，甚至内联 ——
这叫**去虚化（devirtualization）**。

`final` 给编译器提供了关键信息：

```cpp
class FinalDerived final : public Base {
public:
    int compute(int x) const override { return x * 2; }
};

int use_final(const FinalDerived& d, int x) {
    return d.compute(x);     // 因为 FinalDerived 是 final，
                             // 编译器知道不可能有更深的派生类，
                             // 可以直接调用甚至内联
}
```

实测 `use_final(d, 21) = 42`。在 `-O2` 下这个调用会被内联，
虚函数开销降为零。

去虚化的其他触发条件：

- 通过**对象**（而非指针/引用）调用虚函数 —— 类型确定
- 类或虚函数标记 `final`
- 整个程序优化（LTO）能看到所有派生类
- 编译器能通过数据流分析确定实际类型

详见 [04-07 多态的代价](04-07_crtp.md)。

## 六、协变返回类型：覆盖规则的唯一例外

覆盖要求签名完全一致，但**返回类型可以是"更具体"的**：

```cpp
class Base {
public:
    virtual Base* clone() const { return new Base(*this); }
};

class Derived : public Base {
public:
    Derived* clone() const override {       // 返回 Derived* 而不是 Base*
        return new Derived(*this);
    }
};
```

实测可行。这叫**协变返回类型（covariant return type）**。

要求：

- 返回类型必须是**指针或引用**（值类型不行，因为大小不同）
- 派生类返回的类型必须是基类返回类型的**公有派生类**
- 指针/引用的 const 限定不能变弱

好处是调用方在知道具体类型时能拿到精确的类型，不需要再转换：

```cpp
Derived d;
Derived* dp = d.clone();      // 直接得到 Derived*，不需要 static_cast
Base* bp = &d;
Base* bp2 = bp->clone();      // 通过基类指针调用，静态类型是 Base*（但实际对象是 Derived）
```

这是 `clone()` 惯用法的标准写法（见
[04-02 多态基类的拷贝](04-02_derived_lifecycle.md)）。

注意协变**只对返回类型有效，参数类型不能协变** ——
参数协变会破坏 Liskov 替换原则（派生类不能缩小接受的参数范围）。
C++ 不支持参数的逆变或协变。

## 七、小结

| 概念 | 要点 |
|---|---|
| **重载** | 同一作用域，参数不同，编译期决议 |
| **隐藏** | 派生类同名成员遮蔽基类的；几乎总是设计失误 |
| **覆盖** | 虚函数 + 签名完全一致；运行期按对象实际类型决议 |
| 覆盖的签名要求 | 函数名、参数列表、const 限定、引用限定全都要一致 |
| **C++11 之前的问题** | 签名写错时静默变成"隐藏"，编译器完全不警告 |
| `override` `[C++11]` | 显式声明覆盖意图，签名不匹配就编译错误；**零代价** |
| override 能查出 | 参数不匹配、const 不匹配、基类函数非虚、函数名拼错 |
| 配套选项 | `-Wsuggest-override` 检查漏写 |
| 写法 | 写了 `override` 就不要再写 `virtual` |
| `final`（函数）`[C++11]` | 禁止派生类再覆盖 |
| `final`（类）`[C++11]` | 禁止被继承 |
| final 的额外价值 | 给编译器去虚化的依据，可能内联掉虚调用 |
| 协变返回类型 | 覆盖时返回类型可以是更具体的指针/引用；参数类型不能协变 |

**一句话：覆盖虚函数时一律写 `override`，这是现代 C++ 的硬性规范。**

> 下一章：[04-06 纯虚函数、抽象类、接口](04-06_abstract.md)
