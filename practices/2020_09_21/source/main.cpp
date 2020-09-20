#include "example_ostream.hpp"
#include <iostream>

int main(int argc, char **argv)
{
    MyType type = MyType::B;
    std::cout << "type example:" << type << "\n";
    Address address;
    address.zipcode = "123455";
    address.detail = "liff.engineer@gmail.com";
    std::cout << "address example" << address << "\n";
    return 0;
}