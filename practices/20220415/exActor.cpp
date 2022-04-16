#include "Actor.hpp"
#include <iostream>
#include <string>

using namespace abc;

struct Printer {

    template<typename E>
    void on(const E& e) {
        std::cout << "printer:" << e << "\n";
    }
};

int main() {
    Broker broker{};
    broker.description = "example";
    Printer printer{};
    auto s1 = broker.subscribe<int>(printer);
    auto s2 = broker.subscribe<double>(printer);
    
    Broker other{};
    other.description = "hub";
    auto s3 = broker.connect(other);
    auto s4 = other.subscribe<double>(printer);

    broker.publish(10);
    broker.publish(3.1415926);
    broker.publish(1024);

    ActorFactory factory{};
    std::unique_ptr<IActor> actor;
    if (factory.contains("abc")) {
        actor = factory.make("abc");
    }
    return 0;
}
