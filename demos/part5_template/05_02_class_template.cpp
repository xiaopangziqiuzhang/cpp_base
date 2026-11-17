// std=c++17
// 对应章节: blog/part5_template/05-02_class_template.md
// 主题: 类模板、成员函数定义、CTAD、变量模板、成员函数模板

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

// ---------- 1. 类模板的定义 ----------
template <class T, int kMax = 10>
class Stack {
public:
    Stack() : _top(-1), _base(new T[kMax]()) {}

    ~Stack() { delete[] _base; }

    // 禁止拷贝（简化示例；真实实现应该支持）
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    bool empty() const;          // 类内声明
    bool full() const;
    void push(const T& t);
    void pop();
    T& top();

    int size() const { return _top + 1; }   // 类内定义

private:
    int _top;
    T* _base;
};

// 类外定义成员函数：每个都要带完整的模板参数列表
template <class T, int kMax>
bool Stack<T, kMax>::empty() const { return _top == -1; }

template <class T, int kMax>
bool Stack<T, kMax>::full() const { return _top == kMax - 1; }

template <class T, int kMax>
void Stack<T, kMax>::push(const T& t) {
    if (!full()) {
        _base[++_top] = t;
    } else {
        std::cerr << "        栈已满，无法 push\n";
    }
}

template <class T, int kMax>
void Stack<T, kMax>::pop() {
    if (!empty()) {
        --_top;
    } else {
        std::cerr << "        栈为空，无法 pop\n";
    }
}

template <class T, int kMax>
T& Stack<T, kMax>::top() { return _base[_top]; }

void class_template_basics() {
    Stack<int, 3> si;
    si.push(1); si.push(2); si.push(3);
    std::cout << "      Stack<int,3>: size=" << si.size() << " top=" << si.top() << "\n";
    si.push(4);          // 超出容量
    si.pop();
    std::cout << "      pop 之后 top=" << si.top() << "\n";

    Stack<std::string> ss;     // 用默认的 kMax=10
    ss.push("hello");
    ss.push("world");
    std::cout << "      Stack<string>: size=" << ss.size() << " top=" << ss.top() << "\n";
}

// ---------- 2. 类模板的每个实例是独立的类型 ----------
void distinct_types() {
    std::cout << "      Stack<int> 和 Stack<double> 是同一个类型吗: "
              << std::is_same<Stack<int>, Stack<double>>::value << "\n";
    std::cout << "      Stack<int,10> 和 Stack<int,20> 是同一个类型吗: "
              << std::is_same<Stack<int, 10>, Stack<int, 20>>::value << "\n";
    std::cout << "      Stack<int> 和 Stack<int,10> 是同一个类型吗: "
              << std::is_same<Stack<int>, Stack<int, 10>>::value << "（默认参数相同）\n";
    std::cout << "      每组模板实参产生一个【独立的类】，静态成员也各自独立\n";
}

// ---------- 3. 类模板的静态成员：每个实例一份 ----------
template <class T>
class Counter {
public:
    Counter() { ++_count; }
    static int count() { return _count; }
private:
    static int _count;
};

template <class T>
int Counter<T>::_count = 0;      // 类外定义静态成员

void static_members() {
    Counter<int> a, b, c;
    Counter<double> d;
    std::cout << "      Counter<int>::count()    = " << Counter<int>::count() << "\n";
    std::cout << "      Counter<double>::count() = " << Counter<double>::count() << "\n";
    std::cout << "      每个实例化的类有独立的静态成员\n";
}

// ---------- 4. 成员函数模板（类模板里的模板，或普通类里的模板）----------
// 4a. 普通类里的成员函数模板
class CharArray {
public:
    explicit CharArray(const char* pstr)
        : _capacity(std::strlen(pstr) + 1), _buff(new char[_capacity]()) {
        std::strcpy(_buff, pstr);
    }
    ~CharArray() { delete[] _buff; }
    CharArray(const CharArray&) = delete;
    CharArray& operator=(const CharArray&) = delete;

    // 成员函数模板：使用时不用提供模板参数，自动推导
    template <class Iterator>
    void print_elements(Iterator beg, Iterator end) const {
        std::cout << "        ";
        while (beg != end) { std::cout << *beg; ++beg; }
        std::cout << "\n";
    }

    const char* c_str() const { return _buff; }
private:
    std::size_t _capacity;
    char* _buff;
};

// 4b. 类模板里的成员函数模板
template <class T>
class Wrapper {
public:
    explicit Wrapper(T v) : _value(std::move(v)) {}

    // 可以和任意类型的 Wrapper 比较
    template <class U>
    bool equals(const Wrapper<U>& other) const {
        return _value == other.get();
    }

    const T& get() const { return _value; }
private:
    T _value;
};

void member_templates() {
    CharArray ca("hello,world");
    std::cout << "      成员函数模板（自动推导迭代器类型）:\n";
    ca.print_elements(ca.c_str(), ca.c_str() + 5);
    std::vector<char> v{'a', 'b', 'c'};
    ca.print_elements(v.begin(), v.end());

    Wrapper<int> wi(42);
    Wrapper<double> wd(42.0);
    std::cout << "      Wrapper<int>(42).equals(Wrapper<double>(42.0)) = "
              << wi.equals(wd) << "\n";
}

// ---------- 5. CTAD：类模板实参推导 [C++17] ----------
template <class T>
class Box {
public:
    explicit Box(T value) : _value(std::move(value)) {}
    const T& get() const { return _value; }
private:
    T _value;
};

// 对于无法自动推导的情况，可以提供推导指引（deduction guide）
template <class T>
class Pair2 {
public:
    Pair2(T a, T b) : _a(std::move(a)), _b(std::move(b)) {}
    void print() const { std::cout << "        (" << _a << ", " << _b << ")\n"; }
private:
    T _a, _b;
};
// 推导指引：从两个不同类型推导出共同类型
template <class T, class U>
Pair2(T, U) -> Pair2<std::common_type_t<T, U>>;

void ctad() {
    // C++17 之前必须写出模板实参
    Box<int> b1(42);
    Box<std::string> b2(std::string("hello"));

    // C++17：从构造函数实参推导
    Box b3(42);                      // 推导为 Box<int>
    Box b4(std::string("hello"));    // 推导为 Box<std::string>
    std::cout << "      Box b3(42) 推导为 Box<int>: "
              << std::is_same<decltype(b3), Box<int>>::value << "\n";
    std::cout << "      Box b4(string) 推导为 Box<string>: "
              << std::is_same<decltype(b4), Box<std::string>>::value << "\n";

    // 标准库也受益
    std::vector v{1, 2, 3};               // vector<int>
    std::pair p(1, 2.5);                  // pair<int,double>
    auto up = std::make_unique<int>(42);  // make_xxx 函数在 CTAD 之后部分不再必要
    std::cout << "      std::vector v{1,2,3} 推导为 vector<int>: "
              << std::is_same<decltype(v), std::vector<int>>::value << "\n";
    std::cout << "      std::pair p(1, 2.5) 的类型正确: "
              << std::is_same<decltype(p), std::pair<int, double>>::value << "\n";

    // 推导指引生效
    Pair2 pr(1, 2.5);      // 两个实参类型不同，靠推导指引统一为 double
    pr.print();
    std::cout << "      通过推导指引，Pair2(1, 2.5) 推导为 Pair2<double>: "
              << std::is_same<decltype(pr), Pair2<double>>::value << "\n";
    (void)b1; (void)b2; (void)up;
}

// ---------- 6. 变量模板 [C++14] ----------
template <class T>
constexpr T pi = T(3.14159265358979323846L);

template <class T>
constexpr bool is_numeric = std::is_arithmetic<T>::value;

void variable_templates() {
    std::cout << "      pi<float>  = " << pi<float> << "\n";
    std::cout << "      pi<double> = " << pi<double> << "\n";
    std::cout << "      is_numeric<int> = " << is_numeric<int>
              << " is_numeric<std::string> = " << is_numeric<std::string> << "\n";
    std::cout << "      标准库的 _v 后缀就是变量模板: std::is_same_v<int,int> = "
              << std::is_same_v<int, int> << "\n";
}

// ---------- 7. 类模板的友元 ----------
template <class T>
class Secret {
public:
    explicit Secret(T v) : _value(v) {}
    // 声明一个模板函数为友元
    template <class U>
    friend void peek(const Secret<U>& s);
private:
    T _value;
};

template <class U>
void peek(const Secret<U>& s) {
    std::cout << "        peek 看到了私有成员: " << s._value << "\n";
}

void template_friend() {
    Secret<int> s(42);
    peek(s);
}

int main() {
    std::cout << "--- 1. 类模板基础 ---\n";          class_template_basics();
    std::cout << "--- 2. 独立的类型 ---\n";          distinct_types();
    std::cout << "--- 3. 静态成员 ---\n";            static_members();
    std::cout << "--- 4. 成员函数模板 ---\n";        member_templates();
    std::cout << "--- 5. CTAD [C++17] ---\n";        ctad();
    std::cout << "--- 6. 变量模板 [C++14] ---\n";    variable_templates();
    std::cout << "--- 7. 模板友元 ---\n";            template_friend();
}
