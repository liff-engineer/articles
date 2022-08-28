#include "TaskRegistry.hpp"
#include <iostream>

struct Printer:public abc::TaskConcept<std::string,void>{};

struct StringBuilder:public abc::TaskConcept<std::string,std::string,const std::string&>{};

class ITask {
public:
    virtual ~ITask() = default;
    virtual void exec(const std::string& header) const noexcept = 0;
};

template<typename T>
class Task :public ITask {
    T m_obj;
public:
    explicit Task(T obj) :m_obj(std::move(obj)) {};

    void exec(const std::string& header) const noexcept {
        std::cout << header << ":" << m_obj << "\n";
    }
};

struct ITaskConcept {
    using index_type = std::string;
    using type = std::unique_ptr<ITask>;

    static void Run(const type* op, const std::string& header) {
        if (op) (*op)->exec(header);
    }
};

template<>
struct abc::TaskConceptInject<ITask> {
    using type = ITaskConcept;
};

int main()
{
    abc::TaskRegistry obj{};

    obj.add<Printer>("0", []() {
        std::cout << "0\n";
        });
    obj.add<Printer>("1", []() {
        std::cout << "1\n";
        });
    obj.add<StringBuilder>("email", [](auto&& obj)->std::string { return "liff.engineer@gmail.com"; });

    obj.run<Printer>("0");

    obj.run<Printer>("3");

    //obj.run<StringBuilder>("email","");
    std::cout << obj.run<StringBuilder>("email","") << "\n";

    obj.add<ITask>("int", std::make_unique<Task<int>>(10));
    obj.add<ITask>("double", std::make_unique<Task<double>>(3.1415926));
    obj.add<ITask>("string", std::make_unique<Task<std::string>>("liff.engineer@gmail.com"));

    obj.remove<ITask>("string");

    obj.run<ITask>("int", "iV");
    obj.run<ITask>("string", "sV");
    obj.run<ITask>("double", "dV");
    return 0;
}
