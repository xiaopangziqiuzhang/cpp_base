// std=c++17
// 对应章节: blog/part2_memory/02-03_move_semantics.md
// 主题: 值类别、移动构造/赋值、std::move、Rule of Five

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// ---------- 一个会报告自己每一步操作的字符串类 ----------
class MyString {
public:
    // 默认构造
    // 注意初始化列表的顺序必须与成员声明顺序一致（_size 在前，_data 在后），
    // 否则 GCC 会给出 -Wreorder 警告。成员的实际初始化顺序由声明顺序决定，
    // 不由初始化列表的书写顺序决定——详见 03-02。
    MyString() : _size(0), _data(new char[1]{'\0'}) {
        std::cout << "    默认构造\n";
    }

    // 普通构造
    MyString(const char* s)
        : _size(std::strlen(s)), _data(new char[_size + 1]) {
        std::strcpy(_data, s);
        std::cout << "    构造 \"" << _data << "\"\n";
    }

    // 拷贝构造：深拷贝，开销 = 分配 + 复制
    MyString(const MyString& rhs)
        : _size(rhs._size), _data(new char[rhs._size + 1]) {
        std::strcpy(_data, rhs._data);
        std::cout << "    拷贝构造 \"" << _data << "\"（分配 " << _size + 1 << " 字节）\n";
    }

    // 移动构造：偷走资源，开销 = 几次指针赋值
    MyString(MyString&& rhs) noexcept
        : _size(rhs._size), _data(rhs._data) {
        rhs._data = nullptr;      // 关键：让源对象放弃所有权
        rhs._size = 0;
        std::cout << "    移动构造 \"" << (_data ? _data : "") << "\"（只改指针）\n";
    }

    // 拷贝赋值
    MyString& operator=(const MyString& rhs) {
        std::cout << "    拷贝赋值\n";
        if (this != &rhs) {
            char* tmp = new char[rhs._size + 1];   // 先分配，失败也不破坏原状态
            std::strcpy(tmp, rhs._data);
            delete[] _data;
            _data = tmp;
            _size = rhs._size;
        }
        return *this;
    }

    // 移动赋值
    MyString& operator=(MyString&& rhs) noexcept {
        std::cout << "    移动赋值\n";
        if (this != &rhs) {
            delete[] _data;         // 先释放自己的
            _data = rhs._data;      // 接管对方的
            _size = rhs._size;
            rhs._data = nullptr;    // 对方放弃
            rhs._size = 0;
        }
        return *this;
    }

    ~MyString() {
        if (_data) std::cout << "    析构 \"" << _data << "\"\n";
        else       std::cout << "    析构（已被移动，无资源）\n";
        delete[] _data;
    }

    const char* c_str() const { return _data ? _data : ""; }
    std::size_t size() const { return _size; }

private:
    std::size_t _size = 0;
    char* _data = nullptr;
};

// ---------- 1. 没有移动语义时，返回大对象必须拷贝 ----------
MyString make_string_by_value() {
    MyString local("returned");
    return local;            // 返回局部对象
}

void why_move_matters() {
    std::cout << "  返回一个局部对象:\n";
    MyString s = make_string_by_value();
    std::cout << "  得到: " << s.c_str() << "\n";
    std::cout << "  （编译器做了 NRVO，连移动都省了。加 -fno-elide-constructors 可看到移动）\n";
}

// ---------- 2. 值类别：左值 / 右值 ----------
void value_categories() {
    MyString a("lvalue");
    MyString b("other");

    std::cout << "  用左值初始化 -> 拷贝构造:\n";
    MyString c = a;                       // a 是左值 -> 拷贝

    std::cout << "  用右值(临时对象)初始化 -> 移动构造:\n";
    MyString d = MyString("temporary");   // 临时对象是右值 -> 移动

    std::cout << "  用 std::move(左值) -> 移动构造:\n";
    MyString e = std::move(b);            // std::move 把左值转成右值
    std::cout << "  b 被移动后，c_str() = \"" << b.c_str() << "\"（空了）\n";

    std::cout << "  赋值运算符同理:\n";
    c = a;                                 // 拷贝赋值
    c = MyString("rvalue");                // 移动赋值
}

// ---------- 3. std::move 本身不移动任何东西 ----------
void what_move_really_does() {
    MyString a("original");

    std::cout << "  std::move 只是类型转换，这一行什么也没发生:\n";
    MyString&& ref = std::move(a);        // 只是绑定了一个右值引用
    std::cout << "    a 仍然是 \"" << a.c_str() << "\"\n";

    std::cout << "  真正的移动发生在用它构造/赋值的时候:\n";
    MyString b = std::move(ref);          // 这里才调用移动构造
    std::cout << "    现在 a 是 \"" << a.c_str() << "\"，b 是 \"" << b.c_str() << "\"\n";
}

// ---------- 4. 移动后对象的状态：有效但未指定 ----------
void moved_from_state() {
    std::string s = "hello world, this is long enough to heap-allocate";
    std::string t = std::move(s);

    std::cout << "  移动后源 string:\n";
    std::cout << "    size() = " << s.size() << "（标准只保证对象仍有效）\n";
    std::cout << "    可以安全地重新赋值:\n";
    s = "reassigned";                      // 合法
    std::cout << "    s = \"" << s << "\"\n";
    std::cout << "  （不要假设移动后是空字符串，标准未规定具体状态）\n";
}

// ---------- 5. 容器扩容时的移动 vs 拷贝 ----------
void container_growth() {
    std::cout << "  vector<MyString> 扩容时会移动元素:\n";
    std::vector<MyString> v;
    v.reserve(2);
    v.push_back(MyString("first"));       // 移动进容器
    std::cout << "    ---- 触发扩容 ----\n";
    v.push_back(MyString("second"));
    v.push_back(MyString("third"));       // 超过 reserve，触发扩容+搬迁
}

// ---------- 6. push_back vs emplace_back ----------
void push_vs_emplace() {
    std::vector<MyString> v;
    v.reserve(4);

    std::cout << "  push_back(MyString(\"a\")): 构造临时对象 + 移动\n";
    v.push_back(MyString("a"));

    std::cout << "  emplace_back(\"b\"): 直接在容器内构造，无临时对象\n";
    v.emplace_back("b");
}

// ---------- 7. noexcept 对容器性能的影响 ----------
// 注意：如果写 `WithoutNoexcept(WithoutNoexcept&&) = default;`，
// 编译器会根据成员推导出隐式的 noexcept（vector 的移动是 noexcept 的），
// 于是两个类都变成 nothrow 可移动，对比不出差别。
// 要制造真实对比，必须手写移动构造函数并且不加 noexcept。
class WithoutNoexcept {
public:
    WithoutNoexcept() : _data(100, 1) {}
    WithoutNoexcept(const WithoutNoexcept& rhs) : _data(rhs._data) {}
    WithoutNoexcept(WithoutNoexcept&& rhs) /* 故意不加 noexcept */
        : _data(std::move(rhs._data)) {}
private:
    std::vector<int> _data;
};

class WithNoexcept {
public:
    WithNoexcept() : _data(100, 1) {}
    WithNoexcept(const WithNoexcept& rhs) : _data(rhs._data) {}
    WithNoexcept(WithNoexcept&& rhs) noexcept : _data(std::move(rhs._data)) {}
private:
    std::vector<int> _data;
};

void noexcept_impact() {
    std::cout << "  是否 nothrow 可移动构造:\n";
    std::cout << "    WithoutNoexcept: "
              << std::is_nothrow_move_constructible<WithoutNoexcept>::value << "\n";
    std::cout << "    WithNoexcept:    "
              << std::is_nothrow_move_constructible<WithNoexcept>::value << "\n";

    constexpr int kN = 30000;
    auto bench = [](auto tag, auto make) {
        auto t1 = std::chrono::steady_clock::now();
        make();
        auto t2 = std::chrono::steady_clock::now();
        std::cout << "    " << tag << ": "
                  << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()
                  << " us\n";
    };

    std::cout << "  反复 push_back 触发扩容搬迁 " << kN << " 个元素:\n";
    bench("不带 noexcept（扩容时只能拷贝）", [&] {
        std::vector<WithoutNoexcept> v;
        for (int i = 0; i < kN; ++i) v.push_back(WithoutNoexcept{});
    });
    bench("带 noexcept  （扩容时可以移动）", [&] {
        std::vector<WithNoexcept> v;
        for (int i = 0; i < kN; ++i) v.push_back(WithNoexcept{});
    });
}

// ---------- 8. 移动语义带来的性能差异 ----------
void performance() {
    constexpr int kTimes = 200000;

    // 公平的对比方式：准备同样多的源对象，分别用拷贝和移动把它们塞进容器。
    // 两边都做 kTimes 次"把一个已存在的 string 放进 vector"，
    // 唯一的差别是拷贝还是移动。
    auto make_sources = [] {
        std::vector<std::string> v;
        v.reserve(kTimes);
        for (int i = 0; i < kTimes; ++i) v.emplace_back(1000, 'x');
        return v;
    };

    auto src1 = make_sources();
    std::vector<std::string> dst1;
    dst1.reserve(kTimes);
    auto t1 = std::chrono::steady_clock::now();
    for (auto& s : src1) dst1.push_back(s);              // 拷贝：分配 + memcpy
    auto t2 = std::chrono::steady_clock::now();

    auto src2 = make_sources();
    std::vector<std::string> dst2;
    dst2.reserve(kTimes);
    auto t3 = std::chrono::steady_clock::now();
    for (auto& s : src2) dst2.push_back(std::move(s));   // 移动：只改指针
    auto t4 = std::chrono::steady_clock::now();

    auto us = [](auto a, auto b) {
        return std::chrono::duration_cast<std::chrono::microseconds>(b - a).count();
    };
    std::cout << "  把 " << kTimes << " 个 1000 字节 string 放进 vector:\n";
    std::cout << "    push_back(s)            拷贝: " << us(t1, t2) << " us\n";
    std::cout << "    push_back(std::move(s)) 移动: " << us(t3, t4) << " us\n";
}

int main() {
    std::cout << "--- 1. 为什么需要移动 ---\n";      why_move_matters();
    std::cout << "--- 2. 值类别 ---\n";              value_categories();
    std::cout << "--- 3. std::move 的真相 ---\n";    what_move_really_does();
    std::cout << "--- 4. 移动后的状态 ---\n";        moved_from_state();
    std::cout << "--- 5. 容器扩容 ---\n";            container_growth();
    std::cout << "--- 6. push vs emplace ---\n";     push_vs_emplace();
    std::cout << "--- 7. noexcept 的影响 ---\n";     noexcept_impact();
    std::cout << "--- 8. 性能对比 ---\n";            performance();
}
