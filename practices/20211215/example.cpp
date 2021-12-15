#include "repository.hpp"
#include <string>
using namespace abc;

int main(int argc, char** argv) {
    repository repo{};
    {
        auto e = repo.create();
        e.assign(1024, std::string{ "liff.engineer@gmail.com" });
    }
    {
        auto e = repo.create();
        e.assign(3.1415926, std::string{ "liff.engineer@gmail.com" });
    }
    
    std::vector<int> is{};
    std::vector<std::string> ss{};
    repo.each<int, std::string>([&](auto const& e) {
        if (e.exist<int>()) {
            is.emplace_back(e.view<int>());
        }
        if (e.exist<std::string>()) {
            ss.emplace_back(e.view<std::string>());
        }
        });
    repo.foreach<double,std::string>([&](int id,double dv,auto&& v) {
        ss.emplace_back(v);
        });
    return 0;
}
