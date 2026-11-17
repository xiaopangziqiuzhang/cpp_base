// std=c++17
// 对应章节: blog/part3_class/03-08_pimpl.md
// 主题: Pimpl 惯用法 —— 编译期解耦、不完整类型问题
//
// 为了保持 demo 单文件可编译，这里用注释标明哪些代码"应该在头文件里"、
// 哪些"应该在源文件里"。真实工程的文件划分见章节正文。

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ======================================================================
// ===== 对比：不用 Pimpl 的类（头文件暴露全部实现细节）=====
// ======================================================================

// ---- 假设这些是重量级的第三方头文件 ----
namespace heavy_lib {
    struct BigConfig {
        std::vector<std::string> options;
        int flags = 0;
    };
    struct Connection {
        std::string endpoint;
    };
}

// ---- WithoutPimpl.h ----
// 问题：头文件必须 #include 全部依赖，而且暴露了所有私有成员
class WithoutPimpl {
public:
    explicit WithoutPimpl(std::string name);
    void work() const;
private:
    std::string _name;                      // 用户看得到
    heavy_lib::BigConfig _config;           // 用户必须能看到 BigConfig 的定义
    heavy_lib::Connection _conn;            // 以及 Connection 的定义
    std::vector<int> _cache;
    // 加一个成员 -> 对象大小变化 -> 所有包含此头文件的 .cpp 必须重新编译
};

WithoutPimpl::WithoutPimpl(std::string name) : _name(std::move(name)) {}
void WithoutPimpl::work() const {
    std::cout << "      WithoutPimpl(" << _name << ")::work()\n";
}

// ======================================================================
// ===== Pimpl 版本 =====
// ======================================================================

// ---- WithPimpl.h（用户看到的全部内容）----
class WithPimpl {
public:
    explicit WithPimpl(std::string name);
    ~WithPimpl();                            // 必须声明！见正文

    // 移动：声明在头文件，定义在源文件
    WithPimpl(WithPimpl&&) noexcept;
    WithPimpl& operator=(WithPimpl&&) noexcept;

    // 拷贝：需要深拷贝 Impl，所以也要手写
    WithPimpl(const WithPimpl&);
    WithPimpl& operator=(const WithPimpl&);

    void work() const;
    void add_option(const std::string& opt);
    std::size_t option_count() const;

private:
    class Impl;                              // 仅前向声明
    std::unique_ptr<Impl> _impl;             // 唯一的数据成员
};

// ---- WithPimpl.cpp（用户看不到）----
class WithPimpl::Impl {
public:
    explicit Impl(std::string name) : _name(std::move(name)) {
        std::cout << "        Impl(" << _name << ") 构造\n";
    }
    ~Impl() { std::cout << "        Impl(" << _name << ") 析构\n"; }

    void work() const {
        std::cout << "      WithPimpl(" << _name << ")::work(), options="
                  << _config.options.size() << "\n";
    }
    void add_option(const std::string& opt) { _config.options.push_back(opt); }
    std::size_t option_count() const { return _config.options.size(); }

private:
    std::string _name;
    heavy_lib::BigConfig _config;            // 这些依赖只有 .cpp 需要
    heavy_lib::Connection _conn;
    std::vector<int> _cache;
};

WithPimpl::WithPimpl(std::string name)
    : _impl(std::make_unique<Impl>(std::move(name))) {}

// 析构函数必须在这里定义：此时 Impl 已是完整类型
WithPimpl::~WithPimpl() = default;

WithPimpl::WithPimpl(WithPimpl&&) noexcept = default;
WithPimpl& WithPimpl::operator=(WithPimpl&&) noexcept = default;

// 拷贝构造：深拷贝 Impl
WithPimpl::WithPimpl(const WithPimpl& rhs)
    : _impl(rhs._impl ? std::make_unique<Impl>(*rhs._impl) : nullptr) {}

WithPimpl& WithPimpl::operator=(const WithPimpl& rhs) {
    if (this != &rhs) {
        _impl = rhs._impl ? std::make_unique<Impl>(*rhs._impl) : nullptr;
    }
    return *this;
}

// 转发函数（每个公开接口都要写一行）
void WithPimpl::work() const { _impl->work(); }
void WithPimpl::add_option(const std::string& opt) { _impl->add_option(opt); }
std::size_t WithPimpl::option_count() const { return _impl->option_count(); }

// ======================================================================
// ===== 演示不完整类型问题 =====
// ======================================================================
class BadPimpl {
public:
    BadPimpl();
    // ~BadPimpl();      // 如果不声明析构函数，编译器会在【头文件】里生成它，
    //                    // 而此时 Impl 还是不完整类型 ->
    //                    // error: invalid application of 'sizeof' to incomplete type
    //                    // error: possible problem detected in invocation of delete
    void work() const;
private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};

class BadPimpl::Impl {
public:
    void work() const { std::cout << "      BadPimpl::Impl::work()\n"; }
};

BadPimpl::BadPimpl() : _impl(std::make_unique<Impl>()) {}
void BadPimpl::work() const { _impl->work(); }
// 因为本 demo 是单文件，Impl 的定义在同一个翻译单元里可见，
// 所以这里不写析构函数也能编过。
// 真实工程中 Impl 定义在 .cpp，头文件里不声明析构函数就一定编译失败。

// ======================================================================

void comparison() {
    std::cout << "    不用 Pimpl:\n";
    WithoutPimpl a("plain");
    a.work();
    std::cout << "      sizeof(WithoutPimpl) = " << sizeof(WithoutPimpl)
              << " 字节（随成员增加而变化）\n";

    std::cout << "    用 Pimpl:\n";
    WithPimpl b("pimpl");
    b.add_option("verbose");
    b.add_option("debug");
    b.work();
    std::cout << "      sizeof(WithPimpl) = " << sizeof(WithPimpl)
              << " 字节（永远是一个指针大小，与实现无关）\n";
}

void copy_and_move() {
    std::cout << "    拷贝（深拷贝 Impl）:\n";
    WithPimpl a("original");
    a.add_option("opt1");
    WithPimpl b = a;                     // 拷贝构造
    b.add_option("opt2");
    std::cout << "      a.option_count()=" << a.option_count()
              << " b.option_count()=" << b.option_count()
              << "（互不影响，说明是深拷贝）\n";

    std::cout << "    移动（只转移指针）:\n";
    WithPimpl c = std::move(a);
    std::cout << "      移动后 c.option_count()=" << c.option_count() << "\n";
}

void incomplete_type_rules() {
    std::cout << "      必须在【源文件】里定义的函数（此时 Impl 才是完整类型）:\n";
    std::cout << "        ~WithPimpl()           —— 要 delete Impl\n";
    std::cout << "        移动构造 / 移动赋值    —— 隐含要销毁旧的 Impl\n";
    std::cout << "        拷贝构造 / 拷贝赋值    —— 要构造 Impl\n";
    std::cout << "      做法：头文件只声明，源文件用 = default 或手写实现\n";
    BadPimpl bp;
    bp.work();
}

int main() {
    std::cout << "--- 1. 有无 Pimpl 的对比 ---\n";   comparison();
    std::cout << "--- 2. 拷贝与移动 ---\n";          copy_and_move();
    std::cout << "--- 3. 不完整类型规则 ---\n";      incomplete_type_rules();
}
