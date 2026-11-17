// std=c++17
// 对应章节: blog/part1_basics/01-08_exception.md
// 主题: throw/try/catch、标准异常体系、noexcept、RAII 与异常安全

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// ---------- 1. 错误码 vs 异常：没有异常时的困境 ----------
// 错误码风格：返回值被占用，错误可以被忽略
int divide_errcode(double x, double y, double* out) {
    if (y == 0) return -1;      // 调用方可能根本不检查
    *out = x / y;
    return 0;
}

// 异常风格：错误无法被静默忽略，返回值回归业务语义
double divide_throw(double x, double y) {
    if (y == 0) {
        throw std::invalid_argument("division by zero");
    }
    return x / y;
}

void errcode_vs_exception() {
    double result = 0;
    if (divide_errcode(10, 2, &result) == 0) {
        std::cout << "  错误码风格: 10/2 = " << result << "\n";
    }
    // 忽略返回值是合法的 —— 这就是问题
    divide_errcode(10, 0, &result);
    std::cout << "  忽略错误码后 result = " << result << "（用了未更新的值）\n";

    try {
        std::cout << "  异常风格: 10/2 = " << divide_throw(10, 2) << "\n";
        std::cout << divide_throw(10, 0);   // 不会执行到
    } catch (const std::invalid_argument& e) {
        std::cout << "  捕获 invalid_argument: " << e.what() << "\n";
    }
}

// ---------- 2. throw 表达式可以抛任何类型 ----------
void throw_various() {
    // 抛基本类型（能编译，但不推荐）
    try {
        throw 42;
    } catch (int e) {
        std::cout << "  catch(int): " << e << "\n";
    }

    try {
        throw std::string("a string");
    } catch (const std::string& e) {
        std::cout << "  catch(const string&): " << e << "\n";
    }

    // 推荐：抛标准异常体系中的类型
    try {
        throw std::runtime_error("推荐做法");
    } catch (const std::exception& e) {       // 按基类引用捕获
        std::cout << "  catch(const exception&): " << e.what() << "\n";
    }
}

// ---------- 3. catch 的匹配顺序：从上到下，第一个匹配的胜出 ----------
void catch_order() {
    try {
        throw std::out_of_range("越界了");
    }
    // 必须把派生类放在基类之前，否则派生类的 catch 永远不会被执行
    catch (const std::out_of_range& e) {
        std::cout << "  精确匹配 out_of_range: " << e.what() << "\n";
    }
    catch (const std::logic_error& e) {
        std::cout << "  不会到这里（基类）: " << e.what() << "\n";
    }
    catch (const std::exception& e) {
        std::cout << "  也不会到这里: " << e.what() << "\n";
    }
    catch (...) {                              // 兜底
        std::cout << "  catch-all\n";
    }
}

// ---------- 4. 标准异常体系 ----------
void standard_exceptions() {
    std::cout << "  std::exception\n"
                 "   ├── logic_error      (可以在运行前预防的逻辑错误)\n"
                 "   │    ├── invalid_argument\n"
                 "   │    ├── domain_error\n"
                 "   │    ├── length_error\n"
                 "   │    └── out_of_range\n"
                 "   ├── runtime_error    (只能在运行时检测到)\n"
                 "   │    ├── range_error\n"
                 "   │    ├── overflow_error\n"
                 "   │    └── underflow_error\n"
                 "   ├── bad_alloc        (new 失败)\n"
                 "   ├── bad_cast         (dynamic_cast 引用版失败)\n"
                 "   └── bad_typeid\n";

    // 实际例子：vector::at 越界
    std::vector<int> v{1, 2, 3};
    try {
        v.at(10);
    } catch (const std::out_of_range& e) {
        std::cout << "  v.at(10) 抛出: " << e.what() << "\n";
    }
}

// ---------- 5. 自定义异常：继承标准异常 ----------
class ConfigError : public std::runtime_error {
public:
    ConfigError(const std::string& key, const std::string& reason)
        : std::runtime_error("config key '" + key + "': " + reason)
        , _key(key) {}
    const std::string& key() const { return _key; }
private:
    std::string _key;
};

void custom_exception() {
    try {
        throw ConfigError("timeout", "必须是正整数");
    } catch (const ConfigError& e) {
        std::cout << "  ConfigError, key=" << e.key() << ", what=" << e.what() << "\n";
    }
}

// ---------- 6. 为什么异常安全需要 RAII ----------
void leaky_function() {
    int* raw = new int[100];     // 裸指针
    try {
        throw std::runtime_error("中途抛异常");
        // delete[] raw;         // 永远执行不到 -> 内存泄漏
    } catch (...) {
        delete[] raw;            // 只能手工在每个 catch 里释放
        std::cout << "  裸指针：必须在每个出口手工 delete\n";
        throw;                   // 重新抛出
    }
}

void raii_function() {
    auto guard = std::make_unique<int[]>(100);   // RAII
    throw std::runtime_error("中途抛异常");
    // 不需要写任何清理代码，栈展开时 guard 的析构函数自动释放
}

void why_raii() {
    try { leaky_function(); }
    catch (...) { std::cout << "  leaky_function 的异常已处理\n"; }

    try { raii_function(); }
    catch (const std::exception& e) {
        std::cout << "  raii_function 抛出 \"" << e.what()
                  << "\"，内存已被 unique_ptr 自动释放\n";
    }
}

// ---------- 7. RAII 演示栈展开时的析构顺序 ----------
class Tracer {
public:
    explicit Tracer(std::string name) : _name(std::move(name)) {
        std::cout << "    构造 " << _name << "\n";
    }
    // 注意：inner-1/inner-2 的析构发生在栈展开过程中；
    // outer 的析构是函数正常返回时的常规作用域退出，两者机制不同但都会被调用。
    ~Tracer() { std::cout << "    析构 " << _name << "\n"; }
private:
    std::string _name;
};

void inner_scope() {
    Tracer t1("inner-1");
    Tracer t2("inner-2");
    throw std::runtime_error("from inner_scope");
}

void stack_unwinding() {
    Tracer t0("outer");
    try {
        inner_scope();
    } catch (const std::exception& e) {
        std::cout << "    捕获: " << e.what() << "\n";
    }
}

// ---------- 8. noexcept [C++11] ----------
int never_throws() noexcept { return 42; }
int may_throw()             { return 42; }

void noexcept_demo() {
    std::cout << "  noexcept(never_throws()) = " << noexcept(never_throws()) << "\n";
    std::cout << "  noexcept(may_throw())    = " << noexcept(may_throw()) << "\n";

    // 条件 noexcept：常用于模板
    std::cout << "  是否 nothrow 可移动构造 (string): "
              << std::is_nothrow_move_constructible<std::string>::value << "\n";
}

// ---------- 9. 析构函数里不要抛异常 ----------
class BadDestructor {
public:
    ~BadDestructor() noexcept(false) {    // 要显式允许才能抛
        // 如果在栈展开过程中析构函数再抛异常 -> std::terminate
        std::cout << "  析构函数中如果抛异常，且正在栈展开，程序会 terminate\n";
    }
};

int main() {
    std::cout << "--- 1. 错误码 vs 异常 ---\n";   errcode_vs_exception();
    std::cout << "--- 2. throw 各种类型 ---\n";   throw_various();
    std::cout << "--- 3. catch 匹配顺序 ---\n";   catch_order();
    std::cout << "--- 4. 标准异常体系 ---\n";     standard_exceptions();
    std::cout << "--- 5. 自定义异常 ---\n";       custom_exception();
    std::cout << "--- 6. RAII 的必要性 ---\n";    why_raii();
    std::cout << "--- 7. 栈展开 ---\n";           stack_unwinding();
    std::cout << "--- 8. noexcept ---\n";         noexcept_demo();
    std::cout << "--- 9. 析构函数 ---\n";         { BadDestructor b; (void)b; }
}
