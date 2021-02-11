#include "factory.hpp"
#include <iostream>


std::string trimClass(std::string v) {
    auto  i = v.find("class");
    if (i != v.npos) {
        i += strlen("class");
    }
    return v.substr(v.find_first_not_of(' ', i));
}

class ICommand :public Factory<ICommand,std::string>
{
public:
    using Identify_t = Identify<int>;

    template<typename T>
    static Identify_t Id() {
        Identify_t result;
        result.key = typeid(T).name();
        result.key = trimClass(result.key);

        result.payload = T::order;
        return result;
    }
public:
    ICommand(Key) {};
    virtual ~ICommand() = default;
    virtual void execute() const noexcept = 0;
};

class PrintCommand :public ICommand::Registrar<PrintCommand>
{
    std::string m_msg;
public:
    PrintCommand(std::string msg):m_msg(msg) {};
    void execute() const noexcept override {
        std::cout << "Print:"<<m_msg<<"\n";
    }

public:
    static const int order = 1;
};

class ReportCommand :public ICommand::Registrar<ReportCommand>
{
    std::string m_msg;
public:
    ReportCommand(std::string msg) :m_msg(msg) {};
    void execute() const noexcept override {
        std::cout << "Report:" << m_msg << "\n";
    }

public:
    static const int order = 2;
};

//class TCommand :public ICommand
//{
//    std::string m_msg;
//public:
//    TCommand(std::string msg) :m_msg(msg) {};
//    void execute() const noexcept override {
//        std::cout << "Report:" << m_msg << "\n";
//    }
//};

int main(int argc, char** argv) {

    ICommand::Visit([](auto& e) {
        std::cout << e.key << "\n";
        std::cout << e.payload << "\n";
        });

    ICommand::Make("PrintCommand", "what?")->execute();


    return 0;
}
