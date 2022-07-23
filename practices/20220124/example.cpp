#include "complex_value.hpp"

int main(int argc, char** argv) {
    dynamic_tuple obj{};

    obj.reserve(3);
    obj.push_back(10);
    obj.push_back(true);
    obj.push_back(1.414);

    auto obj2 = obj;

    return 0;
}
