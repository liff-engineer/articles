#include "Registry.hpp"
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
struct abc::FactoryInject<T, IReporter> {
    using type = ReporterImpl<T>;
};


class ReporterFactory : public abc::Factory<IReporter>
{
public:
    using Super::Super;

    template<typename O, typename T = void>
    bool Register() {
        return Super::Register<O, T>(O::Code());
    }

    template<typename O, typename T = void,typename K = std::string>
    bool Register(K key) {
        return Super::Register<O, T>(key);
    }
};

template<>
struct abc::Maker<abc::Registry, IReporter> {
    template<typename... Args>
    std::unique_ptr<IReporter> Make(abc::Registry* registry, Args... args) {
        auto factory = abc::Get<ReporterFactory>(registry);
        return factory->Make(std::forward<Args>(args)...);
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

    ReporterFactory factory{};
    factory.Register<Reporter<int>,int>();
    factory.Register<Reporter<double>,double>();
    factory.Register<IntReporter, int>("Int");
    factory.Register<DoubleReporter, double>("Double");

    factory.SetDefaultArgument("int", 1024);
    factory.SetDefaultArgument("double", 3.1415926);

    std::vector<std::unique_ptr<IReporter>> results;
    results.emplace_back(factory.Make("int"));
    results.emplace_back(factory.Make("double"));
    results.emplace_back(factory.Make("int", 256));
    results.emplace_back(factory.Make("double", 1.171));

    results.emplace_back(factory.Make("Int", 128));
    results.emplace_back(factory.Make("Double", 1.414));

    //以下构造会失败
    results.emplace_back(factory.Make("Int"));
    results.emplace_back(factory.Make("Double"));
    for (auto&& o : results) {
        if (o) {
            o->Run();
        }
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
