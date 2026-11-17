// std=c++17
// 对应章节: blog/part3_class/03-04_copy_control.md
// 主题: 深浅拷贝、double free、=default/=delete、五大函数的生成规则

#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ---------- 1. 浅拷贝的灾难 ----------
class ShallowCopy {
public:
    explicit ShallowCopy(const char* s)
        : _data(new char[std::strlen(s) + 1]()) {
        std::strcpy(_data, s);
        std::cout << "      构造, _data=" << static_cast<void*>(_data) << "\n";
    }
    ~ShallowCopy() {
        std::cout << "      析构, 即将 delete " << static_cast<void*>(_data) << "\n";
        delete[] _data;
    }
    // 没有定义拷贝构造函数 -> 编译器生成的版本只拷贝指针值（浅拷贝）
    const char* c_str() const { return _data; }
private:
    char* _data;
};

void shallow_copy_problem() {
    std::cout << "      两个对象的 _data 指向同一块内存，析构两次 = double free\n";
    std::cout << "      （这段代码故意不执行，否则程序会崩溃）\n";
    // ShallowCopy a("hello");
    // ShallowCopy b = a;     // 浅拷贝！a._data == b._data
    // 离开作用域时 double free -> 崩溃或堆损坏
}

// ---------- 2. 深拷贝：正确的做法 ----------
class DeepCopy {
public:
    explicit DeepCopy(const char* s)
        : _data(new char[std::strlen(s) + 1]()) {
        std::strcpy(_data, s);
    }

    // 拷贝构造：分配自己的内存
    DeepCopy(const DeepCopy& rhs)
        : _data(new char[std::strlen(rhs._data) + 1]()) {
        std::strcpy(_data, rhs._data);
        std::cout << "      深拷贝构造, 新地址=" << static_cast<void*>(_data) << "\n";
    }

    // 拷贝赋值：注意自赋值检查和异常安全
    DeepCopy& operator=(const DeepCopy& rhs) {
        std::cout << "      深拷贝赋值\n";
        if (this != &rhs) {                    // 1. 自赋值检查
            char* tmp = new char[std::strlen(rhs._data) + 1]();  // 2. 先分配
            std::strcpy(tmp, rhs._data);
            delete[] _data;                    // 3. 再释放旧的
            _data = tmp;                       // 4. 最后接管
        }
        return *this;                          // 5. 返回 *this 支持链式赋值
    }

    ~DeepCopy() { delete[] _data; }

    const char* c_str() const { return _data; }
private:
    char* _data;
};

void deep_copy() {
    DeepCopy a("hello");
    DeepCopy b = a;              // 深拷贝构造
    DeepCopy c("other");
    c = a;                        // 深拷贝赋值
    std::cout << "      a=" << a.c_str() << " b=" << b.c_str()
              << " c=" << c.c_str() << " 地址互不相同\n";

    a = a;                        // 自赋值，有检查所以安全
    std::cout << "      自赋值后 a=" << a.c_str() << "（没有变成垃圾）\n";
}

// ---------- 3. 不做自赋值检查的后果 ----------
class NoSelfCheck {
public:
    explicit NoSelfCheck(const char* s) : _data(new char[std::strlen(s) + 1]()) {
        std::strcpy(_data, s);
    }
    NoSelfCheck& operator=(const NoSelfCheck& rhs) {
        delete[] _data;                        // 先删！如果 this == &rhs，
        _data = new char[std::strlen(rhs._data) + 1]();  // rhs._data 已经是野指针
        std::strcpy(_data, rhs._data);
        return *this;
    }
    ~NoSelfCheck() { delete[] _data; }
private:
    char* _data;
};

void self_assignment() {
    std::cout << "      先 delete 再 new 的写法，遇到 a = a 会读已释放的内存\n";
    std::cout << "      两种解法：1) 自赋值检查  2) copy-and-swap（根本上避免）\n";
}

// ---------- 4. copy-and-swap：一个函数搞定拷贝赋值和移动赋值 ----------
class CopyAndSwap {
public:
    explicit CopyAndSwap(const char* s) : _data(new char[std::strlen(s) + 1]()) {
        std::strcpy(_data, s);
    }
    CopyAndSwap(const CopyAndSwap& rhs) : _data(new char[std::strlen(rhs._data) + 1]()) {
        std::strcpy(_data, rhs._data);
    }
    CopyAndSwap(CopyAndSwap&& rhs) noexcept : _data(rhs._data) { rhs._data = nullptr; }

    // 按值传参：传左值时拷贝构造，传右值时移动构造
    // 然后交换，旧数据随参数一起析构
    CopyAndSwap& operator=(CopyAndSwap rhs) noexcept {
        swap(*this, rhs);
        return *this;
    }

    ~CopyAndSwap() { delete[] _data; }

    friend void swap(CopyAndSwap& a, CopyAndSwap& b) noexcept {
        std::swap(a._data, b._data);
    }
    const char* c_str() const { return _data ? _data : "(moved)"; }
private:
    char* _data;
};

void copy_and_swap() {
    CopyAndSwap a("aaa"), b("bbb");
    a = b;                              // 拷贝赋值（参数被拷贝构造）
    std::cout << "      a = b 后 a=" << a.c_str() << "\n";
    a = CopyAndSwap("ccc");             // 移动赋值（参数被移动构造）
    std::cout << "      a = 临时对象 后 a=" << a.c_str() << "\n";
    a = a;                              // 自赋值天然安全（参数是独立副本）
    std::cout << "      自赋值后 a=" << a.c_str() << "（天然安全）\n";
}

// ---------- 5. = default 和 = delete [C++11] ----------
class UseDefault {
public:
    UseDefault() = default;                            // 要求编译器生成
    UseDefault(int v) : _v(v) {}
    UseDefault(const UseDefault&) = default;
    UseDefault& operator=(const UseDefault&) = default;
    UseDefault(UseDefault&&) = default;
    UseDefault& operator=(UseDefault&&) = default;
    ~UseDefault() = default;
private:
    int _v = 0;
};

class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;              // 明确禁止拷贝
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;                   // 但允许移动
    NonCopyable& operator=(NonCopyable&&) = default;
};

// = delete 还能禁止特定的重载/隐式转换
class OnlyInt {
public:
    void f(int) { std::cout << "      f(int) 可用\n"; }
    void f(double) = delete;        // 明确禁止传 double
    void f(char) = delete;
};

void default_and_delete() {
    UseDefault a(1);
    UseDefault b = a;
    std::cout << "      = default 让编译器生成，同时保留了其他构造函数\n";

    NonCopyable n1;
    // NonCopyable n2 = n1;         // 编译错误: use of deleted function
    NonCopyable n3 = std::move(n1);  // ok：移动是允许的
    std::cout << "      = delete 的错误信息比设为 private 更清楚\n";

    OnlyInt o;
    o.f(42);
    // o.f(3.14);                   // 编译错误: use of deleted function
    std::cout << "      = delete 可以精确禁止某个重载，防止意外的隐式转换\n";
    (void)b; (void)n3;
}

// ---------- 6. 五大函数的生成规则（最容易踩的坑）----------
class OnlyDtor {
public:
    ~OnlyDtor() {}                      // 只声明了析构函数
    std::vector<int> _data{1, 2, 3};
};

class FullySpecified {
public:
    ~FullySpecified() {}
    FullySpecified() = default;
    FullySpecified(const FullySpecified&) = default;
    FullySpecified& operator=(const FullySpecified&) = default;
    FullySpecified(FullySpecified&&) = default;
    FullySpecified& operator=(FullySpecified&&) = default;
    std::vector<int> _data{1, 2, 3};
};

class HasMoveOnly {
public:
    HasMoveOnly() = default;
    HasMoveOnly(HasMoveOnly&&) = default;   // 声明了移动构造
    std::vector<int> _data;
};

void generation_rules() {
    std::cout << "      只声明析构函数的类:\n";
    std::cout << "        可移动构造: " << std::is_move_constructible<OnlyDtor>::value
              << "（看起来是 true，但实际走的是拷贝！）\n";
    std::cout << "        nothrow 可移动构造: "
              << std::is_nothrow_move_constructible<OnlyDtor>::value
              << "  <- 0 说明不是真正的移动\n";

    std::cout << "      显式声明全部五个的类:\n";
    std::cout << "        nothrow 可移动构造: "
              << std::is_nothrow_move_constructible<FullySpecified>::value
              << "  <- 1，移动能力恢复了\n";

    std::cout << "      声明了移动构造的类:\n";
    std::cout << "        可拷贝构造: " << std::is_copy_constructible<HasMoveOnly>::value
              << "  <- 0，拷贝被隐式删除了\n";

    std::cout << "\n      规则总结:\n";
    std::cout << "        声明析构/拷贝 -> 移动不再自动生成（退化为拷贝）\n";
    std::cout << "        声明移动      -> 拷贝被定义为删除\n";
}

// ---------- 7. Rule of Zero：什么都不写 ----------
class RuleOfZero {
public:
    RuleOfZero(std::string name, std::vector<int> data)
        : _name(std::move(name)), _data(std::move(data)) {}
    // 五个特殊函数一个都不写，编译器生成的全部正确
private:
    std::string _name;
    std::vector<int> _data;
};

void rule_of_zero() {
    std::cout << "      可拷贝: " << std::is_copy_constructible<RuleOfZero>::value
              << " 可移动: " << std::is_move_constructible<RuleOfZero>::value
              << " nothrow可移动: " << std::is_nothrow_move_constructible<RuleOfZero>::value
              << "\n";
    std::cout << "      成员都是 RAII 类型时，编译器生成的五个函数全部正确\n";
}

int main() {
    std::cout << "--- 1. 浅拷贝的问题 ---\n";       shallow_copy_problem();
    std::cout << "--- 2. 深拷贝 ---\n";             deep_copy();
    std::cout << "--- 3. 自赋值 ---\n";             self_assignment();
    std::cout << "--- 4. copy-and-swap ---\n";      copy_and_swap();
    std::cout << "--- 5. default/delete ---\n";     default_and_delete();
    std::cout << "--- 6. 生成规则 ---\n";           generation_rules();
    std::cout << "--- 7. Rule of Zero ---\n";       rule_of_zero();
}
