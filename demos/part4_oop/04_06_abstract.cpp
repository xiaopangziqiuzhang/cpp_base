// std=c++17
// 对应章节: blog/part4_oop/04-06_abstract.md
// 主题: 纯虚函数、抽象类、接口类、虚析构函数的必要性

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ---------- 1. 纯虚函数与抽象类 ----------
namespace abstract_basics {
    class Shape {
    public:
        virtual ~Shape() = default;

        // 纯虚函数：只有声明，没有定义，用 = 0 标记
        virtual double area() const = 0;
        virtual std::string name() const = 0;

        // 抽象类也可以有普通成员函数和数据成员
        void describe() const {
            std::cout << "        " << name() << " 的面积是 " << area() << "\n";
        }
    };

    class Circle : public Shape {
    public:
        explicit Circle(double r) : _r(r) {}
        double area() const override { return 3.14159 * _r * _r; }
        std::string name() const override { return "圆"; }
    private:
        double _r;
    };

    class Rectangle : public Shape {
    public:
        Rectangle(double w, double h) : _w(w), _h(h) {}
        double area() const override { return _w * _h; }
        std::string name() const override { return "矩形"; }
    private:
        double _w, _h;
    };

    // 只实现了一部分纯虚函数 -> 仍然是抽象类
    class Incomplete : public Shape {
    public:
        double area() const override { return 0; }
        // 没实现 name() -> Incomplete 仍是抽象类，不能实例化
    };

    void demo() {
        // Shape s;              // 编译错误：cannot declare variable 's' to be of
        //                       //   abstract type 'Shape'
        // Incomplete i;         // 编译错误：同样是抽象类

        Circle c(2.0);
        Rectangle r(3.0, 4.0);
        c.describe();
        r.describe();

        std::cout << "      抽象类不能实例化，但可以有指针和引用:\n";
        std::vector<std::unique_ptr<Shape>> shapes;
        shapes.push_back(std::make_unique<Circle>(1.0));
        shapes.push_back(std::make_unique<Rectangle>(2.0, 3.0));
        for (const auto& s : shapes) s->describe();
    }
}

// ---------- 2. 纯虚函数也可以有实现 ----------
namespace pure_virtual_with_body {
    class Base {
    public:
        virtual ~Base() = default;
        virtual void f() = 0;      // 纯虚，但下面给了定义
    };

    // 纯虚函数可以有定义，派生类用限定名调用它
    void Base::f() {
        std::cout << "        Base::f() 的默认实现（纯虚函数也能有实现）\n";
    }

    class Derived : public Base {
    public:
        void f() override {
            Base::f();            // 显式调用纯虚函数的实现，复用公共逻辑
            std::cout << "        Derived::f() 自己的部分\n";
        }
    };

    void demo() {
        Derived d;
        d.f();
        std::cout << "      用途：强制派生类覆盖，但同时提供可复用的默认逻辑\n";
    }
}

// ---------- 3. 纯虚析构函数 ----------
namespace pure_virtual_dtor {
    // 想让一个类成为抽象类，但它没有任何需要纯虚的函数时，可以用纯虚析构函数
    class AbstractBase {
    public:
        virtual ~AbstractBase() = 0;      // 纯虚析构函数
    };
    // 必须提供定义！因为派生类的析构函数一定会调用它
    AbstractBase::~AbstractBase() {
        std::cout << "        AbstractBase::~AbstractBase()\n";
    }

    class Concrete : public AbstractBase {
    public:
        ~Concrete() override { std::cout << "        Concrete::~Concrete()\n"; }
    };

    void demo() {
        // AbstractBase ab;      // 编译错误：抽象类
        Concrete c;
        std::cout << "      纯虚析构函数让类变成抽象类，但【必须】提供定义\n";
    }
}

// ---------- 4. 虚析构函数的必要性 ----------
namespace virtual_dtor {
    class BaseNonVirtual {
    public:
        BaseNonVirtual() { std::cout << "        + BaseNonVirtual\n"; }
        ~BaseNonVirtual() { std::cout << "        - BaseNonVirtual\n"; }   // 非虚！
    };

    class DerivedWithResource : public BaseNonVirtual {
    public:
        DerivedWithResource() : _data(new int[100]) {
            std::cout << "        + DerivedWithResource（分配了 400 字节）\n";
        }
        ~DerivedWithResource() {
            delete[] _data;
            std::cout << "        - DerivedWithResource（释放了 400 字节）\n";
        }
    private:
        int* _data;
    };

    class BaseVirtual {
    public:
        BaseVirtual() { std::cout << "        + BaseVirtual\n"; }
        virtual ~BaseVirtual() { std::cout << "        - BaseVirtual\n"; }  // 虚
    };

    class DerivedVirtual : public BaseVirtual {
    public:
        DerivedVirtual() : _data(new int[100]) {
            std::cout << "        + DerivedVirtual（分配了 400 字节）\n";
        }
        ~DerivedVirtual() override {
            delete[] _data;
            std::cout << "        - DerivedVirtual（释放了 400 字节）\n";
        }
    private:
        int* _data;
    };

    void demo() {
        std::cout << "    基类析构函数【非虚】时通过基类指针删除:\n";
        {
            BaseNonVirtual* p = new DerivedWithResource();
            delete p;        // 只调用 ~BaseNonVirtual()！派生类的析构函数不执行
        }
        std::cout << "      ^ 注意【没有】DerivedWithResource 的析构输出 -> 400 字节泄漏\n";
        std::cout << "        （标准上这是未定义行为）\n";

        std::cout << "    基类析构函数【虚】时:\n";
        {
            BaseVirtual* p = new DerivedVirtual();
            delete p;        // 正确调用两层析构函数
        }
        std::cout << "      ^ 两层析构函数都被调用，资源正确释放\n";
    }
}

// ---------- 5. 接口类：纯虚 + 无数据成员 ----------
namespace interfaces {
    // 典型的接口类
    class ISerializable {
    public:
        virtual ~ISerializable() = default;
        virtual std::string serialize() const = 0;
        virtual void deserialize(const std::string& data) = 0;
    };

    class IComparable {
    public:
        virtual ~IComparable() = default;
        virtual int compare_to(const IComparable& other) const = 0;
    };

    class Document : public ISerializable, public IComparable {
    public:
        explicit Document(std::string content) : _content(std::move(content)) {}

        std::string serialize() const override { return "DOC:" + _content; }
        void deserialize(const std::string& data) override {
            _content = data.substr(4);
        }
        int compare_to(const IComparable& other) const override {
            const auto* doc = dynamic_cast<const Document*>(&other);
            if (!doc) return 0;
            return _content.compare(doc->_content);
        }
        const std::string& content() const { return _content; }
    private:
        std::string _content;
    };

    void demo() {
        Document d1("hello");
        Document d2("world");

        std::cout << "      d1.serialize() = " << d1.serialize() << "\n";
        std::cout << "      d1.compare_to(d2) = " << d1.compare_to(d2) << "\n";

        // 可以按任一接口使用
        ISerializable* s = &d1;
        std::cout << "      通过 ISerializable* 调用: " << s->serialize() << "\n";

        std::cout << "      sizeof(Document) = " << sizeof(Document)
                  << "（两个接口基类都是空的，只多虚表指针）\n";
    }
}

// ---------- 6. 抽象类做模板方法 ----------
namespace template_method {
    class DataProcessor {
    public:
        virtual ~DataProcessor() = default;

        // 非虚的公开接口，定义流程骨架（NVI 惯用法）
        void process(const std::string& input) {
            std::cout << "        [1] 校验输入\n";
            if (!validate(input)) {
                std::cout << "        校验失败，中止\n";
                return;
            }
            std::cout << "        [2] 转换数据\n";
            auto result = transform(input);
            std::cout << "        [3] 输出: " << result << "\n";
        }

    protected:
        // 子类定制点
        virtual bool validate(const std::string& input) const = 0;
        virtual std::string transform(const std::string& input) const = 0;
    };

    class UpperCaseProcessor : public DataProcessor {
    protected:
        bool validate(const std::string& input) const override { return !input.empty(); }
        std::string transform(const std::string& input) const override {
            std::string r = input;
            for (char& c : r) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 32);
            return r;
        }
    };

    void demo() {
        UpperCaseProcessor p;
        p.process("hello world");
        p.process("");
        std::cout << "      模板方法：基类定义流程，派生类只填定制点\n";
    }
}

int main() {
    std::cout << "--- 1. 纯虚函数与抽象类 ---\n";    abstract_basics::demo();
    std::cout << "--- 2. 纯虚函数的实现 ---\n";      pure_virtual_with_body::demo();
    std::cout << "--- 3. 纯虚析构函数 ---\n";        pure_virtual_dtor::demo();
    std::cout << "--- 4. 虚析构函数的必要性 ---\n";  virtual_dtor::demo();
    std::cout << "--- 5. 接口类 ---\n";              interfaces::demo();
    std::cout << "--- 6. 模板方法 ---\n";            template_method::demo();
}
