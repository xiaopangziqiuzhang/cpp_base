// std=c++17 flags=-pthread
// 对应章节: blog/part3_class/03-06_const_member.md
// 主题: const 成员函数的本质、const 重载、mutable、const 正确性

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

// ---------- 1. const 成员函数的本质：改变 this 的类型 ----------
class Probe {
public:
    void non_const() {
        // this 的类型是 Probe* const
        std::cout << "      non_const(): this 是 Probe* const\n";
        std::cout << "        decltype(this) 是 const Probe* 吗: "
                  << std::is_same<decltype(this), const Probe*>::value << "\n";
    }

    void is_const() const {
        // this 的类型是 const Probe* const
        std::cout << "      is_const() const: this 是 const Probe* const\n";
        std::cout << "        decltype(this) 是 const Probe* 吗: "
                  << std::is_same<decltype(this), const Probe*>::value << "\n";
        // _v = 1;          // 编译错误：通过 const 指针修改
        // non_const();     // 编译错误：不能调用非 const 成员函数
    }

private:
    int _v = 0;
};

void const_essence() {
    Probe p;
    p.non_const();
    p.is_const();
    std::cout << "      所谓\"const 成员函数不能改成员\"，就是 const 指针的常规行为\n";
}

// ---------- 2. const 对象只能调用 const 成员函数 ----------
class Computer {
public:
    Computer(std::string brand, double price)
        : _brand(std::move(brand)), _price(price) {}

    void print() const {         // const：不修改状态
        std::cout << "      品牌:" << _brand << " 价格:" << _price << "\n";
    }
    void setPrice(double p) { _price = p; }     // 非 const：修改状态

private:
    std::string _brand;
    double _price;
};

void const_object() {
    Computer pc("Huawei", 5699);
    pc.print();
    pc.setPrice(5999);
    pc.print();

    const Computer cpc("ThinkPad", 9999);
    cpc.print();                 // ok：const 成员函数
    // cpc.setPrice(1);          // 编译错误：passing 'const Computer' as 'this'
    std::cout << "      const 对象只能调用 const 成员函数（构造/析构除外）\n";
}

// ---------- 3. const 与非 const 的重载 ----------
class Buffer {
public:
    explicit Buffer(std::size_t n) : _data(n, 0) {}

    // 非 const 版本：返回可写引用
    int& operator[](std::size_t i) {
        std::cout << "      [非const operator[]] ";
        return _data[i];
    }
    // const 版本：返回只读引用
    const int& operator[](std::size_t i) const {
        std::cout << "      [const operator[]] ";
        return _data[i];
    }

    // begin/end 同理
    std::vector<int>::iterator begin() { return _data.begin(); }
    std::vector<int>::const_iterator begin() const { return _data.begin(); }

private:
    std::vector<int> _data;
};

void const_overload() {
    Buffer b(3);
    b[0] = 42;                        // 非 const 版本，可以赋值
    std::cout << "写入 42\n";

    const Buffer& cb = b;
    int v = cb[0];                     // const 版本
    std::cout << "读到 " << v << "\n";
    // cb[0] = 1;                      // 编译错误：返回 const int&

    std::cout << "      标准库到处是这个模式：operator[]、at、front、back、begin、end\n";
}

// ---------- 4. 避免 const/非 const 版本的代码重复 ----------
class Table {
public:
    // 非 const 版本委托给 const 版本（C++ 的惯用写法）
    int* find(const std::string& key) {
        // 先用 const 版本，再 const_cast 去掉 const
        return const_cast<int*>(static_cast<const Table&>(*this).find(key));
    }

    const int* find(const std::string& key) const {
        auto it = _data.find(key);
        return it == _data.end() ? nullptr : &it->second;
    }

    void insert(const std::string& k, int v) { _data[k] = v; }

private:
    std::map<std::string, int> _data;
};

void avoid_duplication() {
    Table t;
    t.insert("a", 1);

    if (int* p = t.find("a")) {
        *p = 100;                        // 非 const 版本返回可写指针
        std::cout << "      非 const find 可以修改: " << *p << "\n";
    }
    const Table& ct = t;
    if (const int* p = ct.find("a")) {
        std::cout << "      const find 只能读: " << *p << "\n";
    }
    std::cout << "      惯用法：非 const 版本转调 const 版本，再 const_cast 去 const\n";
    std::cout << "      （反方向不安全：const 版本不能调非 const 版本）\n";
}

// ---------- 5. mutable：逻辑常量性 ----------
class CachedValue {
public:
    explicit CachedValue(int base) : _base(base) {}

    // 逻辑上是只读查询，但内部要更新缓存和计数
    int compute() const {
        ++_call_count;                   // mutable
        if (!_cached) {
            _cache = _base * _base;      // mutable
            _cached = true;
            std::cout << "      （首次计算）";
        } else {
            std::cout << "      （用缓存）";
        }
        return _cache;
    }

    int call_count() const { return _call_count; }

private:
    int _base;
    mutable int _cache = 0;
    mutable bool _cached = false;
    mutable int _call_count = 0;
};

void mutable_demo() {
    const CachedValue c(12);             // 注意是 const 对象
    std::cout << "compute()=" << c.compute() << "\n";
    std::cout << "compute()=" << c.compute() << "\n";
    std::cout << "      调用次数: " << c.call_count() << "\n";
    std::cout << "      mutable 表达的是\"逻辑上的常量性\"：对外不变，内部实现可变\n";
}

// ---------- 6. mutable 最正当的用途：在 const 成员函数里加锁 ----------
class ThreadSafeCounter {
public:
    void increment() {
        std::lock_guard<std::mutex> lk(_mtx);
        ++_value;
    }

    int get() const {
        std::lock_guard<std::mutex> lk(_mtx);   // 必须 mutable，否则无法 lock
        return _value;
    }

private:
    mutable std::mutex _mtx;      // 不是 mutable 的话，const 成员函数无法加锁
    int _value = 0;
};

void mutable_mutex() {
    ThreadSafeCounter c;
    c.increment();
    c.increment();
    std::cout << "      get() 是 const，但需要 lock -> _mtx 必须是 mutable\n";
    std::cout << "      当前值: " << c.get() << "\n";
}

// ---------- 7. const 成员函数的常见误区：返回内部指针 ----------
class Leaky {
public:
    // 看起来是 const，实际上泄漏了可写访问
    int* data() const { return _data; }     // 返回非 const 指针！
private:
    int* _data = nullptr;
};

class Correct {
public:
    const int* data() const { return _data; }   // const 版本返回 const 指针
    int* data() { return _data; }
private:
    int* _data = nullptr;
};

void const_leak() {
    std::cout << "      陷阱：const 成员函数只保证不改【指针本身】，\n";
    std::cout << "            不保证不改【指针指向的内容】\n";
    std::cout << "      所以 int* data() const 虽然合法，但破坏了 const 正确性\n";
    std::cout << "      正确做法：const 版本返回 const int*\n";
}

int main() {
    std::cout << "--- 1. const 成员函数的本质 ---\n";  const_essence();
    std::cout << "--- 2. const 对象 ---\n";            const_object();
    std::cout << "--- 3. const 重载 ---\n";            const_overload();
    std::cout << "--- 4. 避免重复 ---\n";              avoid_duplication();
    std::cout << "--- 5. mutable ---\n";               mutable_demo();
    std::cout << "--- 6. mutable mutex ---\n";         mutable_mutex();
    std::cout << "--- 7. const 泄漏 ---\n";            const_leak();
}
