#include "Actor.hpp"
#include <iostream>
#include <string>

using namespace abc;

struct Printer {

    template<typename E>
    void On(const E& e) {
        std::cout << "printer:" << e << "\n";
    }
};

int main() {
    Broker broker{};
    Printer printer{};
    auto s1 = broker.Subscribe<int>(printer);
    auto s2 = broker.Subscribe<std::string>(printer);
    
    Broker other{};
    auto s3 = broker.Connect(other);
    auto s4 = other.Subscribe<std::string>(printer);

    broker.Publish(10);
    broker.Publish(std::string{ "liff-b@glodon.com" });
    broker.Publish(1024);

    ActorFactory factory{};
    return 0;
}
