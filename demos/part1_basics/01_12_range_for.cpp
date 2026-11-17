// std=c++17
// 对应章节: blog/part1_basics/01-12_range_for.md
// 主题: 范围 for、自定义类型适配、结构化绑定、if/switch 初始化语句

#include <array>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// ---------- 1. C++98 vs C++11 的遍历 ----------
void traversal_evolution() {
    std::vector<int> v{1, 2, 3, 4, 5};

    std::cout << "  C++98 下标:";
    for (std::size_t i = 0; i < v.size(); ++i) std::cout << " " << v[i];
    std::cout << "\n";

    std::cout << "  C++98 迭代器:";
    for (std::vector<int>::const_iterator it = v.begin(); it != v.end(); ++it)
        std::cout << " " << *it;
    std::cout << "\n";

    std::cout << "  C++11 范围 for:";
    for (int x : v) std::cout << " " << x;
    std::cout << "\n";
}

// ---------- 2. 三种捕获方式 ----------
struct Heavy {
    std::vector<int> data = std::vector<int>(100, 1);
    int id = 0;
};

void three_forms() {
    std::vector<Heavy> items(3);

    for (auto item : items)       { item.id = 1; }   // 拷贝，改不到原对象
    std::cout << "  for (auto x : c)       -> items[0].id = " << items[0].id << "\n";

    for (auto& item : items)      { item.id = 2; }   // 引用，能改
    std::cout << "  for (auto& x : c)      -> items[0].id = " << items[0].id << "\n";

    int sum = 0;
    for (const auto& item : items){ sum += item.id; } // 只读，首选
    std::cout << "  for (const auto& x : c) -> sum = " << sum << "\n";
}

// ---------- 3. 范围 for 的等价展开 ----------
void how_it_works() {
    std::vector<int> v{10, 20, 30};

    // for (auto x : v) { body }  大致等价于：
    {
        auto&& range = v;
        auto it = std::begin(range);
        auto end = std::end(range);
        for (; it != end; ++it) {
            auto x = *it;
            std::cout << "  手工展开得到: " << x << "\n";
        }
    }
    // 注意：end 只在循环开始时求值一次
}

// ---------- 4. 陷阱：循环中修改容器导致迭代器失效 ----------
void invalidation_trap() {
    std::vector<int> v{1, 2, 3};
    std::cout << "  循环体内 push_back 会使迭代器失效（未定义行为），不要这么做\n";

    // 正确做法：先收集，再修改
    std::vector<int> to_add;
    for (int x : v) {
        if (x % 2 == 1) to_add.push_back(x * 10);
    }
    v.insert(v.end(), to_add.begin(), to_add.end());
    std::cout << "  先收集再插入:";
    for (int x : v) std::cout << " " << x;
    std::cout << "\n";
}

// ---------- 5. 让自定义类型支持范围 for ----------
// 只需要提供 begin() 和 end()（成员函数或 ADL 能找到的自由函数）
class IntRange {
public:
    IntRange(int from, int to) : _from(from), _to(to) {}

    class Iterator {
    public:
        explicit Iterator(int v) : _v(v) {}
        int operator*() const        { return _v; }
        Iterator& operator++()       { ++_v; return *this; }
        bool operator!=(const Iterator& rhs) const { return _v != rhs._v; }
    private:
        int _v;
    };

    Iterator begin() const { return Iterator(_from); }
    Iterator end() const   { return Iterator(_to); }
private:
    int _from, _to;
};

void custom_range() {
    std::cout << "  IntRange(1,6):";
    for (int x : IntRange(1, 6)) std::cout << " " << x;
    std::cout << "\n";
}

// ---------- 6. 遍历 map：C++11 的笨办法 vs C++17 结构化绑定 ----------
void map_traversal() {
    std::map<std::string, int> scores{{"alice", 90}, {"bob", 85}, {"carol", 95}};

    std::cout << "  C++11 写法（要用 .first/.second）:\n";
    for (const auto& pair : scores) {
        std::cout << "    " << pair.first << " = " << pair.second << "\n";
    }

    std::cout << "  C++17 结构化绑定:\n";
    for (const auto& [name, score] : scores) {
        std::cout << "    " << name << " = " << score << "\n";
    }

    // 结构化绑定也能用引用修改
    for (auto& [name, score] : scores) {
        score += 1;
        (void)name;
    }
    std::cout << "  全部加 1 后 alice = " << scores["alice"] << "\n";
}

// ---------- 7. 结构化绑定的三种来源 [C++17] ----------
struct Record { int id; std::string name; double value; };

std::tuple<int, std::string, bool> get_tuple() { return {1, "tuple", true}; }

void structured_binding_sources() {
    // 来源一：数组
    int arr[3]{10, 20, 30};
    auto [a, b, c] = arr;
    std::cout << "  数组: " << a << " " << b << " " << c << "\n";

    // 来源二：tuple / pair（任何支持 tuple_size/tuple_element/get 的类型）
    auto [num, text, flag] = get_tuple();
    std::cout << "  tuple: " << num << " " << text << " " << flag << "\n";

    // 来源三：所有非静态成员都是 public 的类（聚合类型）
    Record r{7, "widget", 3.14};
    auto [id, name, value] = r;
    std::cout << "  struct: " << id << " " << name << " " << value << "\n";

    // 绑定到引用，可以写回
    auto& [rid, rname, rvalue] = r;
    rid = 8;
    std::cout << "  通过引用修改后 r.id = " << r.id << "\n";

    // 典型用法：接住返回多个值的函数
    std::map<std::string, int> m;
    auto [iter, inserted] = m.insert({"key", 1});
    std::cout << "  map::insert 返回 inserted=" << inserted
              << " key=" << iter->first << "\n";
    auto [iter2, inserted2] = m.insert({"key", 2});
    std::cout << "  重复插入 inserted=" << inserted2
              << "（已存在，值仍是 " << iter2->second << "）\n";
}

// ---------- 8. if / switch 初始化语句 [C++17] ----------
void if_init_statement() {
    std::map<std::string, int> config{{"timeout", 30}};

    // C++11 写法：it 泄漏到外层作用域
    {
        auto it = config.find("timeout");
        if (it != config.end()) {
            std::cout << "  C++11: timeout = " << it->second << "\n";
        }
        // it 在这里依然可见，容易误用
    }

    // C++17 写法：变量作用域限制在 if 内
    if (auto it = config.find("timeout"); it != config.end()) {
        std::cout << "  C++17: timeout = " << it->second << "\n";
    } else {
        std::cout << "  未找到\n";
    }
    // it 在这里不可见

    // 与结构化绑定组合，是最常见的现代写法
    if (auto [iter, ok] = config.insert({"retries", 3}); ok) {
        std::cout << "  插入成功: " << iter->first << " = " << iter->second << "\n";
    }

    // switch 也支持
    switch (int n = static_cast<int>(config.size()); n) {
        case 1:  std::cout << "  switch: 1 项\n"; break;
        case 2:  std::cout << "  switch: 2 项\n"; break;
        default: std::cout << "  switch: " << n << " 项\n";
    }
}

int main() {
    std::cout << "--- 1. 遍历的演进 ---\n";            traversal_evolution();
    std::cout << "--- 2. 三种形式 ---\n";              three_forms();
    std::cout << "--- 3. 等价展开 ---\n";              how_it_works();
    std::cout << "--- 4. 失效陷阱 ---\n";              invalidation_trap();
    std::cout << "--- 5. 自定义类型适配 ---\n";        custom_range();
    std::cout << "--- 6. 遍历 map ---\n";              map_traversal();
    std::cout << "--- 7. 结构化绑定 [C++17] ---\n";    structured_binding_sources();
    std::cout << "--- 8. if 初始化语句 [C++17] ---\n"; if_init_statement();
}
