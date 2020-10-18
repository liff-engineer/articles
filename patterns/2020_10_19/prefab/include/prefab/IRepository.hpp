#pragma once

#include <array>
#include "IEntity.hpp"
#include "IValue.hpp"

namespace prefab
{
    class IRepository
    {
    public:
        struct TypeCodes {
            HashedStringLiteral* pointer = nullptr;
            std::size_t n = 0;

            TypeCodes() = default;

            template<std::size_t N>
            TypeCodes(std::array<HashedStringLiteral, N>& arg)
                :pointer(arg.data()), n(N) {};

            bool empty() const noexcept {
                return n != 0;
            }

            std::size_t size() const noexcept {
                return n;
            }

            bool has(HashedStringLiteral typeCode) const noexcept {
                for (std::size_t i = 0; i < n; i++) {
                    if (*(pointer + i) == typeCode) {
                        return true;
                    }
                }
                return false;
            }

            template<typename F>
            void visit(F& fn) const {
                for (std::size_t i = 0; i < n; i++) {
                    fn(*(pointer + i));
                }
            }
        };
    public:
        virtual HashedStringLiteral implTypeCode() const  noexcept = 0;
    protected:
        virtual std::unique_ptr<IEntity> createImpl(TypeCodes typeCodes, Value const& argument)  = 0;
        virtual std::size_t destoryImpl(TypeCodes typeCodes, Require const& req)  = 0;
        virtual std::size_t destoryImpl(TypeCodes typeCodes, std::unique_ptr<IEntity>&& entity) noexcept = 0;
        virtual std::size_t destoryImpl(TypeCodes typeCodes, std::unique_ptr<IEntitys>&& entitys) noexcept = 0;
        virtual std::unique_ptr<IEntity> findImpl(TypeCodes typeCodes, Require const& req) const  = 0;
        virtual std::unique_ptr<IEntitys> searchImpl(TypeCodes typeCodes, Require const& req) const  = 0;

    public:
        virtual ~IRepository() = default;

        template<typename... Ts>
        std::array<HashedStringLiteral, sizeof...(Ts)> typeCodesOf() const noexcept {
            return {
                HashedTypeNameOf<Ts>()...
            };
        }

        template<typename... Ts>
        Entity<Ts...> create(Value const& argument = {}) {
            auto typeCodes = typeCodesOf<Ts...>();
            auto impl = createImpl(typeCodes, argument);
            return Entity<Ts...>(std::move(impl));
        }

        template<typename... Ts>
        std::size_t  destory(Require const& req = {}) {
            auto typeCodes = typeCodesOf<Ts...>();
            return destoryImpl(typeCodes, req);
        }

        template<typename... Ts>
        std::size_t destory(Entity<Ts...>&& entity) noexcept {
            auto typeCodes = typeCodesOf<Ts...>();
            return destoryImpl(typeCodes, entity.m_impl);
        }

        template<typename... Ts>
        std::size_t destory(Entitys<Ts...>&& entitys) noexcept {
            auto typeCodes = typeCodesOf<Ts...>();
            return destoryImpl(typeCodes, entitys.m_impl);
        }

        template<typename... Ts>
        Entity<Ts...> find(Require const& req = {}) const {
            auto typeCodes = typeCodesOf<Ts...>();
            return Entity<Ts...>{std::move(findImpl(typeCodes, req))};
        }

        template<typename... Ts>
        Entitys<Ts...> search(Require const& req = {}) const {
            auto typeCodes = typeCodesOf<Ts...>();
            return Entitys<Ts...>{std::move(searchImpl(typeCodes, req))};
        }
    };
}
