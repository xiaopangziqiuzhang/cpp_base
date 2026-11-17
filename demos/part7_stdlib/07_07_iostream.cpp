// std=c++17
// 对应章节: blog/part7_stdlib/07-07_iostream.md
// 主题: 流类型体系、流状态、缓冲区、文件 IO、stringstream

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

// ---------- 1. 流的概念与类型体系 ----------
namespace stream_hierarchy {
    void demo() {
        std::cout << "      \"流\"的概念：把 IO 看成字节的有序流动\n";
        std::cout << "        输入 = 从设备流向内存，输出 = 从内存流向设备\n";
        std::cout << "        好处：统一了控制台、文件、字符串、网络的接口\n";

        std::cout << "      类型体系（简化）:\n";
        std::cout << "                        ios_base\n";
        std::cout << "                           |\n";
        std::cout << "                      basic_ios\n";
        std::cout << "                      /        \\\n";
        std::cout << "            basic_istream    basic_ostream\n";
        std::cout << "              /      \\        /       \\\n";
        std::cout << "        ifstream  istringstream  ofstream  ostringstream\n";
        std::cout << "                      \\        /\n";
        std::cout << "                    basic_iostream\n";
        std::cout << "                      /        \\\n";
        std::cout << "                 fstream    stringstream\n";
        std::cout << "      注意 basic_iostream 是【虚继承】自 basic_ios 的，\n";
        std::cout << "      这是标准库里最著名的菱形继承（见 04-03）\n";

        std::cout << "      三类流:\n";
        std::cout << "        标准流: cin/cout/cerr/clog（<iostream>）\n";
        std::cout << "        文件流: ifstream/ofstream/fstream（<fstream>）\n";
        std::cout << "        串流:   istringstream/ostringstream/stringstream（<sstream>）\n";
    }
}

// ---------- 2. 流的状态 ----------
namespace stream_state {
    void demo() {
        std::cout << "      四个状态位:\n";
        std::cout << "        goodbit —— 一切正常（值为 0）\n";
        std::cout << "        eofbit  —— 到达文件尾\n";
        std::cout << "        failbit —— 操作失败（如格式错误），但流还能恢复\n";
        std::cout << "        badbit  —— 严重错误（如底层设备故障），通常无法恢复\n";

        std::istringstream iss("42 abc");
        int n = 0;
        iss >> n;
        std::cout << "      读入 42 后: good=" << iss.good()
                  << " eof=" << iss.eof() << " fail=" << iss.fail()
                  << " bad=" << iss.bad() << "\n";

        iss >> n;        // 尝试把 "abc" 读成 int -> 失败
        std::cout << "      读 \"abc\" 失败后: good=" << iss.good()
                  << " eof=" << iss.eof() << " fail=" << iss.fail()
                  << " bad=" << iss.bad() << "\n";
        std::cout << "      n 的值 = " << n << "（C++11 起失败时被置为 0）\n";

        iss.clear();     // 清除状态位
        std::cout << "      clear() 之后 good=" << iss.good() << "\n";

        // 正确的恢复流程：clear + 跳过坏数据
        std::istringstream iss2("abc 42");
        int v = 0;
        if (!(iss2 >> v)) {
            iss2.clear();
            std::string skipped;
            iss2 >> skipped;             // 跳过坏数据
            iss2 >> v;
            std::cout << "      恢复后读到: " << v << "（跳过了 \"" << skipped << "\"）\n";
        }

        std::cout << "      流转 bool 等价于 !fail()，所以惯用法是:\n";
        std::cout << "        while (iss >> value) { ... }\n";
    }
}

// ---------- 3. 为什么要缓冲区 ----------
namespace buffering {
    void demo() {
        std::cout << "      为什么要缓冲区:\n";
        std::cout << "        1. 系统调用很贵。每个字符一次 write 是灾难\n";
        std::cout << "        2. 块设备按块读写，攒够一块再操作效率高\n";
        std::cout << "        3. 速度不匹配：CPU 快、磁盘慢，缓冲区解耦两者\n";

        std::cout << "      三种缓冲模式:\n";
        std::cout << "        全缓冲（_IOFBF）—— 缓冲区满才刷。文件默认\n";
        std::cout << "        行缓冲（_IOLBF）—— 遇换行就刷。终端的 stdout 默认\n";
        std::cout << "        无缓冲（_IONBF）—— 立即写。stderr 默认\n";

        // 实测缓冲对性能的影响
        constexpr int kLines = 200000;
        const char* path1 = "/tmp/io_buffered.txt";
        const char* path2 = "/tmp/io_flushed.txt";

        auto t1 = std::chrono::steady_clock::now();
        {
            std::ofstream ofs(path1);
            for (int i = 0; i < kLines; ++i) ofs << "line " << i << "\n";
        }
        auto t2 = std::chrono::steady_clock::now();
        {
            std::ofstream ofs(path2);
            for (int i = 0; i < kLines; ++i) ofs << "line " << i << std::endl;  // 每行都刷
        }
        auto t3 = std::chrono::steady_clock::now();

        auto ms = [](auto a, auto b) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
        };
        std::cout << "      写 " << kLines << " 行:\n";
        std::cout << "        用 \"\\n\"（不刷新）:   " << ms(t1, t2) << " ms\n";
        std::cout << "        用 std::endl（每行刷）: " << ms(t2, t3) << " ms\n";
        std::cout << "      结论: std::endl = \"\\n\" + flush，循环里用它很贵\n";
        std::cout << "            除非真的需要立即可见，否则用 \"\\n\"\n";
        std::remove(path1);
        std::remove(path2);
    }
}

// ---------- 4. cout / cerr / clog 的区别 ----------
namespace std_streams {
    void demo() {
        std::cout << "      cout —— 标准输出，【全缓冲】（重定向到文件时）或行缓冲（终端）\n";
        std::cout << "      cerr —— 标准错误，【无缓冲】，立即输出\n";
        std::cout << "      clog —— 标准错误，【有缓冲】\n";
        std::cout << "      为什么 cerr 无缓冲: 程序崩溃时错误信息不能丢\n";
        std::cout << "      为什么有 clog: 大量日志时需要缓冲的性能\n";

        std::cout << "      cin 和 cout 默认是【绑定】的:\n";
        std::cout << "        任何从 cin 读取之前，cout 会自动 flush\n";
        std::cout << "        这保证了 \"请输入:\" 的提示能先显示出来\n";
        std::cout << "        tie() 返回绑定的流: "
                  << (std::cin.tie() == &std::cout ? "cout" : "其他") << "\n";
        std::cout << "      竞赛常用的加速技巧:\n";
        std::cout << "        std::ios::sync_with_stdio(false);  // 解除与 C stdio 的同步\n";
        std::cout << "        std::cin.tie(nullptr);             // 解除绑定\n";
        std::cout << "        （但之后不能再混用 printf/scanf）\n";
    }
}

// ---------- 5. 操纵符 ----------
namespace manipulators {
    void demo() {
        std::cout << "      进制: " << std::hex << 255 << "(hex) "
                  << std::oct << 255 << "(oct) "
                  << std::dec << 255 << "(dec)\n";
        std::cout << "      带前缀: " << std::showbase << std::hex << 255
                  << std::dec << std::noshowbase << "\n";

        std::cout << "      宽度与填充:\n";
        std::cout << "        [" << std::setw(10) << 42 << "]（右对齐，默认）\n";
        std::cout << "        [" << std::left << std::setw(10) << 42 << "]（左对齐）\n";
        std::cout << "        [" << std::right << std::setfill('0') << std::setw(10) << 42
                  << "]（填 0）\n";
        std::cout << std::setfill(' ');

        std::cout << "      浮点格式:\n";
        double pi = 3.14159265358979;
        std::cout << "        默认:      " << pi << "\n";
        std::cout << "        precision(3): " << std::setprecision(3) << pi << "\n";
        std::cout << "        fixed:     " << std::fixed << std::setprecision(4) << pi << "\n";
        std::cout << "        scientific: " << std::scientific << pi << "\n";
        std::cout << std::defaultfloat << std::setprecision(6);

        std::cout << "      布尔:\n";
        std::cout << "        默认: " << true << " / " << false << "\n";
        std::cout << "        boolalpha: " << std::boolalpha << true << " / " << false
                  << std::noboolalpha << "\n";

        std::cout << "      注意: setw 只影响【下一次】输出，其他大多是【持久】的\n";
        std::cout << "        这是常见的 bug 来源：设了 hex 忘了改回来\n";
    }
}

// ---------- 6. 文件 IO 与文件模式 ----------
namespace file_io {
    void demo() {
        const char* path = "/tmp/io_demo.txt";

        std::cout << "      文件打开模式（可用 | 组合）:\n";
        std::cout << "        ios::in     读\n";
        std::cout << "        ios::out    写（默认截断）\n";
        std::cout << "        ios::app    追加（每次写都到末尾）\n";
        std::cout << "        ios::ate    打开后定位到末尾（但可以 seek 回去）\n";
        std::cout << "        ios::trunc  截断已有内容\n";
        std::cout << "        ios::binary 二进制模式（不转换换行符）\n";

        // 写
        {
            std::ofstream ofs(path);
            if (!ofs) { std::cout << "      打开失败\n"; return; }
            ofs << "line 1\nline 2\nline 3\n";
        }   // 析构自动关闭（RAII，见 02-06）

        // 逐行读 —— 标准写法
        std::cout << "      逐行读取（std::getline）:\n";
        {
            std::ifstream ifs(path);
            std::string line;
            int n = 0;
            while (std::getline(ifs, line)) {
                std::cout << "        第 " << ++n << " 行: " << line << "\n";
            }
        }

        // 一次读入全部内容
        std::cout << "      一次读入全部内容:\n";
        {
            std::ifstream ifs(path, std::ios::binary | std::ios::ate);
            auto size = ifs.tellg();
            ifs.seekg(0);
            std::string content(static_cast<std::size_t>(size), '\0');
            ifs.read(content.data(), size);
            std::cout << "        读入 " << content.size() << " 字节\n";
        }

        // 追加
        {
            std::ofstream ofs(path, std::ios::app);
            ofs << "appended\n";
        }
        {
            std::ifstream ifs(path);
            std::string all((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
            std::cout << "      app 模式追加后共 " << all.size() << " 字节\n";
            std::cout << "      （用 istreambuf_iterator 读全文件是另一个常用惯用法）\n";
        }

        // 二进制读写
        {
            std::vector<int> data{1, 2, 3, 4, 5};
            std::ofstream ofs("/tmp/io_demo.bin", std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(data.data()),
                      static_cast<std::streamsize>(data.size() * sizeof(int)));
        }
        {
            std::ifstream ifs("/tmp/io_demo.bin", std::ios::binary);
            std::vector<int> loaded(5);
            ifs.read(reinterpret_cast<char*>(loaded.data()),
                     static_cast<std::streamsize>(loaded.size() * sizeof(int)));
            std::cout << "      二进制读回: ";
            for (int x : loaded) std::cout << x << " ";
            std::cout << "\n";
            std::cout << "      注意: 二进制格式不跨平台（字节序、对齐、类型大小）\n";
        }

        std::remove(path);
        std::remove("/tmp/io_demo.bin");
    }
}

// ---------- 7. stringstream：格式转换 ----------
namespace string_streams {
    void demo() {
        // 数值 -> 字符串
        std::ostringstream oss;
        oss << "id=" << 42 << " pi=" << std::fixed << std::setprecision(2) << 3.14159;
        std::cout << "      ostringstream 拼接: " << oss.str() << "\n";

        // 字符串 -> 数值
        std::istringstream iss("42 3.14 hello");
        int i; double d; std::string s;
        iss >> i >> d >> s;
        std::cout << "      istringstream 解析: i=" << i << " d=" << d << " s=" << s << "\n";

        // 分割字符串（标准库没有 split）
        std::cout << "      用 getline + stringstream 分割:\n";
        std::string csv = "alice,30,engineer";
        std::istringstream line_ss(csv);
        std::string field;
        while (std::getline(line_ss, field, ',')) {
            std::cout << "        [" << field << "]\n";
        }

        // 复用 stringstream 要注意
        std::ostringstream reuse;
        reuse << "first";
        std::cout << "      复用前: " << reuse.str() << "\n";
        reuse.str("");          // 清空内容
        reuse.clear();           // 清除状态位
        reuse << "second";
        std::cout << "      复用后: " << reuse.str() << "（str(\"\") + clear() 都要调）\n";

        // 性能对比
        constexpr int kTimes = 200000;
        auto t1 = std::chrono::steady_clock::now();
        for (int k = 0; k < kTimes; ++k) {
            std::ostringstream o;              // 每次构造
            o << k;
            volatile auto r = o.str();
            (void)r;
        }
        auto t2 = std::chrono::steady_clock::now();
        for (int k = 0; k < kTimes; ++k) {
            volatile auto r = std::to_string(k);
            (void)r;
        }
        auto t3 = std::chrono::steady_clock::now();

        auto ms = [](auto a, auto b) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
        };
        std::cout << "      " << kTimes << " 次 int 转字符串:\n";
        std::cout << "        ostringstream: " << ms(t1, t2) << " ms\n";
        std::cout << "        to_string:     " << ms(t2, t3) << " ms\n";
        std::cout << "      简单转换用 to_string 或 to_chars（见 07-01），别用 stringstream\n";
    }
}

int main() {
    std::cout << "--- 1. 流的类型体系 ---\n";   stream_hierarchy::demo();
    std::cout << "--- 2. 流的状态 ---\n";       stream_state::demo();
    std::cout << "--- 3. 缓冲区 ---\n";         buffering::demo();
    std::cout << "--- 4. 标准流的区别 ---\n";   std_streams::demo();
    std::cout << "--- 5. 操纵符 ---\n";         manipulators::demo();
    std::cout << "--- 6. 文件 IO ---\n";        file_io::demo();
    std::cout << "--- 7. stringstream ---\n";   string_streams::demo();
}
