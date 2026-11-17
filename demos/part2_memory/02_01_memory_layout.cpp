// std=c++17
// 对应章节: blog/part2_memory/02-01_memory_layout.md
// 主题: 实测程序各内存区的地址分布

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

// ---------- 各内存区的代表 ----------
int g_initialized = 42;          // .data  已初始化全局
int g_uninitialized;             // .bss   未初始化全局
const int g_const = 100;         // .rodata 只读数据
static int s_file_scope = 7;     // .data  内部链接

void some_function() {}          // .text  代码区

void print_addr(const char* label, const void* p) {
    // 注意 std::strlen 返回 size_t（无符号）。如果写成 28 - strlen(label)
    // 而 label 比 28 字节长，减法会下溢成一个巨大的无符号数，
    // 导致 std::string 构造抛 std::length_error。UTF-8 下一个汉字占 3 字节，
    // 很容易超过 28 —— 这个坑我在写这个 demo 时真踩到了。
    constexpr std::size_t kWidth = 34;
    const std::size_t len = std::strlen(label);
    const std::size_t pad = (len < kWidth) ? (kWidth - len) : 1;
    std::cout << "  " << label << std::string(pad, ' ') << p << "\n";
}

void layout_probe() {
    int local = 1;                      // 栈
    static int s_local = 2;             // .data（静态局部，不在栈上）
    char str_array[] = "123456";        // 栈上的数组（内容是拷贝）
    const char* str_literal = "123456"; // 指针在栈，字面量在 .rodata
    int* heap1 = new int[10];           // 堆
    int* heap2 = new int[5];            // 堆
    void* malloced = std::malloc(64);   // 堆

    std::cout << "  ---- 由高地址到低地址 ----\n";
    print_addr("栈 local", &local);
    print_addr("栈 str_array", str_array);
    print_addr("栈 &str_literal(指针本身)", &str_literal);
    std::cout << "\n";
    print_addr("堆 new int[10]", heap1);
    print_addr("堆 new int[5]", heap2);
    print_addr("堆 malloc(64)", malloced);
    std::cout << "\n";
    print_addr(".bss  g_uninitialized", &g_uninitialized);
    print_addr(".data g_initialized", &g_initialized);
    print_addr(".data s_file_scope", &s_file_scope);
    print_addr(".data s_local(静态局部)", &s_local);
    std::cout << "\n";
    print_addr(".rodata g_const", &g_const);
    print_addr(".rodata 字符串字面量", str_literal);
    std::cout << "\n";
    print_addr(".text some_function", reinterpret_cast<const void*>(&some_function));

    delete[] heap1;
    delete[] heap2;
    std::free(malloced);
}

// ---------- 栈的增长方向 ----------
void stack_direction_inner(int* outer_addr) {
    int inner = 0;
    std::cout << "  外层局部变量地址: " << static_cast<void*>(outer_addr) << "\n";
    std::cout << "  内层局部变量地址: " << static_cast<void*>(&inner) << "\n";
    std::cout << "  内层地址 "
              << (&inner < outer_addr ? "更低 -> 栈向低地址增长" : "更高")
              << "\n";
}

void stack_direction() {
    int outer = 0;
    stack_direction_inner(&outer);
}

// ---------- 同一作用域内多个变量的排布 ----------
void stack_order() {
    int a = 1, b = 2, c = 3;
    std::cout << "  &a=" << static_cast<void*>(&a)
              << " &b=" << static_cast<void*>(&b)
              << " &c=" << static_cast<void*>(&c) << "\n";
    std::cout << "  （同作用域内变量的相对顺序由编译器决定，不要依赖它）\n";
}

// ---------- 堆与栈的分配速度对比 ----------
#include <chrono>
void speed_comparison() {
    constexpr int kTimes = 200000;

    auto t1 = std::chrono::steady_clock::now();
    for (int i = 0; i < kTimes; ++i) {
        volatile char buf[64];        // 栈分配：只是移动栈指针
        (void)buf;
    }
    auto t2 = std::chrono::steady_clock::now();

    for (int i = 0; i < kTimes; ++i) {
        char* p = new char[64];       // 堆分配：要走分配器
        delete[] p;
    }
    auto t3 = std::chrono::steady_clock::now();

    auto stack_us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    auto heap_us = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
    std::cout << "  " << kTimes << " 次 64 字节分配:\n";
    std::cout << "    栈: " << stack_us << " us\n";
    std::cout << "    堆: " << heap_us << " us\n";
}

// ---------- 栈空间有限，堆空间大得多 ----------
void stack_limit() {
    std::cout << "  典型栈大小: Linux 默认 8MB (ulimit -s)，Windows 默认 1MB\n";
    std::cout << "  在栈上开大数组会栈溢出:\n";
    std::cout << "    char buf[10*1024*1024];  // 很可能 segfault\n";
    std::cout << "  同样大小在堆上没问题:\n";
    auto* big = new char[10 * 1024 * 1024];
    std::memset(big, 0, 10);
    std::cout << "    new char[10*1024*1024] 成功，地址 " << static_cast<void*>(big) << "\n";
    delete[] big;
}

int main() {
    std::cout << "--- 1. 各内存区地址 ---\n";   layout_probe();
    std::cout << "--- 2. 栈增长方向 ---\n";     stack_direction();
    std::cout << "--- 3. 同作用域变量 ---\n";   stack_order();
    std::cout << "--- 4. 分配速度 ---\n";       speed_comparison();
    std::cout << "--- 5. 空间限制 ---\n";       stack_limit();
}
