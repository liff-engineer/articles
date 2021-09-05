#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <vector>
#include <tuple>

//std::apply µœ÷
namespace std
{
    namespace detail {
        template <class F, class Tuple, std::size_t... I>
        constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
        {
            // This implementation is valid since C++20 (via P1065R2)
            // In C++17, a constexpr counterpart of std::invoke is actually needed here
            return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
        }
    }  // namespace detail

    template <class F, class Tuple>
    constexpr decltype(auto) apply(F&& f, Tuple&& t)
    {
        return detail::apply_impl(
            std::forward<F>(f), std::forward<Tuple>(t),
            std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
    }
}

struct IStore {
    virtual ~IStore() = default;
};

template<typename Arg, typename R>
struct Store :public IStore {
    std::vector<std::pair<Arg, R>> items;

    void emplace_back(Arg&& arg, R result) {
        items.emplace_back(std::make_pair(arg, result));
    }

    auto begin() const {
        return items.begin();
    }

    auto end() const {
        return items.end();
    }
};

class Registry {
    std::map<std::string, std::unique_ptr<IStore>> m_stores;
public:
    Registry() = default;

    template<typename Arg, typename R>
    Store<Arg, R>* of(std::string const& code) noexcept {
        auto it = m_stores.find(code);
        if (it != m_stores.end()) {
            return dynamic_cast<Store<Arg, R>*>(it->second.get());
        }
        m_stores[code] = std::make_unique<Store<Arg, R>>();
        return dynamic_cast<Store<Arg, R>*>(m_stores.at(code).get());
    }

    template<typename Arg, typename R>
    const Store<Arg, R>* of(std::string const& code) const noexcept {
        auto it = m_stores.find(code);
        if (it != m_stores.end()) {
            return dynamic_cast<const Store<Arg, R>*>(it->second.get());
        }
        return nullptr;
    }
public:
    static Registry* gRegistry() noexcept;
};

namespace v2
{
    template<typename Sig,typename Fn>
    struct Callable;

    template<typename R, typename... Args, typename Fn>
    struct Callable<R(Args...), Fn> {
        Fn fn;
        std::string name;

        template<typename F>
        Callable(F&& fn, std::string name)
            :fn(fn), name(std::move(name)) {};

        template<typename... InnerArgs>
        R operator()(InnerArgs&&... args) const {
            return fn(std::forward<InnerArgs>(args)...);
        }
    };

    template<typename Sig, typename Fn>
    struct Gather;

    template<typename R, typename... Args, typename Fn>
    struct Gather<R(Args...), Fn>
    {
        Fn fn;
        std::string name;
        using Arg = std::tuple<std::decay_t<Args>...>;
        Store<Arg, R>* store;

        template<typename F>
        Gather(F&& fn, std::string name)
            :fn(fn), name(std::move(name)) {
            store = Registry::gRegistry()->of<Arg, R>(this->name);
        };

        template<typename... InnerArgs>
        R operator()(InnerArgs&&... args) {
            auto pack = std::make_tuple(args...);
            auto result = fn(std::forward<InnerArgs>(args)...);
            store->emplace_back(std::move(pack), result);
            return result;
        }
    };

    template<typename Sig, typename Fn>
    struct Replayer;

    template<typename R, typename... Args, typename Fn>
    struct Replayer<R(Args...),Fn>
    {
        Fn fn;
        std::string name;
        using Arg = std::tuple<std::decay_t<Args>...>;
        Store<Arg, R>* store;

        template<typename F>
        Replayer(F&& fn, std::string name)
            :fn(fn), name(std::move(name)) {
            store = Registry::gRegistry()->of<Arg, R>(this->name);
        };

        template<typename... InnerArgs>
        R operator()(InnerArgs&&... args) const {
            return fn(std::forward<InnerArgs>(args)...);
        }

        void replay() {
            std::pair<int, int> results;
            for (auto& obj : *store) {
                results.first += 1;
                auto& args = obj.first;
                if (obj.second != std::apply(*this, args)) {
                    results.second += 1;
                    std::cout << name << " replay failed.\n";
                }
            }
            std::cout<< name << " replay " << results.first << " times, failed " << results.second << " times.\n";
        }
    };

    template<typename Sig, typename Fn>
    Callable<Sig, std::decay_t<Fn>> CallableOf(Fn&& fn, std::string name) {
        return { std::forward<Fn>(fn),std::move(name) };
    }

    template<typename Sig, typename Fn>
    Gather<Sig, std::decay_t<Fn>> GatherOf(Fn&& fn, std::string name) {
        return { std::forward<Fn>(fn),std::move(name) };
    }

    template<typename Sig, typename Fn>
    Replayer<Sig, std::decay_t<Fn>> ReplayerOf(Fn&& fn, std::string name) {
        return { std::forward<Fn>(fn),std::move(name) };
    }
}

int add(int a, int b) {
    return a + b;
}

std::string addToString(int a, std::string const& v) {
    return std::to_string(a) + "+" + v;
}

void exampleV2()
{
    using namespace v2;
    auto addGather = GatherOf<int(int, int)>(add, "add");
    auto mulGather = GatherOf<int(int, int)>([](int a, int b)->int { return a * b; }, "mul");
    auto stringAddGather = GatherOf<std::string(int, std::string const&)>(addToString, "addToString");
    std::vector<int> results;
    for (auto& v : {
        std::make_pair(1,2),
        std::make_pair(3,4),
        std::make_pair(5,6),
        std::make_pair(7,8),
        std::make_pair(9,10)
        }) {
        results.push_back(addGather(v.first, v.second));
        mulGather(v.first, v.second);
        stringAddGather(v.first, std::to_string(v.second));
    }

    auto addReplayer = ReplayerOf<int(int, int)>([](int a, int b)->int {
        auto r = a + b;
        if (r > 10) {
            return r - 10;
        }
        return r;
        }, "add");

    addReplayer.replay();
    auto stringAddReplayer = ReplayerOf<std::string(int, std::string const&)>(addToString, "addToString");
    stringAddReplayer.replay();
}

int main(int argc, char** argv) {
    exampleV2();
    return 0;
}

Registry* Registry::gRegistry() noexcept
{
    static Registry regsitry;
    return &regsitry;
}
