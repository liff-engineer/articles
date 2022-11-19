#include "ICommand.hpp"
#include "IExtension.hpp"

int main() {

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
