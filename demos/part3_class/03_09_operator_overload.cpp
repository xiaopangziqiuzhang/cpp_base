// std=c++17
// 对应章节: blog/part3_class/03-09_operator_overload.md
// 主题: 友元、运算符重载的三种形式、特殊运算符的重载

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ---------- 1. 友元函数：全局函数作友元 ----------
namespace friend_function {
    class Point {
    public:
        Point(int x, int y) : _x(x), _y(y) {}

        // 声明全局函数为友元，它能访问私有成员
        friend void print_point(const Point& p);
        friend std::ostream& operator<<(std::ostream& os, const Point& p);

    private:
        int _x, _y;
    };

    void print_point(const Point& p) {
        std::cout << "        friend 函数访问私有成员: (" << p._x << "," << p._y << ")\n";
    }

    std::ostream& operator<<(std::ostream& os, const Point& p) {
        return os << "(" << p._x << "," << p._y << ")";
    }

    void demo() {
        Point p(3, 4);
        print_point(p);
        std::cout << "        operator<< 也是友元: " << p << "\n";
        std::cout << "      友元【不是】成员函数，只是被授权访问私有成员\n";
    }
}

// ---------- 2. 友元成员函数与友元类 ----------
namespace friend_class {
    class Engine;      // 前向声明

    class Car {
    public:
        explicit Car(int power) : _power(power) {}
        // 只把 Engine 的一个成员函数设为友元
        friend void inspect_one(const Car& c);
    private:
        int _power;
    };

    void inspect_one(const Car& c) {
        std::cout << "        单个友元函数看到 _power = " << c._power << "\n";
    }

    class Database;

    class Record {
    public:
        explicit Record(int id) : _id(id) {}
        friend class Database;      // 整个 Database 类都是友元
    private:
        int _id;
        std::string _secret = "内部数据";
    };

    class Database {
    public:
        void dump(const Record& r) const {
            // 可以访问 Record 的全部私有成员
            std::cout << "        友元类访问: id=" << r._id
                      << " secret=" << r._secret << "\n";
        }
    };

    void demo() {
        Car c(150);
        inspect_one(c);
        Record r(42);
        Database db;
        db.dump(r);

        std::cout << "      友元的三个性质:\n";
        std::cout << "        1. 不对称 —— A 是 B 的友元，不代表 B 是 A 的友元\n";
        std::cout << "        2. 不传递 —— 友元的友元不是我的友元\n";
        std::cout << "        3. 不继承 —— \"朋友的孩子不是我的朋友\"（见 04-01）\n";
        std::cout << "      代价: 破坏封装。友元类尤其危险，开放了全部私有成员\n";
    }
}

// ---------- 3. 为什么需要运算符重载 ----------
namespace motivation {
    class Complex {
    public:
        Complex(double re = 0, double im = 0) : _re(re), _im(im) {}

        // 不重载运算符时，只能用函数名
        Complex add(const Complex& rhs) const { return Complex(_re + rhs._re, _im + rhs._im); }
        Complex mul(const Complex& rhs) const {
            return Complex(_re * rhs._re - _im * rhs._im, _re * rhs._im + _im * rhs._re);
        }

        // 重载运算符后
        Complex operator+(const Complex& rhs) const { return add(rhs); }
        Complex operator*(const Complex& rhs) const { return mul(rhs); }

        void print(const char* label) const {
            std::cout << "        " << label << " = " << _re
                      << (_im >= 0 ? "+" : "") << _im << "i\n";
        }
    private:
        double _re, _im;
    };

    void demo() {
        Complex a(1, 2), b(3, 4), c(5, 6);

        // 没有运算符重载：嵌套函数调用，读起来像倒着念
        Complex r1 = a.add(b).mul(c).add(a);
        r1.print("a.add(b).mul(c).add(a)");

        // 有运算符重载：和数学写法一致
        Complex r2 = (a + b) * c + a;
        r2.print("(a + b) * c + a       ");

        std::cout << "      运算符重载的价值: 让自定义类型用起来像内置类型\n";
        std::cout << "      标准库大量依赖它: string 的 +、vector 的 []、\n";
        std::cout << "                        iostream 的 <<、智能指针的 * 和 ->\n";
    }
}

// ---------- 4. 重载的规则 ----------
namespace rules {
    void demo() {
        std::cout << "      可以重载的运算符:\n";
        std::cout << "        算术: + - * / % ++ --\n";
        std::cout << "        关系: == != < > <= >=\n";
        std::cout << "        逻辑: && || !\n";
        std::cout << "        位:   & | ^ ~ << >>\n";
        std::cout << "        赋值: = += -= *= /= %= &= |= ^= <<= >>=\n";
        std::cout << "        其他: [] () -> ->* , new delete\n";

        std::cout << "      【不能】重载的运算符:\n";
        std::cout << "        ::    作用域解析（编译期概念，不作用于对象）\n";
        std::cout << "        .     成员访问（会让 obj.member 的含义不确定）\n";
        std::cout << "        .*    成员指针访问\n";
        std::cout << "        ?:    三目（它的短路语义无法用函数表达）\n";
        std::cout << "        sizeof / alignof / typeid（编译期运算符）\n";

        std::cout << "      其他限制:\n";
        std::cout << "        1. 不能创造新运算符（如 ** 做幂运算）\n";
        std::cout << "        2. 不能改变优先级和结合性\n";
        std::cout << "        3. 不能改变操作数个数（除了 () ）\n";
        std::cout << "        4. 至少有一个操作数是自定义类型\n";
        std::cout << "        5. = [] () -> 必须是【成员函数】\n";
        std::cout << "        6. && || , 重载后会【失去短路/顺序语义】—— 所以别重载\n";
    }
}

// ---------- 5. 三种重载形式 ----------
namespace three_forms {
    class Money {
    public:
        Money(long yuan = 0, int cent = 0) : _total_cent(yuan * 100 + cent) {}

        // 形式一：成员函数
        //   左操作数必须是本类对象（隐含的 this）
        Money operator+(const Money& rhs) const {
            return from_cent(_total_cent + rhs._total_cent);
        }

        // 形式二：友元函数
        //   两个操作数都显式，所以支持左操作数是其他类型
        friend Money operator*(int factor, const Money& m) {
            return from_cent(factor * m._total_cent);
        }
        friend Money operator*(const Money& m, int factor) {
            return from_cent(m._total_cent * factor);
        }

        friend std::ostream& operator<<(std::ostream& os, const Money& m) {
            return os << (m._total_cent / 100) << "." << (m._total_cent % 100 < 10 ? "0" : "")
                      << (m._total_cent % 100) << " 元";
        }

        long cent() const { return _total_cent; }
        static Money from_cent(long c) { Money m; m._total_cent = c; return m; }

    private:
        long _total_cent;
    };

    // 形式三：普通（非友元）全局函数
    //   只能用公开接口，不能访问私有成员
    bool operator==(const Money& a, const Money& b) { return a.cent() == b.cent(); }
    bool operator<(const Money& a, const Money& b)  { return a.cent() < b.cent(); }

    void demo() {
        Money a(10, 50), b(5, 25);

        std::cout << "      成员函数形式 a + b = " << (a + b) << "\n";

        // 这是友元函数存在的关键理由：左操作数不是本类对象
        std::cout << "      友元函数形式 3 * a = " << (3 * a) << "\n";
        std::cout << "      友元函数形式 a * 3 = " << (a * 3) << "\n";
        std::cout << "      （成员函数做不到 3 * a，因为左操作数必须是 Money）\n";

        std::cout << "      普通全局函数 (a == b) = " << (a == b)
                  << " (b < a) = " << (b < a) << "\n";

        std::cout << "      选择原则:\n";
        std::cout << "        改变自身状态的（= += ++）-> 成员函数\n";
        std::cout << "        对称的二元运算（+ - == <）-> 非成员函数（最好是普通函数）\n";
        std::cout << "        需要访问私有成员的非成员 -> 友元\n";
        std::cout << "        左操作数可能是其他类型 -> 必须是非成员\n";
    }
}

// ---------- 6. 自增自减：前置与后置的区分 ----------
namespace increment {
    class Counter {
    public:
        explicit Counter(int v = 0) : _value(v) {}

        // 前置 ++：返回【引用】，效率更高
        Counter& operator++() {
            ++_value;
            return *this;
        }

        // 后置 ++：用一个哑元 int 参数区分，返回【旧值的副本】
        Counter operator++(int) {
            Counter old = *this;      // 必须保存旧值
            ++_value;
            return old;                // 返回副本 -> 比前置版本多一次拷贝
        }

        Counter& operator--() { --_value; return *this; }
        Counter operator--(int) { Counter old = *this; --_value; return old; }

        int value() const { return _value; }
    private:
        int _value;
    };

    void demo() {
        Counter c(5);
        std::cout << "      初始值 " << c.value() << "\n";
        std::cout << "      ++c 返回 " << (++c).value() << "，c 现在是 " << c.value() << "\n";

        Counter d(5);
        std::cout << "      d++ 返回 " << (d++).value() << "，d 现在是 " << d.value() << "\n";

        std::cout << "      区分方式: 后置版本多一个【哑元 int 参数】（不使用，只用于重载决议）\n";
        std::cout << "      性能差别: 前置返回引用，后置要构造并返回旧值的副本\n";
        std::cout << "      所以循环里优先用 ++it 而不是 it++（对迭代器尤其明显）\n";
    }
}

// ---------- 7. 函数调用运算符：仿函数 ----------
namespace call_operator {
    // 有状态的仿函数
    class Accumulator {
    public:
        explicit Accumulator(int init = 0) : _sum(init) {}
        int operator()(int x) { _sum += x; return _sum; }   // 可以有状态
        int total() const { return _sum; }
    private:
        int _sum;
    };

    // 比较器：STL 大量使用这个模式
    struct ByLength {
        bool operator()(const std::string& a, const std::string& b) const {
            return a.size() != b.size() ? a.size() < b.size() : a < b;
        }
    };

    void demo() {
        Accumulator acc(100);
        std::cout << "      有状态的仿函数: " << acc(10) << " " << acc(20)
                  << " " << acc(5) << "\n";
        std::cout << "        （lambda 的 mutable 捕获就是这个，见 06-01）\n";

        // 作为模板参数传给容器
        std::map<std::string, int, ByLength> sorted_by_len{
            {"ccc", 3}, {"a", 1}, {"bb", 2}};
        std::cout << "      用仿函数做 map 的比较器（按长度排序）: ";
        for (const auto& [k, v] : sorted_by_len) std::cout << k << " ";
        std::cout << "\n";
        std::cout << "      operator() 是唯一可以有【任意多个参数】的运算符\n";
        std::cout << "      它让对象能像函数一样调用 —— lambda 的底层就是它（见 06-01）\n";
    }
}

// ---------- 8. 下标运算符：const 与非 const 成对 ----------
namespace subscript {
    class IntArray {
    public:
        explicit IntArray(std::size_t n) : _data(n, 0) {}

        // 非 const 版本：返回可写引用
        int& operator[](std::size_t i) { return _data[i]; }
        // const 版本：返回只读引用
        const int& operator[](std::size_t i) const { return _data[i]; }

        // at() 做边界检查
        int& at(std::size_t i) {
            if (i >= _data.size()) throw std::out_of_range("IntArray::at");
            return _data[i];
        }

        std::size_t size() const { return _data.size(); }
    private:
        std::vector<int> _data;
    };

    void demo() {
        IntArray arr(5);
        arr[0] = 10;                 // 非 const 版本
        arr[1] = 20;

        const IntArray& carr = arr;
        std::cout << "      非 const 对象可写: arr[0] = " << arr[0] << "\n";
        std::cout << "      const 对象只读:    carr[0] = " << carr[0] << "\n";
        // carr[0] = 1;              // 编译错误：返回 const int&

        try {
            arr.at(100);
        } catch (const std::out_of_range& e) {
            std::cout << "      at(100) 抛出 out_of_range（operator[] 不检查）\n";
        }

        std::cout << "      要点:\n";
        std::cout << "        1. 必须是成员函数\n";
        std::cout << "        2. 要提供 const 和非 const 两个版本（见 03-06）\n";
        std::cout << "        3. 返回【引用】才能作为赋值目标\n";
        std::cout << "        4. C++23 之前只能接受一个参数（多维要用 operator()）\n";
    }
}

// ---------- 9. 成员访问运算符：智能指针的核心 ----------
namespace member_access {
    class Resource {
    public:
        explicit Resource(std::string name) : _name(std::move(name)) {}
        void use() const { std::cout << "        Resource::use() on " << _name << "\n"; }
        const std::string& name() const { return _name; }
    private:
        std::string _name;
    };

    // 简化版智能指针，演示 * 和 -> 的重载
    template <class T>
    class SimplePtr {
    public:
        explicit SimplePtr(T* p = nullptr) : _ptr(p) {}
        ~SimplePtr() { delete _ptr; }

        SimplePtr(const SimplePtr&) = delete;
        SimplePtr& operator=(const SimplePtr&) = delete;

        T& operator*() const { return *_ptr; }       // 解引用
        T* operator->() const { return _ptr; }        // 成员访问

        explicit operator bool() const { return _ptr != nullptr; }   // 见下一章

    private:
        T* _ptr;
    };

    void demo() {
        SimplePtr<Resource> p(new Resource("res-1"));

        (*p).use();              // 通过 operator*
        p->use();                // 通过 operator->
        std::cout << "      p->name() = " << p->name() << "\n";

        if (p) std::cout << "      通过 explicit operator bool 判空\n";

        std::cout << "      operator-> 的特殊之处:\n";
        std::cout << "        它返回一个指针，然后编译器【自动继续应用 ->】\n";
        std::cout << "        所以 p->use() 实际是 (p.operator->())->use()\n";
        std::cout << "        这个\"链式解引用\"机制让智能指针能透明代理\n";
    }
}

// ---------- 10. 流运算符：为什么必须是非成员 ----------
namespace stream_operators {
    class Date {
    public:
        Date(int y = 1970, int m = 1, int d = 1) : _y(y), _m(m), _d(d) {}

        // 必须是非成员函数：左操作数是 ostream，不是 Date
        friend std::ostream& operator<<(std::ostream& os, const Date& date) {
            return os << date._y << "-"
                      << (date._m < 10 ? "0" : "") << date._m << "-"
                      << (date._d < 10 ? "0" : "") << date._d;
        }

        friend std::istream& operator>>(std::istream& is, Date& date) {
            is >> date._y >> date._m >> date._d;
            if (!is) date = Date{};        // 失败时置为已知状态（见 07-07）
            return is;
        }
    private:
        int _y, _m, _d;
    };

    void demo() {
        Date d(2024, 1, 15);
        std::cout << "      operator<< 输出: " << d << "\n";

        std::istringstream iss("2025 12 31");
        Date parsed;
        iss >> parsed;
        std::cout << "      operator>> 解析: " << parsed << "\n";

        // 链式输出依赖返回 ostream&
        std::cout << "      链式调用: " << d << " 和 " << parsed << "\n";

        std::cout << "      要点:\n";
        std::cout << "        1. 【必须】是非成员函数（左操作数是流，不是你的类）\n";
        std::cout << "        2. 返回 ostream& / istream& 才能链式调用\n";
        std::cout << "        3. 需要访问私有成员时声明为友元\n";
        std::cout << "        4. 直接在类内定义友元最省事（避免模板的各种语法陷阱）\n";
        std::cout << "        5. operator<< 的参数应是 const T&，operator>> 是 T&\n";
    }
}

int main() {
    std::cout << "--- 1. 友元函数 ---\n";            friend_function::demo();
    std::cout << "--- 2. 友元类 ---\n";              friend_class::demo();
    std::cout << "--- 3. 为什么要重载 ---\n";        motivation::demo();
    std::cout << "--- 4. 重载的规则 ---\n";          rules::demo();
    std::cout << "--- 5. 三种形式 ---\n";            three_forms::demo();
    std::cout << "--- 6. 自增自减 ---\n";            increment::demo();
    std::cout << "--- 7. 函数调用运算符 ---\n";      call_operator::demo();
    std::cout << "--- 8. 下标运算符 ---\n";          subscript::demo();
    std::cout << "--- 9. 成员访问运算符 ---\n";      member_access::demo();
    std::cout << "--- 10. 流运算符 ---\n";           stream_operators::demo();
}
