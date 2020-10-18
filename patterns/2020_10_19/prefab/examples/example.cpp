#include <string>
#include <iostream>
#include "prefab/Repository.hpp"

//虚拟数据组件,整合,只为查看
struct All
{
    int iv;
    double dv;
    std::string sv;
};

//这个需要写在应用程序公共部分
template <>
class prefab::IComponent<All> : public prefab::IComponentBase
{
public:
    HashedStringLiteral typeCode() const noexcept override
    {
        return HashedTypeNameOf<All>();
    }

public:
    virtual All view() const = 0;
};

//内存版实现
template <>
class prefab::VirtualComponent<All> : public prefab::IComponent<All>
{
    prefab::Repository *m_repo = nullptr;
    long long m_key = 0;

public:
    explicit VirtualComponent(prefab::Repository *repo, long long key)
        : prefab::IComponent<All>(), m_repo(repo), m_key(key){};

    prefab::HashedStringLiteral implTypeCode() const noexcept override
    {
        return prefab::HashedTypeNameOf<prefab::VirtualComponent<All>>();
    }

    template <typename T>
    prefab::ValueImpl<std::map<long long, T>> *repoOf() const noexcept
    {
        auto typeCode = prefab::HashedTypeNameOf<T>();
        auto base = m_repo->m_voRepos.at(typeCode).get();
        auto impl = static_cast<prefab::ValueImpl<std::map<long long, T>> *>(base);
        return impl;
    }

    template <typename T>
    T valueOf() const noexcept
    {
        return repoOf<T>()->value().at(m_key);
    }

    All view() const noexcept override
    {
        All result;
        result.iv = valueOf<int>();
        result.dv = valueOf<double>();
        result.sv = valueOf<std::string>();
        return result;
    }

    void assign(All const &o) noexcept
    {
        repoOf<int>()->value()[m_key] = o.iv;
        repoOf<double>()->value()[m_key] = o.dv;
        repoOf<std::string>()->value()[m_key] = o.sv;
    }

    static void write(IComponentBase *handler, Value const &argument) noexcept
    {
        static_cast<prefab::VirtualComponent<All> *>(handler)->assign(argument.as<All>());
    }
};

template <>
struct prefab::VirtualComponentHandler<All>
{
    static bool accept(Repository *repo, long long id, Require const &req) noexcept
    {
        if (req.remark("sv") != prefab::HashedStringLiteral("contain"))
            return false;
        prefab::VirtualComponent<All> e{repo, id};
        return e.valueOf<std::string>().find(req.as<All>().sv) != std::string::npos;
    }
};

using namespace prefab;
void testRepo()
{
    Repository repo;
    repo.registerComponentRepo<int>();
    repo.registerComponentRepo<double>();
    repo.registerComponentRepo<std::string>();
    repo.registerVirualComponent<All>();

    //创建实体1
    auto e1 = repo.create<int, double, std::string>();
    e1.assign<std::string>("liff.engineer@gmail.com");
    e1.assign<double>(3.1415926);
    e1.assign<int>(1018);

    //以参数创建
    auto e2 = repo.create<All>(All{1024, 1.717, "liff.developer@glodon.com"});

    //根据要求查找第一个
    auto findReq = Require(All{0, 0.0, "liff.engineer"}).remark("sv"_hashed, "contain"_hashed);
    auto typeCode = findReq.typeCode();

    auto e = repo.find<All>(findReq);
    if (!e)
        return;
    auto allCopy = e.view<All>();

    std::vector<std::string> results;
    auto searchReq = Require(All{0, 0.0, "liff"}).remark("sv"_hashed, "contain"_hashed);
    for (auto e : repo.search<All, std::string>(searchReq))
    {
        results.emplace_back(e.view<std::string>());
    }
}
int main(int argc, char **argv)
{
    testRepo();
    return 0;
}
