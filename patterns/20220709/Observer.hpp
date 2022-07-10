#pragma once
#include <vector>
#include <algorithm>
#include <memory>
//#include <type_traits>

/// @brief 常规的观察者实现方式
namespace v0
{
    class IObserver
    {
    public:
        virtual ~IObserver() = default;
        IObserver &operator=(IObserver &&other) noexcept = delete;
        virtual void update(double event) = 0;
    };

    class Subject
    {
    public:
        void notify(double event)
        {
            for (auto &observer : m_observers)
            {
                if (observer)
                {
                    observer->update(event);
                }
            }
        }

        void subscribe(IObserver *observer)
        {
            auto it = std::find(m_observers.begin(), m_observers.end(), observer);
            if (it == m_observers.end())
            {
                m_observers.emplace_back(observer);
            }
        }

        void unsubscribe(IObserver *observer)
        {
            auto it = std::find(m_observers.begin(), m_observers.end(), observer);
            if (it != m_observers.end())
            {
                *it = nullptr;
            }
        }

    private:
        std::vector<IObserver *> m_observers;
    };

    struct Event
    {
        std::string payload;
    };

    class IEventObserver
    {
    public:
        virtual ~IEventObserver() = default;
        IEventObserver &operator=(IEventObserver &&other) noexcept = delete;
        virtual void update(Event const &event) = 0;
    };

    class EventSubject
    {
    public:
        void notify(const Event &event)
        {
            for (auto &observer : m_observers)
            {
                if (observer)
                {
                    observer->update(event);
                }
            }
        }

        void subscribe(IEventObserver *observer)
        {
            auto it = std::find(m_observers.begin(), m_observers.end(), observer);
            if (it == m_observers.end())
            {
                m_observers.emplace_back(observer);
            }
        }

        void unsubscribe(IEventObserver *observer)
        {
            auto it = std::find(m_observers.begin(), m_observers.end(), observer);
            if (it != m_observers.end())
            {
                *it = nullptr;
            }
        }

    private:
        std::vector<IEventObserver *> m_observers;
    };
}

/// @brief 使用模板减少重复代码
namespace v1
{
    template <typename T>
    class IObserver
    {
    public:
        virtual ~IObserver() = default;
        IObserver &operator=(IObserver &&other) noexcept = delete;
        virtual void update(const T &event) = 0;
    };

    template <typename T>
    class Subject
    {
    public:
        void notify(const T &event)
        {
            for (auto &observer : m_observers)
            {
                if (observer)
                {
                    observer->update(event);
                }
            }
        }

        void subscribe(IObserver<T> *observer)
        {
            auto it = std::find(m_observers.begin(), m_observers.end(), observer);
            if (it == m_observers.end())
            {
                m_observers.emplace_back(observer);
            }
        }

        void unsubscribe(IObserver<T> *observer)
        {
            auto it = std::find(m_observers.begin(), m_observers.end(), observer);
            if (it != m_observers.end())
            {
                *it = nullptr;
            }
        }

    private:
        std::vector<IObserver<T> *> m_observers;
    };
}

/// @brief 提供Publisher以支持多种Subject(不支持多种Observer实现)
namespace v2
{
    using TypeCode = std::string;
    class IObserver
    {
    public:
        virtual ~IObserver() = default;
        IObserver &operator=(IObserver &&other) noexcept = delete;
        virtual TypeCode code() const noexcept = 0;
    };

    template <typename T>
    class Observer : public IObserver
    {
    public:
        virtual void update(const T &event) = 0;
        TypeCode code() const noexcept override final
        {
            return typeid(T).name();
        }
    };

    class Publisher
    {
    public:
        template <typename T>
        void notify(const T &event)
        {
            auto code = typeid(T).name();
            for (auto &observer : m_observers)
            {
                if (observer && observer->code() == code)
                {
                    if (auto vp = dynamic_cast<Observer<T> *>(observer))
                    {
                        vp->update(event);
                    }
                }
            }
        }

        template <typename T>
        void subscribe(Observer<T> *observer)
        {
            auto it = std::find(m_observers.begin(), m_observers.end(), observer);
            if (it == m_observers.end())
            {
                m_observers.emplace_back(observer);
            }
        }

        void unsubscribe(IObserver *observer)
        {
            auto it = std::find(m_observers.begin(), m_observers.end(), observer);
            if (it != m_observers.end())
            {
                *it = nullptr;
            }
        }

    private:
        std::vector<IObserver *> m_observers;
    };
}

/// @brief 提供免派生的观察者,以支持多Observer实现
namespace v3
{
    using TypeCode = std::string;
    class IObserver
    {
    public:
        virtual ~IObserver() = default;
        IObserver &operator=(IObserver &&other) noexcept = delete;
        virtual TypeCode code() const noexcept = 0;
        virtual const void *address() const noexcept = 0;
    };

    template <typename E>
    class Observer : public IObserver
    {
    public:
        virtual void update(const E &event) = 0;
        TypeCode code() const noexcept override final
        {
            return typeid(E).name();
        }
    };

    template <typename T, typename E>
    class ObserverImpl final : public Observer<E>
    {
        T *m_obj;

    public:
        explicit ObserverImpl(T *obj) : m_obj(obj){};

        void update(const E &event) override
        {
            return m_obj->update(event);
        }

        const void *address() const noexcept override
        {
            return m_obj;
        }
    };

    class Publisher
    {
    public:
        template <typename E>
        void notify(const E &event)
        {
            auto code = typeid(E).name();
            for (auto &observer : m_observers)
            {
                if (observer && observer->code() == code)
                {
                    if (auto vp = dynamic_cast<Observer<E> *>(observer.get()))
                    {
                        vp->update(event);
                    }
                }
            }
        }

        template <typename E, typename T>
        void subscribe(T *obj)
        {
            auto code = typeid(E).name();
            auto it = std::find_if(m_observers.begin(), m_observers.end(),
                                   [&](auto &&observer) -> bool
                                   {
                                       return (observer &&
                                               (observer->code() == code) &&
                                               (observer->address() == obj));
                                   });
            if (it == m_observers.end())
            {
                m_observers.emplace_back(std::make_unique<ObserverImpl<T, E>>(obj));
            }
        }

        template <typename T>
        void unsubscribe(T *obj)
        {
            auto it = std::find_if(m_observers.begin(), m_observers.end(),
                                   [=](auto &&observer) -> bool
                                   {
                                       return (observer && observer->address() == obj);
                                   });
            if (it != m_observers.end())
            {
                *it = nullptr;
            }
        }

        template <typename E, typename T>
        void unsubscribe(T *obj)
        {
            auto code = typeid(E).name();
            auto it = std::find_if(m_observers.begin(), m_observers.end(),
                                   [&](auto &&observer) -> bool
                                   {
                                       return (observer &&
                                               (observer->code() == code) &&
                                               (observer->address() == obj));
                                   });
            if (it != m_observers.end())
            {
                *it = nullptr;
            }
        }

    private:
        std::vector<std::unique_ptr<IObserver>> m_observers;
    };
}

/// @brief 考虑到IObserver已经没用公开的必要了,将其移入Publisher实现
namespace v4
{
    class Publisher
    {
    public:
        template <typename E>
        void notify(const E &event)
        {
            auto code = typeid(E).name();
            for (auto &observer : m_observers)
            {
                if (observer && observer->match(code))
                {
                    if (auto vp = dynamic_cast<Observer<E> *>(observer.get()))
                    {
                        vp->update(event);
                    }
                }
            }
        }

        template <typename E, typename T>
        void subscribe(T *obj)
        {
            auto code = typeid(E).name();
            for (auto &&observer : m_observers)
            {
                if (observer && observer->match(code, obj))
                {
                    return;
                }
            }
            m_observers.emplace_back(std::make_unique<ObserverImpl<T, E>>(obj));
        }

        template <typename T>
        void unsubscribe(T *obj)
        {
            for (auto &&observer : m_observers)
            {
                if (observer && observer->match(obj))
                {
                    observer.reset();
                }
            }
        }

        template <typename E, typename T>
        void unsubscribe(T *obj)
        {
            auto code = typeid(E).name();
            for (auto &&observer : m_observers)
            {
                if (observer && observer->match(code, obj))
                {
                    observer.reset();
                }
            }
        }

    private:
        using TypeCode = std::string;
        class IObserver
        {
        public:
            virtual ~IObserver() = default;
            IObserver &operator=(IObserver &&other) noexcept = delete;
            virtual bool match(const TypeCode &code) const noexcept = 0;
            virtual bool match(void *obj) const noexcept = 0;
            inline bool match(const TypeCode &code, void *obj) const noexcept
            {
                return match(code) && match(obj);
            }
        };

        template <typename E>
        class Observer : public IObserver
        {
        public:
            virtual void update(const E &event) = 0;

            bool match(const TypeCode &code) const noexcept override final
            {
                return typeid(E).name() == code;
            }
        };

        template <typename T, typename E>
        class ObserverImpl final : public Observer<E>
        {
            T *m_obj;

        public:
            explicit ObserverImpl(T *obj) : m_obj(obj){};

            void update(const E &event) override
            {
                return m_obj->update(event);
            }

            bool match(void *obj) const noexcept override
            {
                return obj == m_obj;
            }
        };

    private:
        std::vector<std::unique_ptr<IObserver>> m_observers;
    };
}

/// @brief 为避免开发者手动管理订阅关系,以RAII提供关系管理存根
namespace v5
{
    class Publisher
    {
    public:
        class Stub
        {
        public:
            Stub() = default;
            Stub(const Stub &other) = delete;
            Stub &operator=(const Stub &other) = delete;
            Stub(Stub &&other) noexcept
                : m_owner(other.m_owner), m_index(other.m_index)
            {
                other.m_owner = nullptr;
                other.m_index = 0;
            }

            Stub &operator=(Stub &&other) noexcept
            {
                if (this != std::addressof(other))
                {
                    unsubscribe();
                    m_owner = other.m_owner;
                    m_index = other.m_index;
                    other.m_owner = nullptr;
                }
                return *this;
            }

            ~Stub() noexcept
            {
                unsubscribe();
            }

        private:
            friend class Publisher;
            explicit Stub(Publisher *owner, std::size_t index)
                : m_owner(owner), m_index(index)
            {
            }

            void unsubscribe() noexcept
            {
                if (m_owner && m_index < m_owner->m_observers.size())
                {
                    m_owner->m_observers[m_index].reset();
                }
            }

            Publisher *m_owner{};
            std::size_t m_index{};
        };

    public:
        template <typename E>
        void notify(const E &event)
        {
            auto code = typeid(E).name();
            for (auto &observer : m_observers)
            {
                if (observer && observer->match(code))
                {
                    if (auto vp = dynamic_cast<Observer<E> *>(observer.get()))
                    {
                        vp->update(event);
                    }
                }
            }
        }

        template <typename E, typename T>
        Stub subscribe(T *obj)
        {
            auto code = typeid(E).name();
            for (auto &&observer : m_observers)
            {
                if (observer && observer->match(code, obj))
                {
                    return Stub{};
                }
            }
            m_observers.emplace_back(std::make_unique<ObserverImpl<T, E>>(obj));
            return Stub(this, m_observers.size() - 1);
        }

    private:
        using TypeCode = std::string;
        class IObserver
        {
        public:
            virtual ~IObserver() = default;
            IObserver &operator=(IObserver &&other) noexcept = delete;
            virtual bool match(const TypeCode &code) const noexcept = 0;
            virtual bool match(void *obj) const noexcept = 0;
            inline bool match(const TypeCode &code, void *obj) const noexcept
            {
                return match(code) && match(obj);
            }
        };

        template <typename E>
        class Observer : public IObserver
        {
        public:
            virtual void update(const E &event) = 0;

            bool match(const TypeCode &code) const noexcept override final
            {
                return typeid(E).name() == code;
            }
        };

        template <typename T, typename E>
        class ObserverImpl final : public Observer<E>
        {
            T *m_obj;

        public:
            explicit ObserverImpl(T *obj) : m_obj(obj){};

            void update(const E &event) override
            {
                return m_obj->update(event);
            }

            bool match(void *obj) const noexcept override
            {
                return obj == m_obj;
            }
        };

    private:
        std::vector<std::unique_ptr<IObserver>> m_observers;
    };
}

/// @brief 可以提供订阅函数
namespace v6
{
    class Publisher
    {
    public:
        template <typename E>
        void notify(const E &event)
        {
            auto code = typeid(E).name();
            for (auto &observer : m_observers)
            {
                if (observer && observer->match(code))
                {
                    if (auto vp = dynamic_cast<Observer<E> *>(observer.get()))
                    {
                        vp->update(event);
                    }
                }
            }
        }

        template <typename E, typename T>
        std::size_t subscribe(T *obj)
        {
            auto code = typeid(E).name();
            for (std::size_t i = 0; i < m_observers.size(); i++)
            {
                auto &&observer = m_observers[i];
                if (observer && observer->match(code, obj))
                {
                    return i;
                }
            }
            m_observers.emplace_back(std::make_unique<ObserverImpl<T, E>>(obj));
            return static_cast<int>(m_observers.size() - 1);
        }

        template <typename E, typename Fn>
        std::size_t subscribe(Fn &&fn)
        {
            using T = std::remove_cv_t<std::remove_reference_t<Fn>>;
            m_observers.emplace_back(std::make_unique<CallableObserver<T, E>>(std::forward<Fn>(fn)));
            return static_cast<int>(m_observers.size() - 1);
        }

        void unsubscribe(std::size_t index)
        {
            if (index < m_observers.size())
            {
                m_observers[index].reset();
            }
        }

    private:
        using TypeCode = std::string;
        class IObserver
        {
        public:
            virtual ~IObserver() = default;
            IObserver &operator=(IObserver &&other) noexcept = delete;
            virtual bool match(const TypeCode &code) const noexcept = 0;
            virtual bool match(void *obj) const noexcept = 0;
            inline bool match(const TypeCode &code, void *obj) const noexcept
            {
                return match(code) && match(obj);
            }
        };

        template <typename E>
        class Observer : public IObserver
        {
        public:
            virtual void update(const E &event) = 0;

            bool match(const TypeCode &code) const noexcept override final
            {
                return typeid(E).name() == code;
            }
        };

        template <typename T, typename E>
        class ObserverImpl final : public Observer<E>
        {
            T *m_obj;

        public:
            explicit ObserverImpl(T *obj) : m_obj(obj){};

            void update(const E &event) override
            {
                return m_obj->update(event);
            }

            bool match(void *obj) const noexcept override
            {
                return obj == m_obj;
            }
        };

        template <typename Fn, typename E>
        class CallableObserver final : public Observer<E>
        {
            Fn m_op;

        public:
            explicit CallableObserver(Fn &&fn) : m_op(std::move(fn)){};

            void update(const E &event) override
            {
                return m_op(event);
            }

            bool match(void *obj) const noexcept override
            {
                return false;
            }
        };

    private:
        std::vector<std::unique_ptr<IObserver>> m_observers;
    };
}
