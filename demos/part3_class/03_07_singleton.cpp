// std=c++17 flags=-pthread
// 对应章节: blog/part3_class/03-07_singleton.md
// 主题: 单例模式从裸指针到 Meyers Singleton 的四次进化

#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// ================= 版本一：裸指针懒汉（参考文档里的经典写法）=================
class V1_Naive {
public:
    static V1_Naive* getInstance() {
        if (nullptr == _pInstance) {          // 问题1：多线程下这里有竞态
            _pInstance = new V1_Naive();      // 问题2：谁来 delete？
        }
        return _pInstance;
    }
    void use() const { std::cout << "      V1_Naive::use()\n"; }
private:
    V1_Naive() { std::cout << "      V1_Naive() 构造\n"; }
    ~V1_Naive() { std::cout << "      V1_Naive() 析构\n"; }   // 永远不会被调用
    V1_Naive(const V1_Naive&) = delete;
    V1_Naive& operator=(const V1_Naive&) = delete;

    static V1_Naive* _pInstance;
};
V1_Naive* V1_Naive::_pInstance = nullptr;

void version1() {
    V1_Naive* p1 = V1_Naive::getInstance();
    V1_Naive* p2 = V1_Naive::getInstance();
    assert(p1 == p2);
    p1->use();
    std::cout << "      两次 getInstance 返回同一地址: " << (p1 == p2) << "\n";
    std::cout << "      问题：1) 多线程不安全  2) 析构函数永不调用（资源不释放）\n";
}

// ================= 版本二：加锁 =================
class V2_Locked {
public:
    static V2_Locked* getInstance() {
        std::lock_guard<std::mutex> lk(_mtx);   // 每次调用都要加锁
        if (nullptr == _pInstance) {
            _pInstance = new V2_Locked();
        }
        return _pInstance;
    }
    void use() const { std::cout << "      V2_Locked::use()\n"; }
private:
    V2_Locked() { std::cout << "      V2_Locked() 构造\n"; }
    ~V2_Locked() = default;
    static V2_Locked* _pInstance;
    static std::mutex _mtx;
};
V2_Locked* V2_Locked::_pInstance = nullptr;
std::mutex V2_Locked::_mtx;

void version2() {
    // 多线程并发调用，验证只构造一次
    std::vector<std::thread> threads;
    std::vector<V2_Locked*> results(8, nullptr);
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([i, &results] { results[i] = V2_Locked::getInstance(); });
    }
    for (auto& t : threads) t.join();

    bool all_same = true;
    for (auto* p : results) if (p != results[0]) all_same = false;
    std::cout << "      8 个线程拿到的指针全部相同: " << all_same << "\n";
    std::cout << "      问题：每次 getInstance 都加锁，而只有第一次需要\n";
}

// ================= 版本三：双检锁（曾经流行，但需要 atomic 才正确）=================
class V3_DoubleChecked {
public:
    static V3_DoubleChecked* getInstance() {
        // 第一次检查：无锁，快速路径
        V3_DoubleChecked* tmp = _pInstance.load(std::memory_order_acquire);
        if (tmp == nullptr) {
            std::lock_guard<std::mutex> lk(_mtx);
            tmp = _pInstance.load(std::memory_order_relaxed);
            if (tmp == nullptr) {               // 第二次检查：持锁
                tmp = new V3_DoubleChecked();
                _pInstance.store(tmp, std::memory_order_release);
            }
        }
        return tmp;
    }
    void use() const { std::cout << "      V3_DoubleChecked::use()\n"; }
private:
    V3_DoubleChecked() { std::cout << "      V3_DoubleChecked() 构造\n"; }
    static std::atomic<V3_DoubleChecked*> _pInstance;
    static std::mutex _mtx;
};
std::atomic<V3_DoubleChecked*> V3_DoubleChecked::_pInstance{nullptr};
std::mutex V3_DoubleChecked::_mtx;

void version3() {
    auto* p = V3_DoubleChecked::getInstance();
    p->use();
    std::cout << "      用裸指针 + 普通 if 的双检锁是【错的】：\n";
    std::cout << "        编译器/CPU 可能重排\"分配内存\"和\"运行构造函数\"，\n";
    std::cout << "        另一个线程会拿到未构造完的对象\n";
    std::cout << "      必须用 atomic + acquire/release 内存序才正确（见 08-04）\n";
}

// ================= 版本四：Meyers Singleton（推荐）=================
class V4_Meyers {
public:
    static V4_Meyers& getInstance() {
        static V4_Meyers instance;    // C++11 保证线程安全的初始化
        return instance;              // 返回引用，不是指针
    }
    void use() const { std::cout << "      V4_Meyers::use()\n"; }
private:
    V4_Meyers() { std::cout << "      V4_Meyers() 构造\n"; }
    ~V4_Meyers() { std::cout << "      V4_Meyers() 析构（程序结束时自动）\n"; }
    V4_Meyers(const V4_Meyers&) = delete;
    V4_Meyers& operator=(const V4_Meyers&) = delete;
    V4_Meyers(V4_Meyers&&) = delete;
    V4_Meyers& operator=(V4_Meyers&&) = delete;
};

void version4() {
    // 多线程并发，验证只构造一次且线程安全
    std::atomic<int> ok{0};
    std::vector<std::thread> threads;
    V4_Meyers* first = nullptr;
    std::mutex m;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&] {
            V4_Meyers& inst = V4_Meyers::getInstance();
            std::lock_guard<std::mutex> lk(m);
            if (!first) first = &inst;
            if (&inst == first) ++ok;
        });
    }
    for (auto& t : threads) t.join();
    std::cout << "      8 个线程全部拿到同一实例: " << (ok == 8) << "\n";
    std::cout << "      优点：代码最短、线程安全、自动析构、懒加载、无锁开销\n";
}

// ================= 版本五：call_once（需要返回指针时用）=================
class V5_CallOnce {
public:
    static V5_CallOnce& getInstance() {
        std::call_once(_flag, [] { _pInstance.reset(new V5_CallOnce()); });
        return *_pInstance;
    }
    void use() const { std::cout << "      V5_CallOnce::use()\n"; }

private:
    V5_CallOnce() { std::cout << "      V5_CallOnce() 构造\n"; }
    ~V5_CallOnce() { std::cout << "      V5_CallOnce() 析构\n"; }

    // 析构函数是 private，unique_ptr 的默认删除器访问不到它：
    //   error: 'V5_CallOnce::~V5_CallOnce()' is private within this context
    // 解决办法：提供一个有权限的自定义删除器（嵌套类是本类的成员，有访问权）
    struct Deleter {
        void operator()(V5_CallOnce* p) const { delete p; }
    };

    static std::once_flag _flag;
    static std::unique_ptr<V5_CallOnce, Deleter> _pInstance;
};
std::once_flag V5_CallOnce::_flag;
std::unique_ptr<V5_CallOnce, V5_CallOnce::Deleter> V5_CallOnce::_pInstance;

void version5() {
    V5_CallOnce::getInstance().use();
    std::cout << "      call_once 保证初始化只执行一次，unique_ptr 保证释放\n";
    std::cout << "      适合需要控制析构时机、或初始化逻辑复杂的场合\n";
}

// ================= 单例的真正问题：它是全局状态 =================
// 更好的替代：依赖注入
class Logger {
public:
    explicit Logger(std::string prefix) : _prefix(std::move(prefix)) {}
    void log(const std::string& msg) const {
        std::cout << "      [" << _prefix << "] " << msg << "\n";
    }
private:
    std::string _prefix;
};

class Service {
public:
    explicit Service(Logger& logger) : _logger(logger) {}   // 依赖注入
    void work() { _logger.log("Service::work()"); }
private:
    Logger& _logger;
};

void dependency_injection() {
    Logger prod_logger("PROD");
    Service s(prod_logger);
    s.work();

    Logger test_logger("TEST");     // 测试时可以注入另一个
    Service s2(test_logger);
    s2.work();
    std::cout << "      依赖注入让单元测试可以替换依赖，单例做不到\n";
}

int main() {
    std::cout << "--- V1 裸指针懒汉 ---\n";        version1();
    std::cout << "--- V2 加锁 ---\n";              version2();
    std::cout << "--- V3 双检锁 ---\n";            version3();
    std::cout << "--- V4 Meyers（推荐）---\n";     version4();
    std::cout << "--- V5 call_once ---\n";         version5();
    std::cout << "--- 替代方案：依赖注入 ---\n";   dependency_injection();
    std::cout << "=== main 结束，下面是自动析构 ===\n";
}
