#pragma once
#include <vector>
#include <tuple>
#include <memory>
#include "IComponent.hpp"

namespace prefab
{
    //用来操作tuple的foreach算法
    template <typename Tuple, typename F, std::size_t ...Indices>
    void for_each_impl(Tuple&& tuple, F&& f, std::index_sequence<Indices...>) {
        using swallow = int[];
        (void)swallow {
            1,
                (f(std::get<Indices>(std::forward<Tuple>(tuple))), void(), int{})...
        };
    }

    template <typename Tuple, typename F>
    void for_each(Tuple&& tuple, F&& f) {
        constexpr std::size_t N = std::tuple_size<std::remove_reference_t<Tuple>>::value;
        for_each_impl(std::forward<Tuple>(tuple), std::forward<F>(f),
            std::make_index_sequence<N>{});
    }

    //实体基类
    class IEntity
    {
    public:
        virtual ~IEntity() = default;

        //实现类型Code
        virtual HashedStringLiteral implTypeCode() const noexcept = 0;

        //根据类型获取数据组件
        virtual IComponentBase* component(HashedStringLiteral typeCode) noexcept = 0;
    };

    //实体集合基类
    class IEntitys
    {
    public:
        virtual ~IEntitys() = default;

        //实现类型Code
        virtual HashedStringLiteral implTypeCode() const noexcept = 0;

        //包含的实体(实体生命周期由集合类管理)
        virtual std::vector<IEntity*> entitys() noexcept = 0;
    };

    //引用语义的实体类,不管理实体生命周期
    template<typename... Ts>
    class EntityView {
        std::tuple<IComponent<Ts>* ...> m_components;
    private:
        //从实体中获取并设置组件接口
        template<typename T>
        void setComponentOf(IEntity* entity, IComponent<T>*& component)
        {
            auto typeCode = HashedTypeNameOf<T>();
            auto componentBase = entity->component(typeCode);
            if (componentBase == nullptr || componentBase->typeCode() != typeCode) {
                component = nullptr;
                return;
            }
            component = static_cast<IComponent<T>*>(componentBase);
        }
    public:
        EntityView() = default;
        explicit EntityView(IEntity* entity)
        {
            if (!entity) return;
            for_each(m_components, [&](auto& component) {
                this->setComponentOf(entity, component);
                });
        }

        //判断是否所有组件都存在且有效
        explicit operator bool() const noexcept {
            bool result = true;
            for_each(m_components, [&](auto component) {
                result &= (component != nullptr);
                });
            return result;
        }

        template<typename T>
        decltype(auto) component() const noexcept {
            return std::get<IComponent<T>*>(m_components);
        }

        template<typename T>
        decltype(auto) component() noexcept {
            return std::get<IComponent<T>*>(m_components);
        }

        template<typename T>
        decltype(auto) exist() const noexcept {
            return component<T>()->exist();
        }

        template<typename T>
        decltype(auto) view() const {
            return component<T>()->view();
        }

        template<typename T>
        decltype(auto) remove() noexcept {
            return component<T>()->remove();
        }

        template<typename T>
        decltype(auto) assign(T const& v) noexcept {
            return component<T>()->assign(v);
        }

        template<typename T>
        decltype(auto) replace(T const& v) noexcept {
            return component<T>()->replace(v);
        }
    };


    class IRepository;
    //值语义的实体类,使用智能指针管理实体的生命周期
    template<typename... Ts>
    class Entity :public EntityView<Ts...>
    {
        friend class IRepository;
        std::unique_ptr<IEntity> m_impl;
    public:
        Entity() = default;
        explicit Entity(std::unique_ptr<IEntity>&& entity)
            :EntityView(entity.get()), m_impl(std::move(entity))
        {};
    };

    //实体容器
    template<typename... Ts>
    class Entitys
    {
        friend class IRepository;
        using entity_t = EntityView<Ts...>;
        std::vector<entity_t> m_entitys;
        std::unique_ptr<IEntitys>  m_impl;
    public:
        Entitys() = default;
        explicit Entitys(std::unique_ptr<IEntitys>&& entitys)
            :m_impl(std::move(entitys))
        {
            if (m_impl == nullptr) return;
            auto items = m_impl->entitys();
            m_entitys.reserve(items.size());
            for (auto e : items) {
                m_entitys.emplace_back(entity_t{ e });
            }
        }

        decltype(auto) empty() const noexcept {
            return m_entitys.empty();
        }

        decltype(auto) size() const noexcept {
            return m_entitys.size();
        }

        decltype(auto) at(std::size_t i) const {
            return m_entitys.at(i);
        }

        decltype(auto) operator[](std::size_t i) const noexcept {
            return m_entitys[i];
        }

        decltype(auto) begin() const noexcept {
            return m_entitys.begin();
        }

        decltype(auto) end() const noexcept {
            return m_entitys.end();
        }

        decltype(auto) begin()  noexcept {
            return m_entitys.begin();
        }

        decltype(auto) end()  noexcept {
            return m_entitys.end();
        }

        explicit operator bool() const noexcept {
            return (m_impl != nullptr);
        }
    };
}
