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
        e.assign(std::move(v));
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
            entitys[std::to_string(e.id())] = std::move(obj);
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
    using UnderlyingType = T;
    T  v{};
};


template<typename T, typename U>
struct abc::logic_type_traits<LogicType<T, U>> : std::true_type {
    using underlying_type = T;

    T& get(LogicType<T, U>& o) const noexcept { return o.v; }
    const T& get(const LogicType<T, U>& o) const noexcept { return o.v; }
};

using  Description = LogicType<std::string, struct description_tag>;

struct TestObject {
    int* iV{};
    double* dV{};
    std::string sV{};
    std::string description{};

    static TestObject Build(entity_view<> e) {
        TestObject result{};
        e.project(result.iV, result.dV, result.sV,
            tie<Description>(result.description));
        return result;
    }
};

int main(int argc, char** argv) {
    repository repo{};
    {
        auto e = repo.create();
        e.assign(1024, std::string{ "liff.engineer@gmail.com" });
    }
    {
        entity_view<double, std::string> e = repo.create();
        e.assign(3.1415926, std::string{ "liff-b@glodon.com" });

        e.apply(example, 10, e);
        e.assign(RuntimeEntity{ e });
        e.assign(Description{ "just description" });

        auto obj = TestObject::Build(e);
        if (obj.sV.empty()) {

        }
    }

    auto e = repo.find(std::string{ "liff-b@glodon.com" });
    auto e1 = repo.find_if<std::string>([](const std::string & v) { return v.find("liff.engineer")!= v.npos; });
    
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

    for (auto&& e : repo.iterator<double,std::string>()) {
        if (!e.exist<std::string>())
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

/// @brief 逻辑值对象
/// @tparam T 存储值
/// @tparam Tag 类型标签
template<typename T, typename Tag>
class LogicVO {
    T m_v{};
public:
    using UnderlyingType = T;

    LogicVO() = default;

    explicit LogicVO(T const& v) noexcept(std::is_nothrow_copy_constructible_v<T>)
        :m_v(v) {};

    template<typename U = T, typename = std::enable_if_t<!std::is_reference_v<U>, void>>
    explicit LogicVO(T&& v) noexcept(std::is_nothrow_move_constructible_v<T>)
        :m_v(std::move(v)) {};

    T& get() noexcept { return m_v; }
    std::remove_reference_t<T> const& get() const noexcept { return m_v; }

    operator LogicVO<T&, Tag>() {
        return LogicVO<T&, Tag>(m_v);
    }

    explicit operator T() const noexcept { return m_v; }
};

//using HasArgument = LogicVO<bool, struct HasArgumentTag>;
