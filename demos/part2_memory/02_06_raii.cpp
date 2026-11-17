// std=c++17 flags=-pthread
// 对应章节: blog/part2_memory/02-06_raii.md
// 主题: RAII 原理、scope guard、标准库中的 RAII 类型

#include <cstdio>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

// ---------- 1. RAII 的最小实现 ----------
class FileHandle {
public:
    // 构造函数获取资源
    explicit FileHandle(const char* path, const char* mode)
        : _fp(std::fopen(path, mode)) {
        if (!_fp) throw std::runtime_error("无法打开文件");
        std::cout << "      获取文件句柄\n";
    }

    // 析构函数释放资源
    ~FileHandle() {
        if (_fp) {
            std::fclose(_fp);
            std::cout << "      释放文件句柄\n";
        }
    }

    // 禁止拷贝（句柄不能被复制）
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    // 允许移动（所有权转移）
    FileHandle(FileHandle&& rhs) noexcept : _fp(rhs._fp) { rhs._fp = nullptr; }
    FileHandle& operator=(FileHandle&& rhs) noexcept {
        if (this != &rhs) {
            if (_fp) std::fclose(_fp);
            _fp = rhs._fp;
            rhs._fp = nullptr;
        }
        return *this;
    }

    void write(const char* s) { std::fputs(s, _fp); }
    std::FILE* get() const { return _fp; }
private:
    std::FILE* _fp;
};

void raii_minimal() {
    try {
        FileHandle f("/tmp/raii_demo.txt", "w");
        f.write("RAII 保证了文件一定被关闭\n");
        throw std::runtime_error("中途抛异常");
    } catch (const std::exception& e) {
        std::cout << "      捕获: " << e.what() << "（但文件已经关闭了）\n";
    }
}

// ---------- 2. RAII 的四条要素 ----------
void raii_requirements() {
    std::cout << "      1. 构造函数获取资源，失败就抛异常\n";
    std::cout << "      2. 析构函数释放资源，且绝不抛异常\n";
    std::cout << "      3. 禁止拷贝，或实现深拷贝\n";
    std::cout << "      4. 通常支持移动（表达所有权转移）\n";
}

// ---------- 3. 标准库里的 RAII 类型 ----------
void stdlib_raii() {
    std::cout << "      std::string / vector      -> 内存\n";
    std::cout << "      std::unique_ptr/shared_ptr -> 任意资源\n";
    std::cout << "      std::fstream              -> 文件\n";
    std::cout << "      std::lock_guard/unique_lock -> 互斥量\n";
    std::cout << "      std::thread (C++20 jthread) -> 线程\n";

    {
        std::ofstream ofs("/tmp/raii_fstream.txt");
        ofs << "fstream 的析构函数会 close\n";
    }   // 自动关闭
    std::cout << "      fstream 已自动关闭\n";

    std::mutex mtx;
    {
        std::lock_guard<std::mutex> lk(mtx);     // 构造时 lock
        std::cout << "      lock_guard 持锁期间\n";
    }   // 析构时 unlock，即使中途抛异常也保证解锁
    std::cout << "      lock_guard 已自动解锁\n";
}

// ---------- 4. scope guard：为没有 RAII 封装的资源临时加一层 ----------
template <class F>
class ScopeGuard {
public:
    explicit ScopeGuard(F f) : _f(std::move(f)), _active(true) {}
    ~ScopeGuard() { if (_active) _f(); }

    void dismiss() { _active = false; }      // 主动放弃执行

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&& rhs) noexcept
        : _f(std::move(rhs._f)), _active(rhs._active) { rhs._active = false; }
private:
    F _f;
    bool _active;
};

template <class F>
ScopeGuard<F> make_guard(F f) { return ScopeGuard<F>(std::move(f)); }

// C++17 可以直接用 CTAD，不需要 make_guard
void scope_guard_demo() {
    std::cout << "    正常路径:\n";
    {
        auto guard = make_guard([] { std::cout << "      清理动作执行了\n"; });
        std::cout << "      做正事\n";
    }

    std::cout << "    异常路径:\n";
    try {
        auto guard = make_guard([] { std::cout << "      清理动作仍然执行\n"; });
        throw std::runtime_error("出错");
    } catch (...) {
        std::cout << "      捕获异常\n";
    }

    std::cout << "    dismiss 取消清理（事务提交成功的场合）:\n";
    {
        auto rollback = make_guard([] { std::cout << "      回滚！\n"; });
        std::cout << "      执行事务...成功\n";
        rollback.dismiss();     // 成功了就不回滚
    }
    std::cout << "      （没有回滚输出，因为已 dismiss）\n";
}

// ---------- 5. 用 scope guard 包装 C API ----------
void wrap_c_api() {
    // 假设这是个只提供 acquire/release 的 C 库
    struct CApi {
        static void* acquire() {
            std::cout << "      c_acquire()\n";
            return std::malloc(32);
        }
        static void release(void* h) {
            std::cout << "      c_release()\n";
            std::free(h);
        }
    };

    void* handle = CApi::acquire();
    auto guard = make_guard([handle] { CApi::release(handle); });

    std::cout << "      使用 handle，中间可以随意 return 或抛异常\n";
}

// ---------- 6. 事务性操作：copy-and-swap 实现强异常安全 ----------
class Config {
public:
    void update_unsafe(const std::string& k, const std::string& v) {
        _keys.push_back(k);        // 如果下一行抛异常，_keys 已经改了 -> 状态不一致
        _values.push_back(v);
    }

    void update_safe(const std::string& k, const std::string& v) {
        // 在副本上操作
        auto new_keys = _keys;
        auto new_values = _values;
        new_keys.push_back(k);
        new_values.push_back(v);
        // swap 不抛异常，这一步是"原子"的
        _keys.swap(new_keys);
        _values.swap(new_values);
    }

    std::size_t size() const { return _keys.size(); }
    bool consistent() const { return _keys.size() == _values.size(); }
private:
    std::vector<std::string> _keys;
    std::vector<std::string> _values;
};

void strong_guarantee() {
    Config c;
    c.update_safe("a", "1");
    c.update_safe("b", "2");
    std::cout << "      update_safe 两次后 size=" << c.size()
              << " 一致=" << c.consistent() << "\n";
    std::cout << "      copy-and-swap：先在副本上改，再用不抛异常的 swap 生效\n";
}

// ---------- 7. 为什么析构函数不能抛异常 ----------
void why_dtor_noexcept() {
    std::cout << "      栈展开时若析构函数再抛异常 -> std::terminate\n";
    std::cout << "      C++11 起析构函数默认 noexcept\n";
    std::cout << "      需要可能失败的清理时：析构里吞掉异常 + 另外提供显式 close()\n";
    std::cout << "      （std::fstream 就是这么设计的）\n";
}

int main() {
    std::cout << "--- 1. RAII 最小实现 ---\n";     raii_minimal();
    std::cout << "--- 2. 四条要素 ---\n";          raii_requirements();
    std::cout << "--- 3. 标准库 RAII ---\n";       stdlib_raii();
    std::cout << "--- 4. scope guard ---\n";       scope_guard_demo();
    std::cout << "--- 5. 包装 C API ---\n";        wrap_c_api();
    std::cout << "--- 6. 强异常安全 ---\n";        strong_guarantee();
    std::cout << "--- 7. 析构与异常 ---\n";        why_dtor_noexcept();
}
