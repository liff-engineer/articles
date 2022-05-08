#include "AnySubject.hpp"

namespace abc
{
    namespace v0
    {
        AnySubject* AnySubject::Get()
        {
            static AnySubject obj{};
            return std::addressof(obj);
        }
        void AnySubject::NotifyImpl(const void* subject, const TypeCode& code)
        {
            auto it = m_observers.find(code);
            if (it != m_observers.end()) {
                for (auto&& o : it->second) {
                    o->Update(subject,code);
                }
            }
        }
    }
    namespace v1
    {
        AnySubject* AnySubject::Get()
        {
            static AnySubject obj{};
            return std::addressof(obj);
        }
    }
}
