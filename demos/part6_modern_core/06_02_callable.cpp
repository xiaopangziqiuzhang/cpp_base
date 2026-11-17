// std=c++17
// 对应章节: blog/part6_modern_core/06-02_callable.md
// 主题: 五种可调用对象、std::function、bind、ref、invoke

#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

// ---------- 1. C++ 里的五种可调用对象 ----------
namespace five_kinds {
    // 一、普通函数
    int plain_function(int x) { return x * 2; }

    // 二、函数指针
    using FuncPtr = int(*)(int);

    // 三、函数对象（仿函数）
    struct Functor {
        int operator()(int x) const { return x * 3; }
    };

    // 四、lambda
    // 五、成员函数指针 / 成员变量指针
    struct Widget {
        int value = 10;
        int triple() const { return value * 3; }
        int add(int x) const { return value + x; }
    };

    void demo() {
        std::cout << "      1. 普通函数:       " << plain_function(21) << "\n";

        FuncPtr fp = plain_function;
        std::cout << "      2. 函数指针:       " << fp(21) << "\n";

        Functor f;
        std::cout << "      3. 函数对象:       " << f(14) << "\n";

        auto lam = [](int x) { return x * 4; };
        std::cout << "      4. lambda:         " << lam(10) << "\n";

        Widget w;
        int (Widget::*mem_fn)() const = &Widget::triple;
        int Widget::*mem_var = &Widget::value;
        std::cout << "      5. 成员函数指针:   " << (w.*mem_fn)() << "\n";
        std::cout << "         成员变量指针:   " << (w.*mem_var) << "\n";
        std::cout << "      注意成员指针的调用语法很特殊: (obj.*ptr)(args)\n";
    }
}

// ---------- 2. std::function：统一的可调用对象容器 ----------
namespace std_function {
    int plain(int x) { return x + 1; }
    struct Functor { int operator()(int x) const { return x + 2; } };

    void demo() {
        // 同一个 std::function 可以装任何签名匹配的可调用对象
        std::function<int(int)> f;

        f = plain;                              std::cout << "      装普通函数:   " << f(10) << "\n";
        f = Functor{};                          std::cout << "      装函数对象:   " << f(10) << "\n";
        f = [](int x) { return x + 3; };        std::cout << "      装 lambda:    " << f(10) << "\n";
        f = [](int x) { return x + 4; };        std::cout << "      换一个 lambda:" << f(10) << "\n";

        // 可以放进容器 —— 这是 lambda 做不到的（每个 lambda 类型不同）
        std::vector<std::function<int(int)>> handlers;
        handlers.push_back(plain);
        handlers.push_back(Functor{});
        handlers.push_back([](int x) { return x * 10; });
        std::cout << "      放进 vector（lambda 做不到）: ";
        for (const auto& h : handlers) std::cout << h(1) << " ";
        std::cout << "\n";

        // 空的 function 调用会抛异常
        std::function<void()> empty;
        std::cout << "      空 function 转 bool: " << static_cast<bool>(empty) << "\n";
        try {
            empty();
        } catch (const std::bad_function_call& e) {
            std::cout << "      调用空 function 抛出: " << e.what() << "\n";
        }
    }
}

// ---------- 3. std::function 的代价 ----------
namespace function_cost {
    void demo() {
        constexpr int kTimes = 5000000;
        auto lam = [](int x) { return x * 2; };
        std::function<int(int)> fn = lam;
        int (*fp)(int) = [](int x) { return x * 2; };

        auto bench = [](const char* name, auto&& callable, int times) {
            auto t1 = std::chrono::steady_clock::now();
            long long sum = 0;
            for (int i = 0; i < times; ++i) sum += callable(i);
            auto t2 = std::chrono::steady_clock::now();
            std::cout << "        " << name << ": "
                      << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()
                      << " us (sum=" << sum << ")\n";
        };

        std::cout << "      " << kTimes << " 次调用:\n";
        bench("直接调用 lambda    ", lam, kTimes);
        bench("通过函数指针       ", fp, kTimes);
        bench("通过 std::function ", fn, kTimes);

        std::cout << "      sizeof(lambda) = " << sizeof(lam)
                  << ", sizeof(std::function) = " << sizeof(fn) << "\n";
        std::cout << "      实测结论（-O0 上面；-O2 另测）:\n";
        std::cout << "        -O0: lambda 7241 us / 函数指针 13351 us / function 97203 us\n";
        std::cout << "        -O2: lambda     0 us / 函数指针  6683 us / function  6675 us\n";
        std::cout << "      -O2 下直接调用被整体优化掉（编译器内联后消除了循环），\n";
        std::cout << "      而函数指针和 function 都阻止了内联，两者开销反而接近。\n";
        std::cout << "      所以 std::function 的真正代价是【阻止内联】，而非间接调用本身\n";
    }
}

// ---------- 4. std::bind：绑定参数 ----------
namespace bind_demo {
    int add3(int a, int b, int c) { return a + b + c; }
    void print_two(const std::string& a, const std::string& b) {
        std::cout << "        " << a << " | " << b << "\n";
    }

    void demo() {
        using namespace std::placeholders;

        auto bound = std::bind(add3, 1, 2, _1);     // 固定前两个参数
        std::cout << "      bind(add3, 1, 2, _1)(3) = " << bound(3) << "\n";

        auto reordered = std::bind(add3, _3, _1, _2);    // 重排参数顺序
        std::cout << "      bind(add3, _3,_1,_2)(1,2,3) = " << reordered(1, 2, 3) << "\n";

        auto swapped = std::bind(print_two, _2, _1);
        std::cout << "      交换参数顺序: ";
        swapped("first", "second");

        // 绑定成员函数
        struct Widget {
            int value = 100;
            int add(int x) const { return value + x; }
        };
        Widget w;
        auto member_bound = std::bind(&Widget::add, &w, _1);
        std::cout << "      绑定成员函数: " << member_bound(5) << "\n";

        std::cout << "      现代替代方案：几乎所有 bind 的用法都能用 lambda 更清楚地表达\n";
        auto lambda_version = [](int c) { return add3(1, 2, c); };
        std::cout << "        lambda 版本: " << lambda_version(3) << "\n";
    }
}

// ---------- 5. std::ref / std::cref：在需要拷贝的地方传引用 ----------
namespace ref_demo {
    void increment(int& x) { ++x; }
    void print_size(const std::vector<int>& v) {
        std::cout << "        收到 vector，size=" << v.size() << "\n";
    }

    void demo() {
        int counter = 0;

        // bind 默认【拷贝】参数，所以这样改不到原变量
        auto bad = std::bind(increment, counter);
        bad(); bad();
        std::cout << "      bind 默认拷贝参数，counter 仍是: " << counter << "\n";

        // std::ref 包装成引用
        auto good = std::bind(increment, std::ref(counter));
        good(); good();
        std::cout << "      用 std::ref 后，counter = " << counter << "\n";

        // reference_wrapper 让引用可以放进容器
        int a = 1, b = 2, c = 3;
        std::vector<std::reference_wrapper<int>> refs{a, b, c};
        for (int& r : refs) r *= 10;
        std::cout << "      通过 vector<reference_wrapper<int>> 修改: "
                  << a << " " << b << " " << c << "\n";
        std::cout << "      （std::vector<int&> 是非法的，这是标准解法）\n";

        // thread 也需要 std::ref
        std::cout << "      std::thread 同样默认拷贝参数，需要 std::ref（见 08-01）\n";
    }
}

// ---------- 6. std::invoke [C++17]：统一的调用语法 ----------
namespace invoke_demo {
    struct Widget {
        int value = 42;
        int triple() const { return value * 3; }
    };

    int free_func(int x) { return x + 1; }

    void demo() {
        Widget w;

        // C++17 之前：五种可调用对象有不同的调用语法
        std::cout << "      调用语法的不一致:\n";
        std::cout << "        普通函数:     free_func(1)      = " << free_func(1) << "\n";
        std::cout << "        成员函数:     (w.*&W::triple)() = "
                  << (w.*&Widget::triple)() << "\n";
        std::cout << "        成员变量:     w.*&W::value      = "
                  << (w.*&Widget::value) << "\n";

        // std::invoke 统一了它们
        std::cout << "      std::invoke 统一语法:\n";
        std::cout << "        invoke(free_func, 1)         = "
                  << std::invoke(free_func, 1) << "\n";
        std::cout << "        invoke(&W::triple, w)        = "
                  << std::invoke(&Widget::triple, w) << "\n";
        std::cout << "        invoke(&W::value, w)         = "
                  << std::invoke(&Widget::value, w) << "\n";
        std::cout << "        invoke(lambda, 1)            = "
                  << std::invoke([](int x) { return x * 100; }, 1) << "\n";

        std::cout << "      价值：写泛型代码时不用为五种可调用对象分别处理\n";

        // is_invocable 检测可调用性 [C++17]
        std::cout << "      is_invocable_v<decltype(free_func), int> = "
                  << std::is_invocable_v<decltype(free_func), int> << "\n";
        std::cout << "      is_invocable_v<decltype(free_func), std::string> = "
                  << std::is_invocable_v<decltype(free_func), std::string> << "\n";
    }
}

// ---------- 7. 实际应用：回调注册表 ----------
namespace callback_registry {
    class EventBus {
    public:
        using Handler = std::function<void(const std::string&)>;

        void subscribe(const std::string& event, Handler h) {
            _handlers[event].push_back(std::move(h));
        }

        void publish(const std::string& event, const std::string& data) const {
            auto it = _handlers.find(event);
            if (it == _handlers.end()) return;
            for (const auto& h : it->second) h(data);
        }

    private:
        std::map<std::string, std::vector<Handler>> _handlers;
    };

    void free_handler(const std::string& data) {
        std::cout << "        [free function] " << data << "\n";
    }

    class Logger {
    public:
        void on_event(const std::string& data) const {
            std::cout << "        [Logger::on_event] " << data << "\n";
        }
    };

    void demo() {
        EventBus bus;
        Logger logger;

        bus.subscribe("click", free_handler);                        // 普通函数
        bus.subscribe("click", [](const std::string& d) {             // lambda
            std::cout << "        [lambda] " << d << "\n";
        });
        bus.subscribe("click", [&logger](const std::string& d) {      // 绑定成员函数
            logger.on_event(d);
        });

        bus.publish("click", "button-1");
        std::cout << "      std::function 让不同来源的回调能统一存储\n";
    }
}

int main() {
    std::cout << "--- 1. 五种可调用对象 ---\n";       five_kinds::demo();
    std::cout << "--- 2. std::function ---\n";        std_function::demo();
    std::cout << "--- 3. function 的代价 ---\n";      function_cost::demo();
    std::cout << "--- 4. std::bind ---\n";            bind_demo::demo();
    std::cout << "--- 5. std::ref ---\n";             ref_demo::demo();
    std::cout << "--- 6. std::invoke [C++17] ---\n";  invoke_demo::demo();
    std::cout << "--- 7. 实战：事件总线 ---\n";       callback_registry::demo();
}
