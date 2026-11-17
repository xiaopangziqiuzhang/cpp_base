// std=c++17
// 对应章节: blog/part0_overview/00-01_history.md
// 主题: 探测当前编译器与标准版本

#include <iostream>

// 库特性测试宏需要 <version>（C++20 起）；C++17 下包含对应头文件也会定义一部分
#if __has_include(<version>)
#  include <version>
#endif
#include <optional>
#include <string_view>

int main() {
    std::cout << "=== 编译器 ===\n";
#if defined(__clang__)
    std::cout << "Clang " << __clang_major__ << "." << __clang_minor__ << "\n";
#elif defined(__GNUC__)
    std::cout << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << "\n";
#elif defined(_MSC_VER)
    std::cout << "MSVC " << _MSC_VER << "\n";
#else
    std::cout << "unknown\n";
#endif

    std::cout << "\n=== 标准版本 ===\n";
    std::cout << "__cplusplus = " << __cplusplus << "  -> ";
    if (__cplusplus >= 202302L)      std::cout << "C++23 或更新\n";
    else if (__cplusplus >= 202002L) std::cout << "C++20\n";
    else if (__cplusplus >= 201703L) std::cout << "C++17\n";
    else if (__cplusplus >= 201402L) std::cout << "C++14\n";
    else if (__cplusplus >= 201103L) std::cout << "C++11\n";
    else                             std::cout << "C++98/03\n";

    // MSVC 的坑：不加 /Zc:__cplusplus 时该宏恒为 199711L
#if defined(_MSC_VER) && __cplusplus == 199711L
    std::cout << "（MSVC 未加 /Zc:__cplusplus，上面的值不可信）\n";
#endif

    std::cout << "\n=== 语言特性测试宏 ===\n";
    // 这些宏由 SD-6 / C++20 规范化，GCC/Clang 在更早版本就已支持
#ifdef __cpp_rvalue_references
    std::cout << "rvalue_references     = " << __cpp_rvalue_references << "\n";
#endif
#ifdef __cpp_lambdas
    std::cout << "lambdas               = " << __cpp_lambdas << "\n";
#endif
#ifdef __cpp_constexpr
    std::cout << "constexpr             = " << __cpp_constexpr << "\n";
#endif
#ifdef __cpp_generic_lambdas
    std::cout << "generic_lambdas       = " << __cpp_generic_lambdas << "  [C++14]\n";
#endif
#ifdef __cpp_return_type_deduction
    std::cout << "return_type_deduction = " << __cpp_return_type_deduction << "  [C++14]\n";
#endif
#ifdef __cpp_variable_templates
    std::cout << "variable_templates    = " << __cpp_variable_templates << "  [C++14]\n";
#endif
#ifdef __cpp_structured_bindings
    std::cout << "structured_bindings   = " << __cpp_structured_bindings << "  [C++17]\n";
#endif
#ifdef __cpp_if_constexpr
    std::cout << "if_constexpr          = " << __cpp_if_constexpr << "  [C++17]\n";
#endif
#ifdef __cpp_fold_expressions
    std::cout << "fold_expressions      = " << __cpp_fold_expressions << "  [C++17]\n";
#endif
#ifdef __cpp_deduction_guides
    std::cout << "deduction_guides      = " << __cpp_deduction_guides << "  [C++17]\n";
#endif
#ifdef __cpp_inline_variables
    std::cout << "inline_variables      = " << __cpp_inline_variables << "  [C++17]\n";
#endif
#ifdef __cpp_concepts
    std::cout << "concepts              = " << __cpp_concepts << "  [C++20]\n";
#endif

    std::cout << "\n=== 库特性测试宏 ===\n";
#ifdef __cpp_lib_optional
    std::cout << "lib_optional     = " << __cpp_lib_optional << "  [C++17]\n";
#endif
#ifdef __cpp_lib_string_view
    std::cout << "lib_string_view  = " << __cpp_lib_string_view << "  [C++17]\n";
#endif
#ifdef __cpp_lib_filesystem
    std::cout << "lib_filesystem   = " << __cpp_lib_filesystem << "  [C++17]\n";
#endif
#ifdef __cpp_lib_make_unique
    std::cout << "lib_make_unique  = " << __cpp_lib_make_unique << "  [C++14]\n";
#endif
}
