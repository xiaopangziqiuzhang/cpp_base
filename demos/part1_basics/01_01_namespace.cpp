// std=c++17
// 对应章节: blog/part1_basics/01-01_namespace.md
// 主题: 命名空间的三种用法、匿名空间、嵌套简化、inline namespace、ADL

#include <iostream>
#include <string>
#include <vector>

// ---------- 1. 基本定义：命名空间里可以放什么 ----------
namespace wd {
    int number = 0;              // 变量
    const double kPi = 3.14159;  // 常量

    struct Foo {                 // 类型
        char ch;
        int val;
    };

    void display();              // 函数声明

    template <class T>           // 模板
    T twice(T t) { return t + t; }

    namespace inner {            // 嵌套命名空间
        int number = 100;
    }
}

void wd::display() {  // 在命名空间外定义，用限定名
    std::cout << "wd::display(), number=" << number << "\n";
}

// ---------- 2. 嵌套命名空间的简化写法 [C++17] ----------
// C++98 写法
namespace company {
    namespace project {
        namespace module {
            int version = 1;
        }
    }
}

// C++17 写法：一行搞定
namespace company::project::module2 {
    int version = 2;
}

// ---------- 3. 匿名命名空间：取代 static 的文件内链接 ----------
namespace {
    int file_local_counter = 0;   // 只在本文件可见，具有内部链接
    void bump() { ++file_local_counter; }
}

// ---------- 4. 命名空间别名：缩短冗长的限定名 ----------
namespace very_long_library_name_v2 {
    void work() { std::cout << "library work()\n"; }
}
namespace lib = very_long_library_name_v2;  // 别名

// ---------- 5. 嵌套与遮蔽 ----------
int number = 1;  // 全局

namespace outer {
    int number = 10;

    namespace nested {
        int number = 100;
        void show() {
            std::cout << "nested::number   = " << number << "\n";
            std::cout << "outer::number    = " << outer::number << "\n";
            std::cout << "::number(global) = " << ::number << "\n";  // :: 显式访问全局
        }
    }
}

// ---------- 6. ADL（参数依赖查找）：为什么 cout << 能不加 std:: ----------
namespace geometry {
    struct Vec2 { double x, y; };

    // 这个 operator<< 定义在 geometry 里，不是 std 里
    std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        return os << "(" << v.x << ", " << v.y << ")";
    }
}

void adl_demo() {
    geometry::Vec2 v{1.5, 2.5};
    // 没有写 geometry::operator<<，编译器通过实参类型 Vec2 找到了 geometry 命名空间
    std::cout << "ADL found operator<<: " << v << "\n";
}

// ---------- 7. inline namespace：库的版本演进 [C++11] ----------
namespace mylib {
    namespace v1 {
        const char* api() { return "v1::api"; }
    }
    inline namespace v2 {          // inline：成员会被提升到外层
        const char* api() { return "v2::api"; }
    }
}

void inline_namespace_demo() {
    // mylib::api() 直接解析到 inline 的那个版本
    std::cout << "mylib::api()     = " << mylib::api() << "\n";
    std::cout << "mylib::v1::api() = " << mylib::v1::api() << "\n";
    std::cout << "mylib::v2::api() = " << mylib::v2::api() << "\n";
}

// ---------- 8. using 声明 vs using 指令 ----------
void using_demo() {
    // using 声明：只引入单个名字，作用域从此处到块结束
    using std::cout;
    using std::endl;
    cout << "using declaration works" << endl;

    // using 指令：引入整个命名空间，污染面大
    // using namespace std;  // 不推荐，尤其不要放在头文件或全局作用域
}

int main() {
    std::cout << "--- 1. basic ---\n";
    wd::display();
    std::cout << "wd::kPi = " << wd::kPi << ", wd::inner::number = " << wd::inner::number << "\n";
    std::cout << "wd::twice(21) = " << wd::twice(21) << "\n";

    std::cout << "--- 2. nested [C++17] ---\n";
    std::cout << "company::project::module::version  = "
              << company::project::module::version << "\n";
    std::cout << "company::project::module2::version = "
              << company::project::module2::version << "\n";

    std::cout << "--- 3. anonymous ---\n";
    bump(); bump();
    std::cout << "file_local_counter = " << file_local_counter << "\n";

    std::cout << "--- 4. alias ---\n";
    lib::work();

    std::cout << "--- 5. shadowing ---\n";
    outer::nested::show();

    std::cout << "--- 6. ADL ---\n";
    adl_demo();

    std::cout << "--- 7. inline namespace ---\n";
    inline_namespace_demo();

    std::cout << "--- 8. using ---\n";
    using_demo();
}
