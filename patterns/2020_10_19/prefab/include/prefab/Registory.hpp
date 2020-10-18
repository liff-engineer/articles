#pragma once

#include <map>
#include "IRepository.hpp"
#include "IValue.hpp"

namespace prefab
{
    class Registory
    {
        std::map<HashedStringLiteral, std::unique_ptr<IRepository>> m_repos;
        std::map<HashedStringLiteral, std::unique_ptr<IValue>> m_values;
    public:
        Registory() = default;

        template<typename E>
        void registerRepo(std::unique_ptr<IRepository>&& repo) noexcept {
            m_repos[HashedTypeNameOf<E>()] = std::move(repo);
        }

        template<typename E>
        IRepository* repoOf() const noexcept {
            auto key = HashedTypeNameOf<E>();
            auto it = m_repos.find(key);
            if (it != m_repos.end()) {
                return it->second.get();
            }
            return nullptr;
        }

        template<typename T>
        void registerValue(T&& v) noexcept {
            auto impl = std::make_unique<ValueImpl<T>>(std::move(v));
            m_values[impl->typeCode()] = std::move(impl);
        }

        template<typename T>
        bool hasValue() const noexcept {
            return m_values.find(HashedTypeNameOf<T>()) != m_values.end();
        }

        template<typename T>
        const T& valueOf() const {
            auto impl = m_values.at(HashedTypeNameOf<T>());
            return static_cast<ValueImpl<T>*>(impl)->value();
        }

        template<typename T>
        T& valueOf() {
            auto impl = m_values.at(HashedTypeNameOf<T>());
            return static_cast<ValueImpl<T>*>(impl)->value();
        }
    };




}
