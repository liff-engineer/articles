#include "ICommand.hpp"
#include "IExtension.hpp"
#include <iostream>

template<typename... Args>
void  CreateCommand(Args&&... args) {};

class Command {
public:
    struct Tag {};

    template<typename... Args>
    Command(Args&&... args)
    {
        CreateCommand(Command::Tag{}, std::forward<Args>(args)...);
    }
};

//配合之前的那个派生的TAG，可以选择的

class MyString {
public:
    friend void CreateCommand(Command::Tag) {
        std::cout << "MyString\n";
    }
};


int main() {
    auto obj = Command{};

    for (auto& code : {"CommandA","CommandB","MyCommand-X","MyCommand-Y" ,"MyCommand-Z" }) {
        if (auto v = ICommand::Create(code)) {
            v->run();
        }
    }
    
    auto&& ext = IExtension::Get();
    ext.doX();
    ext.doY();
    return 0;
}
