#include "repository.hpp"
#include <string>
#include <iostream>
#include <tuple>
#include <algorithm>
#include <functional>
#include "nlohmann/json.hpp"

using namespace abc;

void example(int h, entity_view<> e, double* dVp, std::string* sVp) {
    std::cout << "h:" << h << "\ndV:" << *dVp << "\nsV:" << *sVp << "\n";
}

struct RuntimeEntity {
    entity_view<> e;
};

struct serializer_base {
    virtual ~serializer_base() = default;
};

struct json_serialzier_base {
    virtual ~json_serialzier_base() = default;
    virtual void  to(nlohmann::json& j, const entity_view<>& e) = 0;
    virtual void  from(const nlohmann::json& j, entity_view<>& e) = 0;
};

template<typename T>
struct json_serializer final :public json_serialzier_base {
    std::string code;

    json_serializer(std::string arg)
        :code(std::move(arg)){};

    void to(nlohmann::json& j, const entity_view<>& e) override {
        if (auto v = e.view<T>()) {
            j[code] = *v;
        }
    }

    void from(const nlohmann::json& j, entity_view<>& e) override {
        if (!j.contains(code)) return;
        T v{};
        j.at(code).get_to(v);
        e.emplace(std::move(v));
    }
};

struct RepositorySerializer {
    std::unordered_map<std::string_view, std::unique_ptr<json_serialzier_base>> serializers;

    template<typename T>
    RepositorySerializer& AddSerializer(const std::string& code) {
        serializers[abc::type_code_of<T>()] = std::make_unique<json_serializer<T>>(code);
        return *this;
    }

    nlohmann::json ToJson(repository& repo, const std::string& code) {
        nlohmann::json entitys;
        for (auto& e : repo) {
            nlohmann::json obj;
            for (auto& o : serializers) {
                o.second->to(obj, e);
            }
            entitys[std::to_string(e.key())] = std::move(obj);
        }

        nlohmann::json result;
        result[code] = std::move(entitys);
        return result;
    }

    repository  FromJson(const nlohmann::json& json, const std::string& code) {
        repository repo;
        if (json.contains(code)) {
            //首先遍历ID创建出足够的ID,以保证恢复到原始状态?
            auto&& entitys = json.at(code);
            int n = -1;
            for (auto& [k, dump] : entitys.items()) {
                auto id = std::stoi(k);
                n = std::max(id+1, n);
            }

            if (n > 0) {
                repo.resize(n);
            }

            for (auto& [k, je] : entitys.items()) {
                auto id = std::stoi(k);
                auto e = repo.at(id);
                if (!e) continue;
                for (auto& o : serializers) {
                    o.second->from(je, e);
                }
            }
        }
        return repo;
    }
};

#include <fstream>
#include <iomanip>

/// @brief 逻辑类型
template<typename T, typename Tag>
struct LogicType {
//    using UnderlyingType = T;
    T  v{};
};

template<typename T, typename U>
struct abc::project<LogicType<T, U>> :std::true_type {
    using type = T;

    static T& to(LogicType<T, U>& o) noexcept {
        return o.v;
    }

    static const T& to(const LogicType<T, U>& o) noexcept {
        return o.v;
    }
};

using  Description = LogicType<std::string, struct description_tag>;

struct TestObject1 {
    int* iV{};
    double* dV{};
    std::string sV{};
    std::string description{};

    static TestObject1 Build(entity_view<> e) {
        TestObject1 result{};
        e.project(result.iV, result.dV, result.sV,
            tie<Description>(result.description));
        return result;
    }
};

int main(int argc, char** argv) {
    repository repo{};
    {
        auto e = repo.emplace_back();
        e.emplace(1024,1.414, std::string{ "liff.engineer@gmail.com" });
    }
    {
        entity_view<double, std::string> e = repo.emplace_back();
        e.emplace(456,3.1415926, std::string{ "liff.cpplang@gmail.com" });

        e.apply(example, 10, e);
        e.emplace(RuntimeEntity{ e });
        e.emplace(Description{ "just description" });

        if (auto vp = e.view<Description>()) {
            std::cout << *vp << "\n";
        }

        auto desc = e.value_or<Description>(std::string{});

        auto to1 = e.as<abc::examples::TestObject>();

        abc::examples::TestObject to2;
        e.project(to2);

        auto obj = TestObject1::Build(e);
        if (obj.sV.empty()) {

        }
    }

    std::vector<abc::examples::TestObject> values;
    for (auto&& o : repo.project_views<abc::examples::TestObject>()) {
        values.emplace_back(o);
        o.dV = o.iV * 2.0;
    }

    auto strings = repo.values<std::string>();
    auto it = std::any_of(strings.begin(), strings.end(), [](const auto& v) {  return v.value() == std::string{"liff.cpplang@gmail.com"}; });

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
        auto string = e.value_or(std::string{});
    }

    for (auto&& v : repo.values<int>()) {
        is.emplace_back(v.value());
    }

    for (auto&& e : repo.views<double,std::string>()) {
        if (!e.contains<std::string>())
            continue;
        ss.emplace_back(*e.view<std::string>());
    }

    repository repo1 = repo;

    RepositorySerializer serializer{};
    serializer.AddSerializer<int>("intger")
        .AddSerializer<double>("number")
        .AddSerializer<std::string>("string");

    auto json = serializer.ToJson(repo1, "demo");
    std::string strJson = json.dump(4);

    auto repo2 = serializer.FromJson(json, "demo");
    return 0;
}
