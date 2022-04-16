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
    auto s1 = broker.subscribe<int>(printer);
    auto s2 = broker.subscribe<std::string>(printer);
    
    Broker other{};
    auto s3 = broker.connect(other);
    auto s4 = other.subscribe<std::string>(printer);

    broker.publish(10);
    broker.publish(std::string{ "liff-b@glodon.com" });
    broker.publish(1024);

    Registry registry;
    auto vp = registry.emplace<ActorFactory>();
    vp->registerMaker("abc", []()->std::unique_ptr<IActor> { return nullptr; });
    auto fvp = registry.at<ActorFactory>();
    ActorFactory factory{};
    return 0;
}
