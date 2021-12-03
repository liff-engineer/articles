#include "broker.hpp"
#include <iostream>

struct Actor
{
    broker::action_stub stub;
    void launch(broker& client) {
        stub += binder(*this).bind(client.endpoint<int, double>());
    }

    void reply(int query, double& reply) {
        reply *= query;
    }
};

void example() {
    broker client{};

    auto stub = bind(client.endpoint<int, double>(),
        [](auto&& query, auto&& reply) {
            if (query == 1) {
                reply = 1.414;
            }
            else if (query == 2) {
                reply = 1.73;
            }
            else {
                reply = 3.1415926;
            }
        });
    Actor actor{};
    actor.launch(client);

    {
        double dV = {};
        client.request(10, dV);
        std::cout << "responce:" << dV << "\n";
        dV = {};
        client.request(1, dV);
        std::cout << "responce:" << dV << "\n";
    }
    stub += bind(client.endpoint<int, double>(), [](auto&& query, auto&& reply) { reply = query * 2.0; });

    auto v1 = client.request(2, response<double>{});
    auto v2 = client.request(10, response<double>{});
}

int main(int argc, char** argv) {
    example();

    return 0;
}
