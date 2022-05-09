#include <iostream>
#include <algorithm>
#include "Repo.hpp"

using namespace abc;


struct RuntimeEntity {
    EntityView e;
};

/// @brief 逻辑类型
template<typename T, typename Tag>
struct LogicType {
    //    using UnderlyingType = T;
    T  v{};
};

using  Description = LogicType<std::string, struct description_tag>;


//#include <iostream>

void RepoTest() {
    using namespace abc::v1;

    im::ValueArray<double> dVs{ 8 };
    dVs.emplace(0, 1023.5);
    dVs.emplace(1, 3.1415);
    dVs.emplace(2, 1.7);
    dVs.emplace(4, 2.3);

    for (auto&& o : im::ValueArrayProxy<double>{ &dVs }) {
        std::cout << o.key() << ":" << o.value() << "\n";
    }
    if (auto vp = dVs[2]) {
        *vp = 1.717;
    }
    auto size = dVs.size();
    if (dVs.contains(1)) {
        std::cout << "contains(1)\n";
    }
    dVs.erase(0);
    auto clone = dVs.clone();
    if (dVs.empty()) {

    }


    Repository repo{};
    {
        auto e = repo.emplace_back();
        e.emplace(1024, 1.414, std::string{ "liff.engineer@gmail.com" });
    }
    {
        auto e = repo.emplace_back();
        e.emplace(456, 3.1415926, std::string{ "liff.cpplang@gmail.com" });
        if (auto vp = e.view<std::string>()) {
            std::cout << *vp << "\n";
        }
    }

    auto strings = repo.values<std::string>();
    auto it = std::any_of(strings.begin(), strings.end(), [](const auto& v) {  return v.value() == std::string{ "liff.cpplang@gmail.com" }; });

    auto e = repo.find(std::string{ "liff.cpplang@gmail.com" });
    std::vector<std::string> ss{};
    for (auto&& e : repo) {
        if (!e.contains<std::string>())
            continue;
        ss.emplace_back(*e.view<std::string>());
    }

    if (ss.empty()) {

    }
}

int main(int argc, char** argv) {
    {
        RepoTest();
        //return 0;
    }
    Repository repo{};
    {
        auto e = repo.emplace_back();
        e.emplace(1024, 1.414, std::string{ "liff.engineer@gmail.com" });
    }
    {
        auto e = repo.emplace_back();
        e.emplace(456, 3.1415926, std::string{ "liff.cpplang@gmail.com" });

        e.emplace(RuntimeEntity{ e });
        e.emplace(Description{ "just description" });

        if (auto vp = e.view<Description>()) {
            std::cout << vp->v << "\n";
        }
    }

    auto strings = repo.values<std::string>();
    auto it = std::any_of(strings.begin(), strings.end(), [](const auto& v) {  return v.value() == std::string{ "liff.cpplang@gmail.com" }; });

    auto e = repo.find(std::string{ "liff.cpplang@gmail.com" });

    std::vector<int> is{};
    std::vector<std::string> ss{};
    for (auto&& e : repo) {
        if (auto v = e.view<int>()) {
            is.emplace_back(*v);
        }
        if (auto v = e.view<std::string>()) {
            ss.emplace_back(*v);
        }
    }

    for (auto&& v : repo.values<int>()) {
        is.emplace_back(v.value());
    }

    for (auto&& e : repo) {
        if (!e.contains<std::string>())
            continue;
        ss.emplace_back(*e.view<std::string>());
    }
    return 0;
}
