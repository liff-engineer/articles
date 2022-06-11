#include "factory.h"
#include <iostream>

//常规的接口定义
class IReporter {
public:
    virtual ~IReporter() = default;
    virtual void Run() = 0;
};

//工厂定义
class ReporterFactory : public abc::Factory<IReporter, std::string>
{
public:
    using Super::Register;//使用基类的Register

    /// @brief 注册IReporter实现
    template<typename T, typename Arg = void>
    bool Register() {
        return Super::Register<T, Arg>(T::Code());
    }
};

//如果需要避免派生自IReporter可以考虑使用该包装类
template<typename T>
class ReporterImpl :public IReporter {
    T m_obj;
public:
    explicit ReporterImpl(T obj) :m_obj(std::move(obj)) {};

    void Run() override {
        m_obj.Run();
    }
};

//当类型T通过ReporterImpl提供IReporter派生类时可以注入,工厂就可以识别
template<typename T>
struct abc::IFactoryInject<T, IReporter> {
    using type = ReporterImpl<T>;
};

/// @brief IReporter实现示例
template<typename T>
class Reporter :public IReporter {
    T m_obj;
public:
    explicit Reporter(T obj) :m_obj(std::move(obj)) {};

    static std::string Code() {
        if constexpr (std::is_same_v<T, std::string>) {
            return "std::string";
        }
        else {
            return typeid(T).name();
        }
    }

    void Run() override {
        std::cout << "(" << Code() << "):" << m_obj << "\n";;
    }
};

struct IntReporter
{
    int iV;
    IntReporter(int v) :iV(v) {};
    void Run() {
        std::cout << "(IntReporter):" << iV << "\n";;
    }
};

struct DoubleReporter
{
    double dV;
    DoubleReporter(double v) :dV(v) {};
    void Run() {
        std::cout << "(DoubleReporter):" << dV << "\n";;
    }
};

int main() {

    ReporterFactory factory{};
    factory.Register<Reporter<int>, int>();
    factory.Register<Reporter<double>, double>();
    factory.Register<Reporter<std::string>, std::string>();
    factory.Register<IntReporter, int>("Int");
    factory.Register<DoubleReporter, double>("Double");

    factory.SetDefaultArgument("int", 1024);
    factory.SetDefaultArgument("double", 3.1415926);
    factory.SetDefaultArgument("std::string", std::string{"liff.engineer@gmail.com"});

    std::vector<std::unique_ptr<IReporter>> results;
    results.emplace_back(factory.Make("int"));
    results.emplace_back(factory.Make("double"));
    results.emplace_back(factory.Make("int", 256));
    results.emplace_back(factory.Make("double", 1.171));

    results.emplace_back(factory.Make("Int", 128));
    results.emplace_back(factory.Make("Double", 1.414));
    results.emplace_back(factory.Make("std::string"));

    //以下构造会失败
    results.emplace_back(factory.Make("Int"));
    results.emplace_back(factory.Make("Double"));
    for (auto&& o : results) {
        if (o) {
            o->Run();
        }
    }

    return 0;
}
