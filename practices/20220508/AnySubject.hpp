#pragma once

#include "TypeRegistry.hpp"
#include <vector>
#include <unordered_map>
#include <memory>

namespace abc
{
    namespace v0
    {
        class TYPE_REGISTRY_API IObserver {
        public:
            virtual ~IObserver() = default;
            virtual void Update(const void* subject, const TypeCode& code) = 0;
        };

        template<typename Subject,typename T>
        class Observer final :public IObserver {
            T m_obj;
        public:
            explicit Observer(T&& obj) :m_obj(std::move(obj)) {};

            void Update(const void* subject, const TypeCode& code)  override {
                if (code == Of<Subject>()) {
                    m_obj.Update(*reinterpret_cast<const Subject*>(subject));
                }
            }
        };

        class TYPE_REGISTRY_API AnySubject {
        public:
            AnySubject() = default;
            template<typename T>
            void Notify(const T& subject) {
                NotifyImpl(std::addressof(subject), Of<T>());
            }

            template<typename Subject, typename T>
            std::shared_ptr<IObserver> RegisterObserver(T&& obj) {
                auto result = std::make_shared<Observer<Subject,T>>(std::move(obj));
                m_observers[Of<Subject>()].push_back(result);
                return result;
            }

            static AnySubject* Get();
        private:
            void NotifyImpl(const void* subject, const TypeCode& code);

            std::unordered_map<TypeCode, std::vector<std::shared_ptr<IObserver>>>  m_observers;
        };

    }

    namespace v1
    {
        class TYPE_REGISTRY_API IObserver {
        public:
            virtual ~IObserver() = default;
            virtual void Update(const void* subject, const TypeCode& code) = 0;
        };

        template<typename Subject, typename T>
        class Observer final :public IObserver {
            T m_obj;
        public:
            explicit Observer(T&& obj) :m_obj(std::move(obj)) {};

            void Update(const void* subject, const TypeCode& code)  override {
                //该检查可以没有
                if (code == Of<Subject>()) {
                    m_obj.Update(*reinterpret_cast<const Subject*>(subject));
                }
            }
        };

        class TYPE_REGISTRY_API AnySubject {
        public:
            AnySubject() = default;

            template<typename T>
            void Notify(const T& subject) const {
                static auto code = Of<T>();
                if (code >= m_observers.size()) {
                    return;
                }
                for (auto&& o : m_observers[code]) {
                    o->Update(std::addressof(subject), code);
                }
            }

            template<typename Subject, typename T>
            std::shared_ptr<IObserver> RegisterObserver(T&& obj) {
                static auto code = Of<Subject>();
                auto result = std::make_shared<Observer<Subject, T>>(std::move(obj));
                //保证观察者数量
                m_observers.resize(std::max(std::size_t(code+1), m_observers.size()));
                m_observers[code].push_back(result);
                return result;
            }

            static AnySubject* Get();
        private:
            std::vector<std::vector<std::shared_ptr<IObserver>>> m_observers;
        };
    }
}
