// std=c++17
// 对应章节: blog/part7_stdlib/07-06_regex_filesystem.md
// 主题: regex 的三个操作、性能注意；filesystem 的路径/遍历/查询

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

// filesystem 在 GCC 9 之前位于 experimental，且需要 -lstdc++fs
#if __has_include(<filesystem>)
#  include <filesystem>
namespace fs = std::filesystem;
#  define HAS_FILESYSTEM 1
#elif __has_include(<experimental/filesystem>)
#  include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#  define HAS_FILESYSTEM 1
#else
#  define HAS_FILESYSTEM 0
#endif

// ---------- 1. regex 的三个基本操作 ----------
namespace regex_basics {
    void demo() {
        // 操作一：regex_match —— 整个字符串必须完全匹配
        std::regex date_re(R"(\d{4}-\d{2}-\d{2})");
        std::cout << "      regex_match（必须【完全】匹配）:\n";
        std::cout << "        \"2024-01-15\" -> "
                  << std::regex_match("2024-01-15", date_re) << "\n";
        std::cout << "        \"date: 2024-01-15\" -> "
                  << std::regex_match("date: 2024-01-15", date_re)
                  << "（有多余内容，不匹配）\n";

        // 操作二：regex_search —— 查找子串
        std::cout << "      regex_search（查找【子串】）:\n";
        std::string text = "date: 2024-01-15 end";
        std::smatch m;
        if (std::regex_search(text, m, date_re)) {
            std::cout << "        找到: " << m[0]
                      << " 位置 " << m.position(0) << "\n";
        }

        // 操作三：regex_replace —— 替换
        std::cout << "      regex_replace（替换）:\n";
        std::string result = std::regex_replace(text, date_re, "YYYY-MM-DD");
        std::cout << "        " << result << "\n";
    }
}

// ---------- 2. 捕获组 ----------
namespace capture_groups {
    void demo() {
        std::regex log_re(R"((\w+)\s+\[(\d{4}-\d{2}-\d{2})\]\s+(.+))");
        std::string line = "ERROR [2024-01-15] disk full";

        std::smatch m;
        if (std::regex_match(line, m, log_re)) {
            std::cout << "      完整匹配 m[0] = " << m[0] << "\n";
            std::cout << "      级别  m[1] = " << m[1] << "\n";
            std::cout << "      日期  m[2] = " << m[2] << "\n";
            std::cout << "      消息  m[3] = " << m[3] << "\n";
            std::cout << "      组数 m.size() = " << m.size() << "\n";
        }

        // 替换时引用捕获组
        std::string reordered = std::regex_replace(line, log_re, "[$2] $1: $3");
        std::cout << "      用 $N 引用捕获组重排: " << reordered << "\n";
    }
}

// ---------- 3. 遍历所有匹配 ----------
namespace iterate_matches {
    void demo() {
        std::string text = "a=1, b=22, c=333";
        std::regex pair_re(R"((\w+)=(\d+))");

        std::cout << "      sregex_iterator 遍历所有匹配:\n";
        auto begin = std::sregex_iterator(text.begin(), text.end(), pair_re);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            std::cout << "        " << (*it)[1] << " => " << (*it)[2] << "\n";
        }

        std::cout << "      sregex_token_iterator 做分割:\n";
        std::regex comma(R"(\s*,\s*)");
        std::sregex_token_iterator tok(text.begin(), text.end(), comma, -1);  // -1 = 取分隔符之间
        std::sregex_token_iterator tok_end;
        for (; tok != tok_end; ++tok) std::cout << "        [" << *tok << "]\n";
    }
}

// ---------- 4. 正则语法与选项 ----------
namespace regex_options {
    void demo() {
        std::cout << "      六种语法（默认 ECMAScript）:\n";
        std::cout << "        ECMAScript（默认，功能最全）、basic、extended\n";
        std::cout << "        awk、grep、egrep\n";

        // 忽略大小写
        std::regex ci(R"(hello)", std::regex::icase);
        std::cout << "      icase: regex_match(\"HELLO\") = "
                  << std::regex_match("HELLO", ci) << "\n";

        // multiline [C++17]
        std::cout << "      [C++17] std::regex::multiline 让 ^ $ 匹配行首行尾\n";

        // 优化标志：构造慢一些，匹配快一些
        std::regex opt(R"(\d+)", std::regex::optimize);
        std::cout << "      optimize: 构造更慢，匹配更快（适合反复使用的正则）\n";

        std::cout << "      原始字符串字面量让正则可读得多（见 01-09）:\n";
        std::cout << "        传统写法: \"\\\\d{4}-\\\\d{2}\"\n";
        std::cout << "        原始写法: R\"(\\d{4}-\\d{2})\"\n";
    }
}

// ---------- 5. regex 的性能问题 ----------
namespace regex_perf {
    void demo() {
        constexpr int kTimes = 20000;
        std::string text = "ERROR [2024-01-15] disk full on /dev/sda1";

        // 错误做法：循环里构造正则
        auto t1 = std::chrono::steady_clock::now();
        int count1 = 0;
        for (int i = 0; i < kTimes; ++i) {
            std::regex re(R"(\d{4}-\d{2}-\d{2})");       // 每次都编译正则！
            if (std::regex_search(text, re)) ++count1;
        }
        auto t2 = std::chrono::steady_clock::now();

        // 正确做法：正则对象提到循环外
        std::regex re(R"(\d{4}-\d{2}-\d{2})");
        int count2 = 0;
        for (int i = 0; i < kTimes; ++i) {
            if (std::regex_search(text, re)) ++count2;
        }
        auto t3 = std::chrono::steady_clock::now();

        // 更快的做法：手写解析（如果格式固定）
        int count3 = 0;
        for (int i = 0; i < kTimes; ++i) {
            auto pos = text.find('[');
            if (pos != std::string::npos && text.size() > pos + 11) ++count3;
        }
        auto t4 = std::chrono::steady_clock::now();

        auto us = [](auto a, auto b) {
            return std::chrono::duration_cast<std::chrono::microseconds>(b - a).count();
        };
        std::cout << "      " << kTimes << " 次匹配:\n";
        std::cout << "        循环内构造 regex: " << us(t1, t2) << " us\n";
        std::cout << "        regex 提到循环外: " << us(t2, t3) << " us\n";
        std::cout << "        手写 find 解析:   " << us(t3, t4) << " us\n";
        std::cout << "      结论: 1) regex 对象一定要复用  2) std::regex 本身不快\n";
        std::cout << "            格式固定时手写解析快一个数量级以上\n";
        (void)count1; (void)count2; (void)count3;
    }
}

// ---------- 6. filesystem：路径操作 ----------
#if HAS_FILESYSTEM
namespace path_ops {
    void demo() {
        fs::path p = "/home/user/docs/report.tar.gz";

        std::cout << "      路径分解:\n";
        std::cout << "        完整路径: " << p << "\n";
        std::cout << "        父目录 parent_path():    " << p.parent_path() << "\n";
        std::cout << "        文件名 filename():       " << p.filename() << "\n";
        std::cout << "        主干   stem():           " << p.stem() << "\n";
        std::cout << "        扩展名 extension():      " << p.extension()
                  << "（只取最后一个）\n";
        std::cout << "        根目录 root_path():      " << p.root_path() << "\n";
        std::cout << "        是绝对路径: " << p.is_absolute() << "\n";

        std::cout << "      路径拼接（用 / 运算符，跨平台）:\n";
        fs::path dir = "/tmp";
        fs::path file = dir / "subdir" / "file.txt";
        std::cout << "        \"/tmp\" / \"subdir\" / \"file.txt\" = " << file << "\n";
        std::cout << "        （自动用平台的分隔符，不用手工拼 \"/\" 或 \"\\\\\"）\n";

        std::cout << "      路径规范化:\n";
        fs::path messy = "/tmp/./subdir/../file.txt";
        std::cout << "        原始:            " << messy << "\n";
        std::cout << "        lexically_normal: " << messy.lexically_normal() << "\n";

        std::cout << "      相对路径:\n";
        fs::path base = "/tmp/a/b";
        fs::path target = "/tmp/a/c/d.txt";
        std::cout << "        " << target << " 相对于 " << base << " = "
                  << target.lexically_relative(base) << "\n";
    }
}

// ---------- 7. filesystem：文件系统查询与操作 ----------
namespace fs_ops {
    void demo() {
        fs::path tmpdir = "/tmp/cpp_fs_demo";

        // 创建目录
        fs::create_directories(tmpdir / "sub1" / "sub2");
        std::cout << "      create_directories 创建了嵌套目录\n";

        // 写几个文件
        for (const char* name : {"a.txt", "b.log", "c.txt"}) {
            std::ofstream ofs(tmpdir / name);
            ofs << "content of " << name << "\n";
        }
        { std::ofstream ofs(tmpdir / "sub1" / "nested.txt"); ofs << "nested\n"; }

        // 查询
        std::cout << "      查询:\n";
        std::cout << "        exists: " << fs::exists(tmpdir / "a.txt") << "\n";
        std::cout << "        is_directory: " << fs::is_directory(tmpdir) << "\n";
        std::cout << "        is_regular_file: " << fs::is_regular_file(tmpdir / "a.txt") << "\n";
        std::cout << "        file_size: " << fs::file_size(tmpdir / "a.txt") << " 字节\n";

        auto space = fs::space(tmpdir);
        std::cout << "        space().available: " << (space.available >> 30) << " GB\n";

        // 非递归遍历
        std::cout << "      directory_iterator（非递归）:\n";
        std::vector<std::string> entries;
        for (const auto& entry : fs::directory_iterator(tmpdir)) {
            entries.push_back(entry.path().filename().string());
        }
        std::sort(entries.begin(), entries.end());
        for (const auto& e : entries) std::cout << "        " << e << "\n";

        // 递归遍历
        std::cout << "      recursive_directory_iterator（递归）:\n";
        std::vector<std::string> all;
        for (const auto& entry : fs::recursive_directory_iterator(tmpdir)) {
            if (entry.is_regular_file()) {
                all.push_back(fs::relative(entry.path(), tmpdir).string());
            }
        }
        std::sort(all.begin(), all.end());
        for (const auto& e : all) std::cout << "        " << e << "\n";

        // 按扩展名过滤
        std::cout << "      过滤 .txt 文件:\n";
        int txt_count = 0;
        for (const auto& entry : fs::recursive_directory_iterator(tmpdir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") ++txt_count;
        }
        std::cout << "        找到 " << txt_count << " 个 .txt\n";

        // 拷贝、重命名、删除
        fs::copy_file(tmpdir / "a.txt", tmpdir / "a_copy.txt",
                      fs::copy_options::overwrite_existing);
        fs::rename(tmpdir / "a_copy.txt", tmpdir / "a_renamed.txt");
        std::cout << "      copy_file + rename 完成: "
                  << fs::exists(tmpdir / "a_renamed.txt") << "\n";

        // 错误处理的两种方式
        std::cout << "      错误处理:\n";
        try {
            // file_size 标了 [[nodiscard]]，忽略返回值会告警（见 01-09），
            // 所以这里把返回值接住
            auto ignored = fs::file_size("/nonexistent/path");
            (void)ignored;
        } catch (const fs::filesystem_error& e) {
            std::cout << "        抛异常版本: " << e.code().message() << "\n";
        }
        std::error_code ec;
        auto sz = fs::file_size("/nonexistent/path", ec);
        std::cout << "        error_code 版本: ec=" << ec.message()
                  << " 返回值=" << static_cast<long long>(sz) << "\n";
        std::cout << "      （大部分函数都有两个重载：抛异常 / 填 error_code）\n";

        // 清理
        auto removed = fs::remove_all(tmpdir);
        std::cout << "      remove_all 删除了 " << removed << " 个条目\n";
    }
}
#endif

int main() {
    std::cout << "--- 1. regex 三个操作 ---\n";      regex_basics::demo();
    std::cout << "--- 2. 捕获组 ---\n";              capture_groups::demo();
    std::cout << "--- 3. 遍历所有匹配 ---\n";        iterate_matches::demo();
    std::cout << "--- 4. 语法与选项 ---\n";          regex_options::demo();
    std::cout << "--- 5. regex 的性能 ---\n";        regex_perf::demo();
#if HAS_FILESYSTEM
    std::cout << "--- 6. 路径操作 [C++17] ---\n";    path_ops::demo();
    std::cout << "--- 7. 文件系统操作 [C++17] ---\n"; fs_ops::demo();
#else
    std::cout << "--- 6/7. filesystem 不可用 ---\n";
#endif
}
