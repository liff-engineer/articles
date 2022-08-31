#include <string>
#include "As.hpp"

struct MyObject
{
    std::string v;

    template<typename T>
    decltype(auto) As() const noexcept {
        return abc::AsImpl<T,decltype(v)>::As(v);
    }

    template<typename T>
    decltype(auto) As() noexcept {
        return abc::AsImpl<T,decltype(v)>::As(v);
    }
};

template<>
struct abc::AsImpl<double, std::string>
{
    static double As(const std::string& v) {
        return std::stod(v);
    }
};

template<>
struct abc::AsImpl<float, std::string>
{
    static float As(const std::string& v) {
        return std::stof(v);
    }
};

template<typename T>
struct abc::AsImpl<T, std::string, std::enable_if_t<std::is_signed_v<T>>>
{
    static T As(const std::string& v) {
        return std::stoll(v);
    }
};

template<typename T>
struct abc::AsImpl<T, std::string, std::enable_if_t<std::is_unsigned_v<T>>>
{
    static T As(const std::string& v) {
        return std::stoull(v);
    }
};

int main()
{
    MyObject obj{ "3.1415926" };

    auto dV = obj.As<double>();
    auto iV = obj.As<int>();
    auto fV = obj.As<float>();
    obj.v = "1024";
    auto lV = obj.As<long long>();
    auto uV = obj.As<unsigned long long>();
    return 0;
}
