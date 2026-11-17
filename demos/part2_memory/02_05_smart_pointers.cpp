// std=c++17
// 对应章节: blog/part2_memory/02-05_smart_pointers.md
// 主题: unique_ptr / shared_ptr / weak_ptr、循环引用、自定义删除器

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Res {
public:
    explicit Res(std::string name) : _name(std::move(name)) {
        std::cout << "      + 获取 " << _name << "\n";
    }
    ~Res() { std::cout << "      - 释放 " << _name << "\n"; }
    void use() const { std::cout << "      . 使用 " << _name << "\n"; }
    const std::string& name() const { return _name; }
private:
    std::string _name;
};

// ---------- 1. 裸指针的三个问题 ----------
void raw_pointer_problems() {
    std::cout << "    问题一：忘记 delete -> 泄漏\n";
    {
        Res* p = new Res("leaked");
        p->use();
        // 忘了 delete p;  —— 这里故意泄漏来演示（实际代码不要这样）
        delete p;   // 补上，避免 demo 真的泄漏
    }

    std::cout << "    问题二：提前 return 或抛异常 -> 泄漏\n";
    std::cout << "      （见 01-08，手工 delete 在异常面前不可靠）\n";

    std::cout << "    问题三：所有权不清\n";
    std::cout << "      Res* get();  // 调用方该不该 delete？签名没说\n";
}

// ---------- 2. unique_ptr：独占所有权 ----------
void unique_ptr_basics() {
    std::cout << "    作用域结束自动释放:\n";
    {
        auto p = std::make_unique<Res>("unique-1");
        p->use();
        std::cout << "      (离开作用域)\n";
    }

    std::cout << "    不可拷贝，只可移动:\n";
    auto p1 = std::make_unique<Res>("unique-2");
    // auto p2 = p1;                    // 编译错误：拷贝构造被删除
    auto p2 = std::move(p1);            // ok：所有权转移
    std::cout << "      移动后 p1 是否为空: " << (p1 == nullptr) << "\n";
    p2->use();

    std::cout << "    release / reset / get:\n";
    auto p3 = std::make_unique<Res>("unique-3");
    Res* raw = p3.get();                // 借用，不转移所有权
    raw->use();
    p3.reset();                         // 立即释放
    std::cout << "      reset 后是否为空: " << (p3 == nullptr) << "\n";

    auto p4 = std::make_unique<Res>("unique-4");
    Res* released = p4.release();        // 放弃所有权，返回裸指针
    std::cout << "      release 后需要手工 delete\n";
    delete released;
}

// ---------- 3. unique_ptr 作为函数返回值表达所有权转移 ----------
std::unique_ptr<Res> factory(const std::string& name) {
    return std::make_unique<Res>(name);   // 签名就说明了"所有权归你"
}

void takes_ownership(std::unique_ptr<Res> p) {   // 按值接收 = 接管所有权
    std::cout << "      函数接管了 " << p->name() << "\n";
}

void borrows(const Res& r) {                     // 引用 = 只借用
    std::cout << "      函数只借用 " << r.name() << "\n";
}

void ownership_semantics() {
    auto p = factory("owned");
    borrows(*p);                          // 借用，p 仍持有
    takes_ownership(std::move(p));        // 转移，之后 p 为空
    std::cout << "      转移后 p 是否为空: " << (p == nullptr) << "\n";
}

// ---------- 4. unique_ptr 管理数组 ----------
void unique_ptr_array() {
    auto arr = std::make_unique<int[]>(5);     // 自动用 delete[]
    for (int i = 0; i < 5; ++i) arr[i] = i * i;
    std::cout << "      arr:";
    for (int i = 0; i < 5; ++i) std::cout << " " << arr[i];
    std::cout << "\n      （实践中优先用 std::vector）\n";
}

// ---------- 5. shared_ptr：共享所有权 + 引用计数 ----------
void shared_ptr_basics() {
    std::cout << "    引用计数变化:\n";
    auto p1 = std::make_shared<Res>("shared-1");
    std::cout << "      创建后 use_count = " << p1.use_count() << "\n";
    {
        auto p2 = p1;                      // 拷贝 -> 计数 +1
        std::cout << "      拷贝后 use_count = " << p1.use_count() << "\n";
        p2->use();
        std::cout << "      (p2 离开作用域)\n";
    }
    std::cout << "      p2 销毁后 use_count = " << p1.use_count() << "\n";
    std::cout << "      (p1 离开作用域，计数归零才真正释放)\n";
}

// ---------- 6. make_shared vs new 的差别 ----------
void make_shared_benefit() {
    // 两次分配：一次给对象，一次给控制块
    std::shared_ptr<Res> a(new Res("by-new"));
    // 一次分配：对象和控制块放在一起
    auto b = std::make_shared<Res>("by-make-shared");

    std::cout << "      make_shared 只做一次内存分配，且异常安全\n";
    std::cout << "      f(shared_ptr<T>(new T), g()) 的求值顺序可能导致泄漏\n";
}

// ---------- 7. 循环引用：shared_ptr 的致命问题 ----------
struct NodeBad {
    std::string name;
    std::shared_ptr<NodeBad> next;      // 互相持有 shared_ptr
    std::shared_ptr<NodeBad> prev;
    explicit NodeBad(std::string n) : name(std::move(n)) {
        std::cout << "      + NodeBad " << name << "\n";
    }
    ~NodeBad() { std::cout << "      - NodeBad " << name << "\n"; }
};

struct NodeGood {
    std::string name;
    std::shared_ptr<NodeGood> next;     // 持有下一个
    std::weak_ptr<NodeGood> prev;       // 弱引用上一个，不增加计数
    explicit NodeGood(std::string n) : name(std::move(n)) {
        std::cout << "      + NodeGood " << name << "\n";
    }
    ~NodeGood() { std::cout << "      - NodeGood " << name << "\n"; }
};

void cycle_problem() {
    std::cout << "    用 shared_ptr 互相持有 -> 计数永远不归零，泄漏:\n";
    {
        auto a = std::make_shared<NodeBad>("A");
        auto b = std::make_shared<NodeBad>("B");
        a->next = b;
        b->prev = a;        // 形成环
        std::cout << "      a.use_count=" << a.use_count()
                  << " b.use_count=" << b.use_count() << "\n";
        std::cout << "      (离开作用域，但析构函数不会被调用！)\n";
    }
    std::cout << "      ^ 注意上面没有任何 \"- NodeBad\" 输出，这就是泄漏\n";

    std::cout << "    把反向指针改成 weak_ptr -> 正常释放:\n";
    {
        auto a = std::make_shared<NodeGood>("A");
        auto b = std::make_shared<NodeGood>("B");
        a->next = b;
        b->prev = a;        // weak_ptr 不增加计数
        std::cout << "      a.use_count=" << a.use_count()
                  << " b.use_count=" << b.use_count() << "\n";
        std::cout << "      (离开作用域)\n";
    }
}

// ---------- 8. weak_ptr 的使用：lock() 检查对象是否还活着 ----------
void weak_ptr_usage() {
    std::weak_ptr<Res> weak;
    {
        auto strong = std::make_shared<Res>("observed");
        weak = strong;
        std::cout << "      对象存活时 expired=" << weak.expired() << "\n";
        if (auto locked = weak.lock()) {      // 提升为 shared_ptr
            locked->use();
            std::cout << "      lock() 成功，期间 use_count=" << locked.use_count() << "\n";
        }
        std::cout << "      (strong 离开作用域)\n";
    }
    std::cout << "      对象销毁后 expired=" << weak.expired() << "\n";
    if (auto locked = weak.lock()) {
        locked->use();
    } else {
        std::cout << "      lock() 返回空，安全地知道对象已不在\n";
    }
}

// ---------- 9. 自定义删除器 ----------
void custom_deleter() {
    std::cout << "    管理 FILE*:\n";
    {
        auto closer = [](std::FILE* f) {
            if (f) { std::fclose(f); std::cout << "      文件已关闭\n"; }
        };
        std::unique_ptr<std::FILE, decltype(closer)> fp(std::fopen("/tmp/sp_demo.txt", "w"), closer);
        if (fp) {
            std::fputs("hello from smart pointer\n", fp.get());
            std::cout << "      写入成功\n";
        }
    }

    std::cout << "    管理 malloc 的内存:\n";
    {
        std::unique_ptr<void, decltype(&std::free)> mem(std::malloc(64), &std::free);
        std::cout << "      malloc 的内存由 unique_ptr 管理，自动 free\n";
    }

    std::cout << "    shared_ptr 的删除器不进类型，更灵活:\n";
    {
        std::shared_ptr<Res> p(new Res("custom-deleted"),
                               [](Res* r) {
                                   std::cout << "      自定义删除器被调用\n";
                                   delete r;
                               });
    }
}

// ---------- 10. enable_shared_from_this ----------
class Widget : public std::enable_shared_from_this<Widget> {
public:
    std::shared_ptr<Widget> self() { return shared_from_this(); }
    ~Widget() { std::cout << "      - Widget\n"; }
};

void shared_from_this_demo() {
    auto p = std::make_shared<Widget>();
    auto q = p->self();                  // 安全地从 this 得到 shared_ptr
    std::cout << "      use_count = " << p.use_count() << "（两个 shared_ptr 指向同一对象）\n";
    // 注意：如果对象不是由 shared_ptr 管理的，shared_from_this() 会抛 bad_weak_ptr
}

int main() {
    std::cout << "--- 1. 裸指针的问题 ---\n";        raw_pointer_problems();
    std::cout << "--- 2. unique_ptr ---\n";          unique_ptr_basics();
    std::cout << "--- 3. 所有权语义 ---\n";          ownership_semantics();
    std::cout << "--- 4. 管理数组 ---\n";            unique_ptr_array();
    std::cout << "--- 5. shared_ptr ---\n";          shared_ptr_basics();
    std::cout << "--- 6. make_shared ---\n";         make_shared_benefit();
    std::cout << "--- 7. 循环引用 ---\n";            cycle_problem();
    std::cout << "--- 8. weak_ptr ---\n";            weak_ptr_usage();
    std::cout << "--- 9. 自定义删除器 ---\n";        custom_deleter();
    std::cout << "--- 10. shared_from_this ---\n";   shared_from_this_demo();
}
