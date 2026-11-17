// std=c++17
// 对应章节: blog/part3_class/03-03_destructor.md
// 主题: 析构函数、各种存储期对象的生灭时机与顺序

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ---------- 会报告生灭的类 ----------
class Tracer {
public:
    explicit Tracer(std::string name) : _name(std::move(name)) {
        std::cout << "      + " << _name << "\n";
    }
    ~Tracer() { std::cout << "      - " << _name << "\n"; }
    const std::string& name() const { return _name; }
private:
    std::string _name;
};

// ---------- 1. 需要显式析构函数的场合 ----------
class Computer {
public:
    Computer(const char* brand, double price)
        : _brand(new char[std::strlen(brand) + 1]())    // 构造时申请堆资源
        , _price(price) {
        std::strcpy(_brand, brand);
        std::cout << "      Computer(" << _brand << ")\n";
    }

    ~Computer() {
        std::cout << "      ~Computer(" << _brand << ") 回收堆空间\n";
        delete[] _brand;      // 析构时必须回收，否则泄漏
    }

    // 有了自定义析构函数，就必须考虑拷贝控制（Rule of Five，见 03-04）
    Computer(const Computer&) = delete;
    Computer& operator=(const Computer&) = delete;
private:
    char* _brand;
    double _price;
};

void when_needed() {
    Computer pc("Huawei MateBook14", 5699);
    std::cout << "      成员里有裸指针/句柄时，编译器生成的默认析构函数不够用\n";
}

// ---------- 2. 不需要显式析构函数的场合（Rule of Zero）----------
class ModernComputer {
public:
    ModernComputer(std::string brand, double price)
        : _brand(std::move(brand)), _price(price) {}
    // 不需要写析构函数：string 自己会清理
private:
    std::string _brand;
    double _price;
};

void rule_of_zero() {
    ModernComputer pc("ThinkPad X1", 9999);
    std::cout << "      成员都是 RAII 类型时，一个特殊函数都不用写\n";
}

// ---------- 3. 局部对象：逆序析构 ----------
void local_objects() {
    Tracer a("local-a");
    Tracer b("local-b");
    Tracer c("local-c");
    std::cout << "      构造顺序 a,b,c -> 析构顺序应为 c,b,a\n";
}

// ---------- 4. 嵌套作用域 ----------
void nested_scope() {
    Tracer outer("outer");
    {
        Tracer inner("inner");
        std::cout << "      内层作用域结束\n";
    }
    std::cout << "      inner 已析构，outer 还活着\n";
}

// ---------- 5. 静态局部对象：首次执行到定义处才构造，程序结束才析构 ----------
void static_local(int call_id) {
    std::cout << "      第 " << call_id << " 次调用\n";
    static Tracer s("static-local");     // 只构造一次
    std::cout << "      s.name() = " << s.name() << "\n";
}

// ---------- 6. 全局对象：main 之前构造，main 之后析构 ----------
Tracer g_global("global");

// ---------- 7. 堆对象：delete 时才析构 ----------
void heap_objects() {
    Tracer* p = new Tracer("heap-raw");
    std::cout << "      new 出来的对象，不 delete 就不会析构\n";
    delete p;

    {
        auto up = std::make_unique<Tracer>("heap-unique");
        std::cout << "      unique_ptr 管理的堆对象，离开作用域自动析构\n";
    }
}

// ---------- 8. 成员对象与基类的析构顺序 ----------
class Member {
public:
    explicit Member(const char* n) : _t(n) {}
private:
    Tracer _t;
};

class BaseC {
public:
    BaseC() : _t("Base成员") { std::cout << "      BaseC() 构造完成\n"; }
    virtual ~BaseC() { std::cout << "      ~BaseC() 开始\n"; }
private:
    Tracer _t;
};

class DerivedC : public BaseC {
public:
    DerivedC() : _m1("Derived成员1"), _m2("Derived成员2") {
        std::cout << "      DerivedC() 构造完成\n";
    }
    ~DerivedC() override { std::cout << "      ~DerivedC() 开始\n"; }
private:
    Member _m1;
    Member _m2;
};

void composite_order() {
    std::cout << "    构造/析构顺序:\n";
    DerivedC d;
    std::cout << "    ---- 即将析构 ----\n";
}

// ---------- 9. 容器中的对象 ----------
void container_objects() {
    std::cout << "    vector 析构时会析构所有元素:\n";
    {
        std::vector<Tracer> v;
        v.reserve(3);
        v.emplace_back("vec-0");
        v.emplace_back("vec-1");
        v.emplace_back("vec-2");
        std::cout << "      ---- vector 即将销毁 ----\n";
    }
}

// ---------- 10. 析构函数可以显式调用，但几乎不该这么做 ----------
void explicit_destructor_call() {
    std::cout << "      placement new 构造的对象必须显式调用析构函数（见 02-02）\n";
    alignas(Tracer) unsigned char buf[sizeof(Tracer)];
    Tracer* p = new (buf) Tracer("placement");
    p->~Tracer();          // 这是唯一的正当场合
    std::cout << "      普通对象显式调用析构函数会导致二次析构，是未定义行为\n";
}

int main() {
    std::cout << "=== main 开始（注意 global 已在此之前构造）===\n";
    std::cout << "--- 1. 需要显式析构 ---\n";       when_needed();
    std::cout << "--- 2. Rule of Zero ---\n";       rule_of_zero();
    std::cout << "--- 3. 局部对象逆序析构 ---\n";   local_objects();
    std::cout << "--- 4. 嵌套作用域 ---\n";         nested_scope();
    std::cout << "--- 5. 静态局部对象 ---\n";
    static_local(1); static_local(2); static_local(3);
    std::cout << "--- 7. 堆对象 ---\n";             heap_objects();
    std::cout << "--- 8. 成员与基类顺序 ---\n";     composite_order();
    std::cout << "--- 9. 容器元素 ---\n";           container_objects();
    std::cout << "--- 10. 显式调用析构函数 ---\n";  explicit_destructor_call();
    std::cout << "=== main 结束（之后才析构 static-local 和 global）===\n";
}
