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
        using H = typename Holder<T, Args...>::type;
        auto obj = std::make_unique<Value<T, H::type>>(H::get, std::forward<Args>(args)...);
        auto result = obj->get();
        m_entrys.emplace_back(std::move(obj));
        return result;
    }

    template<typename T>
    std::size_t erase() noexcept {
        static auto code = typeid(T).name();
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
        static auto code = typeid(T).name();
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
        static auto code = typeid(T).name();
        for (auto& e : m_entrys) {
            if (!e || std::strcmp(e->code(), code) != 0)
                continue;
            if (auto vp = dynamic_cast<Entry<T>*>(e.get())) {
                fn(vp->get());
            }
        }
    }
private:
    template<typename T, typename U, typename E = void>
    struct HolderTrait {
        using type = std::conditional_t<std::is_base_of_v<T, U>, U, T>;
        static T* get(type& obj) { return &obj; }
    };

    template<typename T, typename U>
    struct HolderTrait<T, U*, std::enable_if_t<std::is_base_of_v<T, U>>> {
        using type = U*;
        static T* get(type& obj) { return obj; }
    };

    template<typename T, typename U>
    struct HolderTrait<T, std::shared_ptr<U>, std::enable_if_t<std::is_base_of_v<T, U>>> {
        using type = std::shared_ptr<U>;
        static T* get(type& obj) { return obj.get(); }
    };

    template<typename T, typename U>
    struct HolderTrait<T, std::unique_ptr<U>, std::enable_if_t<std::is_base_of_v<T, U>>> {
        using type = std::unique_ptr<U>;
        static T* get(type& obj) { return obj.get(); }
    };

    template<typename T, typename...Args>
    struct Holder {
        using type = HolderTrait<T, T>;
    };

    template<typename T, typename U>
    struct Holder<T, U> {
        using type = HolderTrait<T, std::remove_cv_t<std::remove_reference_t<U>>>;
    };
private:
    struct IEntry {
        virtual const char* code() const noexcept = 0;
    };

    template<typename T>
    struct Entry :public IEntry {
        virtual T* get() noexcept = 0;
        const char* code() const noexcept final { return typeid(T).name(); }
    };

    template<typename T, typename Holder>
    struct Value final :Entry<T> {
    public:
        template<typename... Args>
        explicit Value(T* (*op)(Holder&), Args&&... args)
            :m_obj(std::forward<Args>(args)...), m_op(op) {};
        T* get() noexcept override { return m_op(m_obj); }
    private:
        Holder m_obj;
        T* (*m_op)(Holder&);
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
    hub.emplace<std::string>(v->begin(),v->end());
    hub.emplace<std::string>(v);
    hub.emplace<ITask>(std::make_unique<Task>());
    hub.emplace<ITask>(Task{});

    print(hub);

    auto result = hub.erase<ITask>();
    return 0;
}
