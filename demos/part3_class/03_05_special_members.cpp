// std=c++17
// 对应章节: blog/part3_class/03-05_special_members.md
// 主题: const/引用/类对象/static 成员，静态成员函数，C++17 inline 静态成员

#include <iostream>
#include <string>
#include <vector>

// ---------- 1. 四种特殊数据成员 ----------
class Inner {
public:
    explicit Inner(int v) : _v(v) { std::cout << "        Inner(" << v << ")\n"; }
    int value() const { return _v; }
private:
    int _v;
};

class SpecialMembers {
public:
    SpecialMembers(int cv, int& ref, int inner_v)
        : _const_member(cv)      // const 成员：必须在初始化列表
        , _ref_member(ref)       // 引用成员：必须在初始化列表
        , _obj_member(inner_v)   // 类对象成员：必须在初始化列表（无默认构造时）
    {
        ++_instance_count;       // static 成员：在构造函数体里改没问题
    }

    void print() const {
        std::cout << "      const=" << _const_member
                  << " ref=" << _ref_member
                  << " obj=" << _obj_member.value()
                  << " count=" << _instance_count << "\n";
    }

    static int instance_count() { return _instance_count; }

private:
    const int _const_member;
    int& _ref_member;
    Inner _obj_member;
    static int _instance_count;   // 声明，不是定义
};

// 静态数据成员必须在类外定义和初始化（C++17 之前）
int SpecialMembers::_instance_count = 0;

void special_data_members() {
    int x = 42;
    SpecialMembers a(1, x, 100);
    a.print();
    SpecialMembers b(2, x, 200);
    b.print();
    std::cout << "      通过类名访问静态成员: " << SpecialMembers::instance_count() << "\n";
}

// ---------- 2. static 数据成员：所有对象共享一份 ----------
class Computer {
public:
    Computer(std::string brand, double price)
        : _brand(std::move(brand)), _price(price) {
        _totalPrice += _price;      // 累加到共享的总价
    }

    void print() const {
        std::cout << "      品牌:" << _brand << " 价格:" << _price
                  << " 总价:" << _totalPrice << "\n";
    }

    // 静态成员函数：没有 this，只能访问静态成员
    static void printTotalPrice() {
        std::cout << "      [静态函数] 总价:" << _totalPrice << "\n";
        // std::cout << _brand;     // 编译错误：没有 this，无法访问非静态成员
    }

private:
    std::string _brand;
    double _price;
    static double _totalPrice;
};
double Computer::_totalPrice = 0;

void static_members() {
    Computer pc1("Huawei MateBook14", 5699);
    pc1.print();
    Computer pc2("ThinkPad X1", 9999);
    pc2.print();

    Computer::printTotalPrice();     // 推荐：通过类名调用
    pc1.printTotalPrice();           // 也可以，但不推荐（看起来像在操作 pc1）

    std::cout << "      静态成员存在全局/静态区，不占对象空间\n";
    std::cout << "      sizeof(Computer) = " << sizeof(Computer)
              << "（只含 string + double，不含 static）\n";
}

// ---------- 3. C++17 inline 静态成员：不用在类外定义 ----------
class ModernStatic {
public:
    static inline int counter = 0;                       // [C++17]
    static inline const std::string kName = "modern";     // [C++17]
    static constexpr int kMax = 100;                      // constexpr 隐式 inline

    // C++17 之前必须这样：
    //   类内: static int counter;
    //   类外: int ModernStatic::counter = 0;
};

void inline_static() {
    ++ModernStatic::counter;
    ++ModernStatic::counter;
    std::cout << "      counter=" << ModernStatic::counter
              << " kName=" << ModernStatic::kName
              << " kMax=" << ModernStatic::kMax << "\n";
    std::cout << "      [C++17] static inline 成员可以在类内直接初始化\n";
}

// ---------- 4. static constexpr 成员用作编译期常量 ----------
template <std::size_t N>
class FixedBuffer {
public:
    static constexpr std::size_t kCapacity = N;
    std::size_t size() const { return _used; }
private:
    char _data[N]{};
    std::size_t _used = 0;
};

void constexpr_static() {
    FixedBuffer<64> buf;
    static_assert(FixedBuffer<64>::kCapacity == 64, "编译期可用");
    int arr[FixedBuffer<64>::kCapacity]{};      // 可以作数组大小
    std::cout << "      kCapacity=" << FixedBuffer<64>::kCapacity
              << " sizeof(arr)=" << sizeof(arr) << "\n";
    (void)buf;
}

// ---------- 5. 静态成员的典型用途 ----------
// 用途一：实例计数
class Counted {
public:
    Counted()  { ++_alive; ++_total; }
    ~Counted() { --_alive; }
    static int alive() { return _alive; }
    static int total() { return _total; }
private:
    static inline int _alive = 0;      // [C++17]
    static inline int _total = 0;
};

// 用途二：共享的常量表
class HttpStatus {
public:
    static const char* text(int code) {
        switch (code) {
            case 200: return kOk;
            case 404: return kNotFound;
            case 500: return kServerError;
            default:  return kUnknown;
        }
    }
private:
    static inline const char* kOk = "OK";
    static inline const char* kNotFound = "Not Found";
    static inline const char* kServerError = "Internal Server Error";
    static inline const char* kUnknown = "Unknown";
};

// 用途三：工厂 / 单例（见 03-07）
void static_usages() {
    std::cout << "      用途一：实例计数\n";
    {
        Counted a, b, c;
        std::cout << "        存活=" << Counted::alive()
                  << " 累计=" << Counted::total() << "\n";
    }
    std::cout << "        作用域结束后 存活=" << Counted::alive()
              << " 累计=" << Counted::total() << "\n";

    std::cout << "      用途二：共享常量表\n";
    std::cout << "        HttpStatus::text(404) = " << HttpStatus::text(404) << "\n";
}

// ---------- 6. 静态成员函数不能是 const/virtual ----------
class StaticLimits {
public:
    static void f() {}
    // static void g() const {}     // 编译错误：静态成员函数不能是 const
    //                                  （const 修饰的是 this，而它没有 this）
    // static virtual void h() {}   // 编译错误：静态成员函数不能是虚函数
    //                                  （虚调用需要通过对象找虚表）
};

void static_limits() {
    std::cout << "      静态成员函数不能是 const（没有 this 可修饰）\n";
    std::cout << "      静态成员函数不能是 virtual（虚调用需要对象）\n";
    StaticLimits::f();
}

// ---------- 7. 嵌套类型与成员类型别名 ----------
class Container {
public:
    using value_type = int;                   // 成员类型别名
    using size_type = std::size_t;

    class Iterator {                           // 嵌套类
    public:
        explicit Iterator(value_type* p) : _p(p) {}
        value_type& operator*() const { return *_p; }
    private:
        value_type* _p;
    };

    enum class Mode { Fast, Safe };            // 嵌套枚举

    void show() const {
        std::cout << "      嵌套类型要用 Container::Iterator 访问\n";
    }
};

void nested_types() {
    Container c;
    c.show();
    Container::value_type v = 42;
    Container::Iterator it(&v);
    std::cout << "      *it = " << *it << "\n";
    Container::Mode m = Container::Mode::Fast;
    std::cout << "      Mode::Fast = " << static_cast<int>(m) << "\n";
    std::cout << "      标准容器都提供 value_type/size_type 等别名，供泛型代码使用\n";
}

int main() {
    std::cout << "--- 1. 四种特殊数据成员 ---\n";  special_data_members();
    std::cout << "--- 2. static 数据成员 ---\n";   static_members();
    std::cout << "--- 3. inline static [C++17] ---\n"; inline_static();
    std::cout << "--- 4. static constexpr ---\n";  constexpr_static();
    std::cout << "--- 5. 静态成员的用途 ---\n";    static_usages();
    std::cout << "--- 6. 静态成员函数的限制 ---\n"; static_limits();
    std::cout << "--- 7. 嵌套类型 ---\n";          nested_types();
}
