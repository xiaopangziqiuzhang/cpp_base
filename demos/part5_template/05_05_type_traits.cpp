// std=c++17
// 对应章节: blog/part5_template/05-05_type_traits.md
// 主题: static_assert、type_traits、SFINAE、enable_if、if constexpr

#include <iostream>
#include <list>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

// ---------- 1. static_assert [C++11] ----------
namespace static_assert_demo {
    template <class T>
    class RequiresInteger {
        // C++11 需要消息，C++17 起可以省略
        static_assert(std::is_integral<T>::value, "T 必须是整型");
    public:
        explicit RequiresInteger(T v) : _v(v) {}
        T get() const { return _v; }
    private:
        T _v;
    };

    template <class T>
    T safe_divide(T a, T b) {
        static_assert(std::is_arithmetic_v<T>, "只能用于算术类型");
        return b == 0 ? T{} : a / b;
    }

    void demo() {
        RequiresInteger<int> ri(42);
        std::cout << "      RequiresInteger<int>(42).get() = " << ri.get() << "\n";
        // RequiresInteger<double> rd(1.5);   // 编译错误:
        //   error: static assertion failed: T 必须是整型

        std::cout << "      safe_divide(10, 3) = " << safe_divide(10, 3) << "\n";
        std::cout << "      safe_divide(10, 0) = " << safe_divide(10, 0) << "\n";
        std::cout << "      static_assert 把模板的隐式要求变成显式的、报错清晰的约束\n";
        std::cout << "      对比：不加 static_assert 时，错误会出现在模板内部几十行外\n";
    }
}

// ---------- 2. type_traits 全景 ----------
namespace traits_overview {
    struct Empty {};
    struct WithVirtual { virtual ~WithVirtual() = default; };

    template <class T>
    void report(const char* name) {
        std::cout << "        " << name << ":\n";
        std::cout << "          整型=" << std::is_integral_v<T>
                  << " 浮点=" << std::is_floating_point_v<T>
                  << " 指针=" << std::is_pointer_v<T>
                  << " 类=" << std::is_class_v<T> << "\n";
        std::cout << "          可默认构造=" << std::is_default_constructible_v<T>
                  << " 可拷贝=" << std::is_copy_constructible_v<T>
                  << " nothrow可移动=" << std::is_nothrow_move_constructible_v<T> << "\n";
        std::cout << "          多态=" << std::is_polymorphic_v<T>
                  << " 平凡=" << std::is_trivial_v<T>
                  << " 大小=" << sizeof(T) << "\n";
    }

    void demo() {
        report<int>("int");
        report<std::string>("std::string");
        report<WithVirtual>("WithVirtual");
    }
}

// ---------- 3. 类型变换 traits ----------
namespace transformations {
    void demo() {
        std::cout << "      remove_reference_t<int&>      是 int: "
                  << std::is_same_v<std::remove_reference_t<int&>, int> << "\n";
        std::cout << "      remove_const_t<const int>     是 int: "
                  << std::is_same_v<std::remove_const_t<const int>, int> << "\n";
        std::cout << "      remove_pointer_t<int*>        是 int: "
                  << std::is_same_v<std::remove_pointer_t<int*>, int> << "\n";
        std::cout << "      decay_t<const int&>           是 int: "
                  << std::is_same_v<std::decay_t<const int&>, int> << "\n";
        std::cout << "      conditional_t<true,int,double> 是 int: "
                  << std::is_same_v<std::conditional_t<true, int, double>, int> << "\n";
        std::cout << "      common_type_t<int,double>     是 double: "
                  << std::is_same_v<std::common_type_t<int, double>, double> << "\n";
        std::cout << "      decay 最常用：去掉引用和 cv 限定，相当于按值传参的类型\n";
    }
}

// ---------- 4. SFINAE：替换失败不是错误 ----------
namespace sfinae {
    // 优先级标签分发（priority tag）：这是标准库常用的 SFINAE 组织方式。
    // 注意：不能简单地用 `f(const T&, ...)` 做兜底 —— 省略号只在
    // "前面的参数需要转换"时才降低优先级；这里两个候选的第一个参数都精确匹配，
    // 结果是二义性错误：
    //   error: call of overloaded 'get_size(std::vector<int>&)' is ambiguous
    template <int N> struct Priority : Priority<N - 1> {};
    template <> struct Priority<0> {};

    // 高优先级：T 有 size() 时才可用
    template <class T>
    auto get_size_impl(const T& t, Priority<1>) -> decltype(t.size()) {
        std::cout << "        用 t.size(): ";
        return t.size();
    }

    // 低优先级：兜底
    template <class T>
    std::size_t get_size_impl(const T&, Priority<0>) {
        std::cout << "        没有 size()，返回 1: ";
        return 1;
    }

    template <class T>
    std::size_t get_size(const T& t) { return get_size_impl(t, Priority<1>{}); }

    // 检测某个成员是否存在的惯用法
    template <class T, class = void>
    struct has_push_back : std::false_type {};

    template <class T>
    struct has_push_back<T, std::void_t<decltype(std::declval<T&>().push_back(
                                            std::declval<typename T::value_type>()))>>
        : std::true_type {};

    void demo() {
        std::vector<int> v{1, 2, 3};
        std::cout << "      get_size(vector) = " << get_size(v) << "\n";
        std::cout << "      get_size(int)    = " << get_size(42) << "\n";

        // 注意选对照组：std::string 也有 push_back，所以它不是好的反例
        std::cout << "      has_push_back<vector<int>>  = "
                  << has_push_back<std::vector<int>>::value << "\n";
        std::cout << "      has_push_back<std::string>  = "
                  << has_push_back<std::string>::value << "（string 确实有 push_back）\n";
        std::cout << "      has_push_back<std::set<int>>= "
                  << has_push_back<std::set<int>>::value << "（set 没有，这才是反例）\n";
        std::cout << "      SFINAE：模板替换失败时不报错，只是把该候选从重载集里剔除\n";
    }
}

// ---------- 5. enable_if：条件启用模板 ----------
namespace enable_if_demo {
    // 版本一：整型用位运算
    template <class T>
    std::enable_if_t<std::is_integral_v<T>, T>
    half(T value) {
        std::cout << "        整型版本（位移）: ";
        return value >> 1;
    }

    // 版本二：浮点用除法
    template <class T>
    std::enable_if_t<std::is_floating_point_v<T>, T>
    half(T value) {
        std::cout << "        浮点版本（除法）: ";
        return value / 2;
    }

    // enable_if 放在模板参数里（更常见的写法）
    template <class T, class = std::enable_if_t<std::is_pointer_v<T>>>
    void only_pointers(T p) {
        std::cout << "        只接受指针: " << (p ? "非空" : "空") << "\n";
    }

    void demo() {
        std::cout << "      half(10)   = " << half(10) << "\n";
        std::cout << "      half(10.0) = " << half(10.0) << "\n";
        int x = 1;
        only_pointers(&x);
        // only_pointers(x);        // 编译错误：没有匹配的重载
        std::cout << "      enable_if 的三种位置：返回类型、模板参数默认值、函数参数默认值\n";
    }
}

// ---------- 6. if constexpr [C++17]：取代大部分 SFINAE ----------
namespace if_constexpr_demo {
    // 用 if constexpr 重写上面的 half，一个函数搞定
    template <class T>
    T half(T value) {
        if constexpr (std::is_integral_v<T>) {
            std::cout << "        整型分支: ";
            return value >> 1;              // 只有整型时这行才被编译
        } else if constexpr (std::is_floating_point_v<T>) {
            std::cout << "        浮点分支: ";
            return value / 2;
        } else {
            static_assert(std::is_arithmetic_v<T>, "只支持算术类型");
            return T{};
        }
    }

    // 处理任意类型的通用打印
    template <class T>
    void smart_print(const T& value) {
        if constexpr (std::is_pointer_v<T>) {
            std::cout << "        指针，指向: " << (value ? "有效" : "空") << "\n";
        } else if constexpr (std::is_same_v<T, std::string>) {
            std::cout << "        字符串，长度 " << value.size() << ": " << value << "\n";
        } else if constexpr (std::is_arithmetic_v<T>) {
            std::cout << "        数值: " << value << "\n";
        } else {
            std::cout << "        其他类型\n";
        }
    }

    // 配合可变参数：递归的编译期分支
    template <class... Args>
    void print_all(const Args&... args) {
        std::cout << "        共 " << sizeof...(args) << " 个参数\n";
        if constexpr (sizeof...(args) > 0) {
            ((std::cout << "          " << args << "\n"), ...);
        }
    }

    void demo() {
        std::cout << "      half(10)   = " << half(10) << "\n";
        std::cout << "      half(10.0) = " << half(10.0) << "\n";

        int x = 42;
        smart_print(&x);
        smart_print(std::string("hello"));
        smart_print(3.14);

        print_all(1, 2.5, "three");
        print_all();
        std::cout << "      if constexpr 让不成立的分支【不被实例化】，\n";
        std::cout << "      所以同一个函数里可以写对不同类型都非法的代码\n";
    }
}

// ---------- 7. 实战：根据类型选择最优实现 ----------
namespace practical {
    // 对随机访问迭代器用 O(1) 的距离计算，否则 O(n) 遍历
    template <class Iter>
    std::size_t my_distance(Iter first, Iter last) {
        using Category = typename std::iterator_traits<Iter>::iterator_category;
        if constexpr (std::is_base_of_v<std::random_access_iterator_tag, Category>) {
            std::cout << "        随机访问迭代器，O(1): ";
            return static_cast<std::size_t>(last - first);
        } else {
            std::cout << "        前向迭代器，O(n) 遍历: ";
            std::size_t n = 0;
            while (first != last) { ++first; ++n; }
            return n;
        }
    }

    void demo() {
        std::vector<int> v{1, 2, 3, 4, 5};
        std::list<int> l{1, 2, 3};
        std::cout << "      vector: " << my_distance(v.begin(), v.end()) << "\n";
        std::cout << "      list:   " << my_distance(l.begin(), l.end()) << "\n";
        std::cout << "      这正是 std::distance 的实现思路（它用标签分发）\n";
    }
}

int main() {
    std::cout << "--- 1. static_assert ---\n";          static_assert_demo::demo();
    std::cout << "--- 2. type_traits 全景 ---\n";       traits_overview::demo();
    std::cout << "--- 3. 类型变换 ---\n";               transformations::demo();
    std::cout << "--- 4. SFINAE ---\n";                 sfinae::demo();
    std::cout << "--- 5. enable_if ---\n";              enable_if_demo::demo();
    std::cout << "--- 6. if constexpr [C++17] ---\n";   if_constexpr_demo::demo();
    std::cout << "--- 7. 实战：选择最优实现 ---\n";     practical::demo();
}
