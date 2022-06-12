#include "Registry.hpp"
#include "factory.h"
#include <iostream>
#include <string>

//常规的接口定义
class IReporter {
public:
    virtual ~IReporter() = default;
    virtual void Run() = 0;
};

//避免派生自IReporter可以考虑使用该包装类
template<typename T>
class ReporterImpl :public IReporter {
    T m_obj;
public:
    explicit ReporterImpl(T obj) :m_obj(std::move(obj)) {};

    void Run() override {
        m_obj.Run();
    }
};

template<typename T>
struct abc::IFactoryInject<T, IReporter> {
    using type = ReporterImpl<T>;
};

class ReporterFactory : public abc::Factory<IReporter,std::string>
{
public:
    using Super::Register;//使用基类的Register

    /// @brief 注册IReporter实现
    template<typename T, typename Arg = void>
    bool Register() {
        return Super::Register<T, Arg>(T::Code());
    }
};

template<typename T>
class Reporter :public IReporter {
    T m_obj;
public:
    explicit Reporter(T obj) :m_obj(std::move(obj)) {};

    static std::string Code() {
        return typeid(T).name();
    }

    void Run() override {
        std::cout << "(" << typeid(T).name() << "):" << m_obj << "\n";;
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

//指定IReporter的工厂为ReporterFactory
template<>
struct abc::RegistryFactoryInject<IReporter> {
    using type = ReporterFactory;
};

void TestRegistryFactory() {
    {//注册类实例
        auto factory = abc::Get<ReporterFactory>();
        factory->Register<IntReporter, int>("Int");
        factory->Register<DoubleReporter, double>("Double");
    }

    auto vp1 = abc::Make<IReporter>("Int", 1024);
    auto vp2 = abc::Make<IReporter>("Double", 3.333);
    if (vp1) {
        vp1->Run();
    }
    if (vp2) {
        vp2->Run();
    }
}

int main() {
    {
        TestRegistryFactory();
    }

    abc::Registry registry{};
    registry.emplace<IntReporter>(10);
    registry.emplace<DoubleReporter>(3.14);
    registry.emplace<Reporter<int>>(1234);
    registry.emplace<Reporter<double>>(1234.456);
    registry.bind<IReporter, Reporter<int>>();

    auto vp1 = registry.get<IntReporter>();
    auto vp2 = registry.get<DoubleReporter>();
    vp1->Run();
    vp2->Run();
    registry.bind<IReporter, Reporter<double>>();
    auto vp3 = registry.get<IReporter>();
    vp3->Run();
    return 0;
}

std::size_t abc::Registry::Impl::GetIndex(const char* code)
{
    static std::vector<std::string> codes;
    for (std::size_t i = 0; i < codes.size(); i++) {
        if (codes[i] == code) {
            return i;
        }
    }
    codes.emplace_back(code);
    return codes.size() - 1;
}
