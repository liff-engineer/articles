#include <iostream>
#include <memory>
#include <vector>
#include <string>

class Aggregator {
public:
    Aggregator() = default;
    Aggregator(Aggregator&& other) noexcept = default;
    Aggregator& operator=(Aggregator&& other) noexcept = default;
protected:
    template<typename T, typename... Args>
    T* emplace(Args&&... args) {
        return emplace_impl<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    std::size_t erase() noexcept {
        auto code = typeid(T).name();
        std::size_t result{};
        for (auto& e : m_entrys) {
            if (!e || std::strcmp(e->code(), code) != 0)
                continue;
            e = nullptr;
            result += 1;
        }
        return result;
    }

    template<typename T>
    std::size_t erase(const T* obj) noexcept {
        auto code = typeid(T).name();
        std::size_t result{};
        for (auto& e : m_entrys) {
            if (!e || std::strcmp(e->code(), code) != 0)
                continue;
            if (auto vp = dynamic_cast<Entry<T>*>(e.get())) {
                if (vp->get() == obj) {
                    e = nullptr;
                    result += 1;
                }
            }
        }
        return result;
    }

    template<typename T, typename Fn>
    void visit(Fn&& fn) const noexcept {
        auto code = typeid(T).name();
        for (auto& e : m_entrys) {
            if (!e || std::strcmp(e->code(), code) != 0)
                continue;
            if (auto vp = dynamic_cast<Entry<T>*>(e.get())) {
                fn(vp->get());
            }
        }
    }
private:
    template<typename T, typename U>
    T* emplace_impl(U&& arg) {
        using H = typename Holder<T, std::remove_cv_t<std::remove_reference_t<U>>>::type;
        auto obj = std::make_unique<Value<T, H>>(std::forward<U>(arg));
        auto result = obj->get();
        m_entrys.emplace_back(std::move(obj));
        return result;
    }

    template<typename T, typename... Args>
    T* emplace_impl(Args&&... args) {
        auto obj = std::make_unique<Value<T>>(std::forward<Args>(args)...);
        auto result = obj->get();
        m_entrys.emplace_back(std::move(obj));
        return result;
    }
private:
    struct IEntry {
        virtual const char* code() const noexcept = 0;
    };

    template<typename I>
    struct Entry :public IEntry {
        virtual I* get() noexcept = 0;
        const char* code() const noexcept final { return typeid(I).name(); }
    };

    template<typename T, typename U, typename = void>
    struct Holder {
        using type = T;
    };

    template<typename T, typename U>
    struct Holder<T, U*, std::enable_if_t<std::is_base_of<T, U>::value>> {
        using type = U*;
    };

    template<typename T, typename U>
    struct Holder<T, std::shared_ptr<U>, std::enable_if_t<std::is_base_of<T, U>::value>> {
        using type = std::shared_ptr<U>;
    };

    template<typename T, typename U>
    struct Holder<T, std::unique_ptr<U>, std::enable_if_t<std::is_base_of<T, U>::value>> {
        using type = std::unique_ptr<U>;
    };

    template<typename T>
    struct Getter {
        template<typename U>
        T* get(U& obj) const noexcept { return &obj; }

        template<typename U>
        T* get(U* obj) const noexcept { return obj; }

        template<typename U>
        T* get(std::unique_ptr<U>& obj) const noexcept { return obj.get(); }

        template<typename U>
        T* get(std::shared_ptr<U>& obj) const noexcept { return obj.get(); }
    };

    template<typename T, typename Holder = T>
    class Value final :public Entry<T> {
    public:
        template<typename... Args>
        explicit Value(Args&&... args)
            :m_obj(std::forward<Args>(args)...)
        {};
        T* get() noexcept override { return Getter<T>{}.get(m_obj); }
    private:
        Holder m_obj;
    };
private:
    std::vector<std::unique_ptr<IEntry>> m_entrys;
};

class ITask {
public:
    virtual ~ITask() = default;
    virtual void run() = 0;
};

class Task :public ITask {
public:
    void run() override {
        std::cout << "Task::run()\n";
    }
};

class ServiceAggregator :private Aggregator {
public:
    using Aggregator::emplace;
    using Aggregator::erase;
    using Aggregator::visit;
};


void print(const ServiceAggregator& hub) {
    hub.visit<std::string>([](std::string* obj) {
        if (obj) {
            std::cout << *obj << "\n";
        }
        });
}

int main() {
    Aggregator obj{};

    Aggregator other{ std::move(obj) };

    ServiceAggregator hub{};
    auto v = hub.emplace<std::string>("liff.engineer@gmail.com");
    hub.emplace<std::string>(std::make_unique<std::string>("unique_ptr"));
    hub.emplace<std::string>(std::make_shared<std::string>("shared_ptr"));
    hub.emplace<std::string>(v);
    hub.emplace<ITask>(std::make_unique<Task>());

    print(hub);

    auto result = hub.erase<ITask>();
    return 0;
}
