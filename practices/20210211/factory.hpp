#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <type_traits>

//http://www.nirfriedman.com/2018/04/29/unforgettable-factory/
template<typename I, typename... Args>
class Factory {
public:
    template<typename T = void>
    struct Identify;

    template<>
    struct Identify<void> {
        std::string key;
    };

    template<typename T>
    struct Identify {
        std::string key;
        T           payload;
    };

    struct Creator :I::Identify_t
    {
        Creator() = default;
        Creator(typename I::Identify_t arg)
            :I::Identify_t(arg) {};

        std::unique_ptr<I>(*creator)(Args...) = nullptr;
    };

    template<typename... Ts>
    static std::unique_ptr<I> Make(std::string const& k, Ts... args) {
        return creators().at(k).creator(std::forward<Ts>(args)...);
    }

    template<typename Fn>
    static void Visit(Fn&& fn) {
        for (auto& [k, e] : creators()) {
            fn(e);
        }
    }

    template<typename T>
    struct Registrar :I {
        friend T;

        static bool AutoRegister() {
            Creator creator = I::Id<T>();
            creator.creator = [](Args... args)->std::unique_ptr<I> {
                return std::make_unique<T>(std::forward<Args>(args)...);
            };
            Factory::creators()[creator.key] = creator;
            return true;
        }
        static bool registered;
    private:
        Registrar() :I(Key{}) {
            (void)registered;
        };
    };
protected:
    class Key {
        Key() {};

        template<typename T>
        friend struct Registrar;
    };

    Factory() = default;

    static auto& creators() {
        static std::unordered_map<std::string, Creator> d;
        return d;
    }
};

template<typename I, typename... Args>
template<typename T>
bool Factory<I, Args...>::Registrar<T>::registered = Factory<I, Args...>::Registrar<T>::AutoRegister();
