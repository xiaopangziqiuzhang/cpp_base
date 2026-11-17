// std=c++17
// 对应章节: blog/part2_memory/02-02_new_delete.md
// 主题: new/delete 的工作步骤、operator new 重载、placement new

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <string>

// ---------- 1. new/delete vs malloc/free ----------
class Widget {
public:
    Widget() : _id(++s_count) { std::cout << "    Widget() id=" << _id << "\n"; }
    ~Widget() { std::cout << "    ~Widget() id=" << _id << "\n"; }
    int id() const { return _id; }
private:
    int _id;
    static int s_count;
};
int Widget::s_count = 0;

void new_vs_malloc() {
    std::cout << "  new 会调用构造函数:\n";
    Widget* w = new Widget;
    std::cout << "  delete 会调用析构函数:\n";
    delete w;

    std::cout << "  malloc 不调用构造函数（对象未初始化，用它是未定义行为）:\n";
    void* raw = std::malloc(sizeof(Widget));
    std::cout << "    malloc 返回了 " << sizeof(Widget) << " 字节的未初始化内存\n";
    std::free(raw);     // free 也不调用析构函数

    std::cout << "  new 自动计算大小，malloc 要手工传:\n";
    int* p1 = new int;                      // 不用写 sizeof
    int* p2 = static_cast<int*>(std::malloc(sizeof(int)));  // 必须写
    delete p1;
    std::free(p2);
}

// ---------- 2. new 的初始化行为 ----------
void new_initialization() {
    int* p1 = new int;        // 未初始化，值不确定
    int* p2 = new int();      // 零初始化
    int* p3 = new int{};      // 零初始化 [C++11]
    int* p4 = new int(42);    // 指定值
    int* p5 = new int{42};    // 指定值 [C++11]

    std::cout << "  new int()  -> " << *p2 << "\n";
    std::cout << "  new int{}  -> " << *p3 << "\n";
    std::cout << "  new int(42)-> " << *p4 << "\n";
    std::cout << "  new int{42}-> " << *p5 << "\n";
    std::cout << "  （new int 不初始化，读它的值是未定义行为）\n";
    delete p1; delete p2; delete p3; delete p4; delete p5;

    // 数组
    int* arr1 = new int[5];      // 5 个未初始化的 int
    int* arr2 = new int[5]();    // 5 个 0
    int* arr3 = new int[5]{1,2}; // [C++11] 1,2,0,0,0
    std::cout << "  new int[5]()  前三个: " << arr2[0] << "," << arr2[1] << "," << arr2[2] << "\n";
    std::cout << "  new int[5]{1,2} 全部: ";
    for (int i = 0; i < 5; ++i) std::cout << arr3[i] << " ";
    std::cout << "\n";
    delete[] arr1; delete[] arr2; delete[] arr3;
}

// ---------- 3. delete 和 delete[] 必须配对 ----------
void delete_pairing() {
    std::cout << "  数组必须用 delete[]：\n";
    Widget* arr = new Widget[2];
    delete[] arr;      // 正确：逆序调用每个元素的析构函数

    std::cout << "  用 delete 删数组是未定义行为（只析构第一个，且堆结构可能损坏）\n";
    std::cout << "  用 delete[] 删单对象同样是未定义行为\n";

    // delete 空指针是安全的，无需检查
    Widget* null = nullptr;
    delete null;       // 合法，什么也不做
    std::cout << "  delete nullptr 是安全的，不需要先判空\n";
}

// ---------- 4. new 失败时抛 bad_alloc，nothrow 版本返回 nullptr ----------
// 这个大小的选择有两个讲究：
//  1. 必须是运行期值。写成常量表达式时 GCC 直接在编译期拒绝：
//     error: size '...' of array exceeds maximum object size
//  2. 不能超过 PTRDIFF_MAX/sizeof(T)，否则抛的是 bad_array_new_length
//     （长度检查在分配之前，连 nothrow 版本也会抛），而不是我们想演示的 bad_alloc。
// 这里取 1<<60 个 int ≈ 4 EB：能通过长度检查，但系统一定分配不出来。
std::size_t absurd_size() {
    volatile std::size_t n = static_cast<std::size_t>(1) << 60;
    return n;
}

void allocation_failure() {
    try {
        int* p = new int[absurd_size()];
        delete[] p;
    } catch (const std::bad_alloc& e) {
        std::cout << "  new 失败抛出 bad_alloc: " << e.what() << "\n";
    }

    // nothrow 版本：失败返回 nullptr 而不抛异常
    int* p = new (std::nothrow) int[absurd_size()];
    std::cout << "  new(std::nothrow) 失败时返回 "
              << (p == nullptr ? "nullptr" : "有效指针") << "\n";
    delete[] p;     // delete nullptr 是安全的
}

// ---------- 5. 重载类的 operator new / operator delete ----------
class Student {
public:
    Student(int id, const char* name)
        : _id(id), _name(new char[std::strlen(name) + 1]()) {
        std::strcpy(_name, name);
        std::cout << "    Student() 构造\n";
    }
    ~Student() {
        delete[] _name;
        std::cout << "    ~Student() 析构\n";
    }

    // 类专属的 operator new：只负责分配内存，不构造对象
    void* operator new(std::size_t sz) {
        std::cout << "    Student::operator new(" << sz << ") 被调用\n";
        return std::malloc(sz);
    }
    void operator delete(void* p) {
        std::cout << "    Student::operator delete 被调用\n";
        std::free(p);
    }
    // 数组版本是独立的
    void* operator new[](std::size_t sz) {
        std::cout << "    Student::operator new[](" << sz << ") 被调用\n";
        return std::malloc(sz);
    }
    void operator delete[](void* p) {
        std::cout << "    Student::operator delete[] 被调用\n";
        std::free(p);
    }

    void print() const { std::cout << "    id=" << _id << " name=" << _name << "\n"; }
private:
    int _id;
    char* _name;
};

void operator_new_overload() {
    std::cout << "  单对象:\n";
    Student* s = new Student(100, "Jackie");
    s->print();
    delete s;
}

// ---------- 6. placement new：在已有内存上构造对象 ----------
void placement_new() {
    // 预先准备一块内存（这里用栈上的缓冲区）
    alignas(Widget) char buffer[sizeof(Widget) * 2];

    std::cout << "  在栈缓冲区上构造对象:\n";
    Widget* w1 = new (buffer) Widget;                        // placement new
    Widget* w2 = new (buffer + sizeof(Widget)) Widget;

    std::cout << "    w1 在 " << static_cast<void*>(w1)
              << ", buffer 在 " << static_cast<void*>(buffer) << "（相同）\n";

    std::cout << "  placement new 构造的对象必须手工调用析构函数:\n";
    w2->~Widget();       // 显式调用析构函数，不释放内存
    w1->~Widget();
    // 不能 delete w1！内存不是 operator new 分配的
    std::cout << "    （绝不能对它们用 delete）\n";
}

// ---------- 7. 只能创建堆对象 / 只能创建栈对象 ----------
// 只能建堆对象：把析构函数设为 private，提供 destroy 方法
class HeapOnly {
public:
    static HeapOnly* create() { return new HeapOnly; }
    void destroy() { delete this; }
    void use() { std::cout << "    HeapOnly::use()\n"; }
private:
    HeapOnly() { std::cout << "    HeapOnly() 构造\n"; }
    ~HeapOnly() { std::cout << "    ~HeapOnly() 析构\n"; }
};

// 只能建栈对象：把 operator new 设为 private
class StackOnly {
public:
    StackOnly() { std::cout << "    StackOnly() 构造\n"; }
    ~StackOnly() { std::cout << "    ~StackOnly() 析构\n"; }
    void use() { std::cout << "    StackOnly::use()\n"; }
private:
    void* operator new(std::size_t) = delete;        // [C++11] 更明确的写法
    void operator delete(void*) = delete;
};

void restricted_objects() {
    std::cout << "  只能建堆对象:\n";
    HeapOnly* h = HeapOnly::create();
    h->use();
    h->destroy();
    // HeapOnly stack_obj;    // 编译错误：构造/析构函数私有

    std::cout << "  只能建栈对象:\n";
    StackOnly s;
    s.use();
    // StackOnly* p = new StackOnly;   // 编译错误：operator new 被删除
}

int main() {
    std::cout << "--- 1. new vs malloc ---\n";        new_vs_malloc();
    std::cout << "--- 2. 初始化行为 ---\n";           new_initialization();
    std::cout << "--- 3. delete 配对 ---\n";          delete_pairing();
    std::cout << "--- 4. 分配失败 ---\n";             allocation_failure();
    std::cout << "--- 5. 重载 operator new ---\n";    operator_new_overload();
    std::cout << "--- 6. placement new ---\n";        placement_new();
    std::cout << "--- 7. 限制创建位置 ---\n";         restricted_objects();
}
