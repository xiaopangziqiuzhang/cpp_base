// std=c++17
// 对应章节: blog/part1_basics/01-11_uniform_init.md
// 主题: 统一初始化、initializer_list、最烦人的解析、窄化转换

#include <initializer_list>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// ---------- 1. C++98 的初始化方式有多少种 ----------
struct Aggregate { int a; double b; };

void old_initialization() {
    int x1 = 1;              // 拷贝初始化
    int x2(2);               // 直接初始化
    int arr[] = {1, 2, 3};   // 聚合初始化（只能用于数组/聚合类型）
    Aggregate ag = {1, 2.5}; // 聚合初始化
    std::vector<int> v(3, 7);// 构造函数，3 个 7
    int* p = new int(5);     // new 表达式

    std::cout << "  x1=" << x1 << " x2=" << x2 << " arr[0]=" << arr[0]
              << " ag.a=" << ag.a << " v.size()=" << v.size() << " *p=" << *p << "\n";
    delete p;
}

// ---------- 2. C++11：一种语法覆盖所有场合 ----------
class Point {
public:
    Point(int x, int y) : _x(x), _y(y) {}
    int x() const { return _x; }
    int y() const { return _y; }
private:
    int _x, _y;
};

void uniform_initialization() {
    int x{1};                       // 基本类型
    int arr[]{1, 2, 3};             // 数组
    Aggregate ag{1, 2.5};           // 聚合类型
    Point pt{3, 4};                 // 有构造函数的类
    std::vector<int> v{1, 2, 3};    // 标准容器
    std::map<std::string, int> m{{"a", 1}, {"b", 2}};   // 嵌套
    int* p = new int{5};            // new 表达式

    std::cout << "  x=" << x << " arr[2]=" << arr[2] << " ag.b=" << ag.b
              << " pt=(" << pt.x() << "," << pt.y() << ")"
              << " v.size()=" << v.size() << " m[\"a\"]=" << m["a"]
              << " *p=" << *p << "\n";
    delete p;
}

// ---------- 3. 窄化转换：{} 会拒绝丢精度的隐式转换 ----------
void narrowing_conversion() {
    double d = 3.99;

    int a = d;        // 允许，静默截断为 3
    int b(d);         // 允许，静默截断
    // int c{d};      // 编译错误: narrowing conversion of '3.99' from 'double' to 'int'

    std::cout << "  int a = 3.99  -> " << a << "（静默截断）\n";
    std::cout << "  int b(3.99)   -> " << b << "（静默截断）\n";
    std::cout << "  int c{3.99}   -> 编译错误，{} 拒绝窄化转换\n";

    // 常量表达式且值能精确表示时，{} 是允许的
    int ok{3};
    char ch{65};           // 65 能被 char 精确表示
    // char bad{300};      // 编译错误：300 超出 char 范围
    std::cout << "  int ok{3}=" << ok << " char ch{65}='" << ch << "'\n";
}

// ---------- 4. 最烦人的解析（most vexing parse）----------
struct Widget {
    Widget() { std::cout << "  Widget()\n"; }
    void use() const { std::cout << "  Widget::use()\n"; }
};

void most_vexing_parse() {
    // Widget w();     // 这不是定义对象！被解析成"声明一个返回 Widget 的无参函数"
    // w.use();        // 编译错误: request for member 'use' in 'w', which is of non-class type

    Widget w1;         // 正确：无参构造
    Widget w2{};       // 正确：{} 不会有这个歧义
    w1.use();
    w2.use();

    // 更经典的版本：想用迭代器区间构造 vector
    // std::vector<int> v(std::istream_iterator<int>(std::cin), std::istream_iterator<int>());
    //   -> 被解析成函数声明
    // 用 {} 就没这个问题
}

// ---------- 5. initializer_list：{} 背后的类型 ----------
class Collection {
public:
    // 接受任意个同类型元素
    Collection(std::initializer_list<int> list) : _data(list) {
        std::cout << "  Collection(initializer_list), size=" << list.size() << "\n";
    }
    // 普通构造函数
    Collection(int count, int value) : _data(count, value) {
        std::cout << "  Collection(int,int), size=" << _data.size() << "\n";
    }
    std::size_t size() const { return _data.size(); }
private:
    std::vector<int> _data;
};

void initializer_list_demo() {
    Collection c1{1, 2, 3};    // 调 initializer_list 版本
    Collection c2(3, 7);       // 调 (int,int) 版本
    std::cout << "  c1.size()=" << c1.size() << " c2.size()=" << c2.size() << "\n";

    // 自己写函数接受 initializer_list
    auto sum = [](std::initializer_list<int> list) {
        int total = 0;
        for (int v : list) total += v;     // 可以范围 for
        return total;
    };
    std::cout << "  sum({1,2,3,4,5}) = " << sum({1, 2, 3, 4, 5}) << "\n";
}

// ---------- 6. 陷阱：initializer_list 构造函数优先级极高 ----------
void vector_trap() {
    std::vector<int> v1(3, 7);   // 3 个 7     -> [7,7,7]
    std::vector<int> v2{3, 7};   // 两个元素   -> [3,7]

    std::cout << "  vector<int> v1(3,7): size=" << v1.size() << " 内容:";
    for (int x : v1) std::cout << " " << x;
    std::cout << "\n  vector<int> v2{3,7}: size=" << v2.size() << " 内容:";
    for (int x : v2) std::cout << " " << x;
    std::cout << "\n";

    // 只有 initializer_list 完全无法匹配时，才考虑其他构造函数
    std::vector<std::string> s1(3, "x");   // 3 个 "x"
    std::vector<std::string> s2{3, "x"};   // 编译错误？不是——3 不能转 string，
                                           // 所以退回到 (count, value) 版本
    std::cout << "  vector<string> s1(3,\"x\"): size=" << s1.size() << "\n";
    std::cout << "  vector<string> s2{3,\"x\"}: size=" << s2.size()
              << "（3 无法转 string，退回普通构造函数）\n";
}

// ---------- 7. {} 用于类成员初始值 [C++11] ----------
class Config {
public:
    Config() = default;                       // 用下面的类内初始值
    Config(int t) : timeout(t) {}              // 覆盖 timeout
    void print() const {
        std::cout << "  timeout=" << timeout << " retries=" << retries
                  << " name=" << name << " tags.size()=" << tags.size() << "\n";
    }
private:
    // 类内成员初始值 [C++11]：不必在每个构造函数里重复写
    int timeout{30};
    int retries{3};
    std::string name{"default"};
    std::vector<int> tags{1, 2, 3};
};

void member_init() {
    Config c1;
    Config c2(60);
    c1.print();
    c2.print();
}

// ---------- 8. 返回值与函数实参里的 {} ----------
Point make_point() { return {7, 8}; }             // 不必写类型名
void takes_point(const Point& p) {
    std::cout << "  takes_point: (" << p.x() << "," << p.y() << ")\n";
}

void in_expressions() {
    Point p = make_point();
    std::cout << "  make_point() 返回 (" << p.x() << "," << p.y() << ")\n";
    takes_point({9, 10});                          // 实参里直接写 {}
}

int main() {
    std::cout << "--- 1. C++98 的多种初始化 ---\n";  old_initialization();
    std::cout << "--- 2. 统一初始化 ---\n";          uniform_initialization();
    std::cout << "--- 3. 窄化转换 ---\n";            narrowing_conversion();
    std::cout << "--- 4. 最烦人的解析 ---\n";        most_vexing_parse();
    std::cout << "--- 5. initializer_list ---\n";    initializer_list_demo();
    std::cout << "--- 6. vector 陷阱 ---\n";         vector_trap();
    std::cout << "--- 7. 类内成员初始值 ---\n";      member_init();
    std::cout << "--- 8. 表达式中的 {} ---\n";       in_expressions();
}
