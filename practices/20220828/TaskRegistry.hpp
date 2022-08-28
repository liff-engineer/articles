/// 任务注册处(这里的任务指类似于函数,能且只能被调用)
/// 任何任务Task概念需要满足以下条件
/// a) 定义了index_type作为键类型
/// b) 定义了type作为任务类型
/// c) [可选]提供了静态的Run函数,接收const type*及参数列表,来执行任务 
/// 
/// 可以特化TaskConceptInject使得某T的Task概念定位到特定类型

#pragma once 

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace abc
{

    template<typename T>
    struct TaskConceptInject {
        using type = T;
    };

    class TaskRegistry {
        template<typename T>
        using Task = typename TaskConceptInject<T>::type;
    public:
        template<typename T, typename I, typename... Args>
        void add(I&& idx, Args&&... args) {
            if (auto vp = of<T>()) {
                return vp->add(std::forward<I>(idx), std::forward<Args>(args)...);
            }
        }

        template<typename T, typename I>
        void remove(I&& idx) {
            if (auto vp = of<T>()) {
                return vp->remove(std::forward<I>(idx));
            }
        }

        template<typename T, typename I>
        auto find(I&& idx) const noexcept {
            const Task<T>::type* obj{};
            if (auto vp = of<T>()) {
                obj = vp->find(std::forward<I>(idx));
            }
            return obj;
        }

        template<typename T, typename I, typename... Args>
        auto run(I&& idx, Args&&... args) const noexcept {
            return Task<T>::Run(find<T>(std::forward<I>(idx)), std::forward<Args>(args)...);
        }
    private:
        template<typename T>
        class Store;

        template<typename T>
        Store<Task<T>>* of() {
            static const auto code = typeid(Store<Task<T>>).name();
            for (auto&& store : m_stores) {
                if (!store->Is(code))
                    continue;
                if (auto vp = dynamic_cast<Store<Task<T>>*>(store.get())) {
                    return vp;
                }
            }
            auto store = std::make_unique<Store<Task<T>>>();
            auto result = store.get();
            m_stores.emplace_back(std::move(store));
            return result;
        }

        template<typename T>
        const Store<Task<T>>* of() const noexcept {
            static const auto code = typeid(Store<Task<T>>).name();
            for (auto&& store : m_stores) {
                if (!store->Is(code))
                    continue;
                if (auto vp = dynamic_cast<const Store<Task<T>>*>(store.get())) {
                    return vp;
                }
            }
            return nullptr;
        }
    private:
        class IStore {
        public:
            virtual ~IStore() = default;
            virtual bool Is(const char* code) const noexcept = 0;
        };

        std::vector<std::unique_ptr<IStore>> m_stores;

        template<typename Task>
        class Store final :public IStore {
            using index_type = typename Task::index_type;
            using value_type = typename Task::type;
        public:
            template<typename... Args>
            void add(const index_type& idx, Args&&... args) {
                m_tasks[idx] = std::move(value_type{ std::forward<Args>(args)... });
            }

            void remove(const index_type& idx) {
                m_tasks.erase(idx);
            }

            const value_type* find(const index_type& idx) const noexcept {
                auto it = m_tasks.find(idx);
                if (it != m_tasks.end()) {
                    return std::addressof(it->second);
                }
                return nullptr;
            }

            bool Is(const char* code) const noexcept final {
                static const auto name = typeid(Store).name();
                return (std::strcmp(name, code) == 0);
            }
        private:
            std::unordered_map<index_type, value_type> m_tasks;
        };
    };

    template<typename I, typename R, typename... Args>
    struct TaskConcept {
        using index_type = I;
        using type = std::function<R(Args...)>;

        static R Run(const type* op, Args&&... args) {
            if (op) return (*op)(std::forward<Args>(args)...);
            return {};
        }
    };

    template<typename I, typename... Args>
    struct TaskConcept<I, void, Args...> {
        using index_type = I;
        using type = std::function<void(Args...)>;

        static void Run(const type* op, Args&&... args) {
            if (op) return (*op)(std::forward<Args>(args)...);
        }
    };
}
