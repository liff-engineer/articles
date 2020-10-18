#pragma once
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <stdexcept>
#include "prefab/IRepository.hpp"

namespace prefab
{
    //注意该内存实现只是为了验证整个解决方案
    //鉴于各种场景使用的底层ID类型均不相同,
    //而且create/destory/find/search针对特定entity会有不同参数处理
    //在使用时,可以参考本实现实现相应场景
    inline namespace verify
    {
        //基于内存的存储库Component采用map
        template<typename T>
        class Component :public IComponent<T>
        {
            long long m_key;
            std::map<long long, T>* m_repo = nullptr;
        public:
            explicit Component(std::map<long long, T>* repo, long long key)
                :m_repo{ repo }, m_key(key)
            {};

            HashedStringLiteral implTypeCode() const noexcept override {
                return HashedTypeNameOf<Component<T>>();
            }

            bool exist() const noexcept override {
                return m_repo->find(m_key) != m_repo->end();
            }
            T    view() const  override {
                return m_repo->at(m_key);
            }
            bool remove() noexcept {
                if (!exist()) return  false;
                m_repo->erase(m_key);
                return true;
            }
            bool assign(T const& v) noexcept {
                if (exist()) return false;
                (*m_repo)[m_key] = v;
                return true;
            }
            T    replace(T const& v) noexcept {
                return std::exchange(m_repo->at(m_key), v);
            }
        };

        //虚拟数据组件,提供以下能力
        //1.组合多个数据组件的部分内容形成新数据组件
        //2.功能性数据组件,用在创建/销毁/查询等接口起到辅助作用
        //!!注意需要派生自IComponent<T>,通过特化实现!!
        //目前的模板定义只是演示
        class Repository;
        template<typename T>
        class VirtualComponent :public IComponent<T>
        {
            long long m_key = -1;
            Repository* m_impl = nullptr;
        public:
            explicit VirtualComponent(Repository* impl, long long key)
                :m_impl(impl), m_key(key) {};
            
            //如果对创建有影响是体现到IComponent<T>的写入性接口
            static void write(IComponentBase* handler, Value const& argument) noexcept {
                auto typeCode = HashedTypeNameOf<T>();
                if (handler->typeCode() != argument.typeCode()) {
                    return;
                }
                static_cast<IComponent<T>*>(handler)->assign(argument.as<T>());
            }
            //如果是过滤处理则需要特别实现
        };
        
        //具体的筛选等实现
        template<typename T>
        struct VirtualComponentHandler 
        {
            //针对实体标识符为id的目标,是否满足req
            static bool accept(Repository* repo, long long id, Require const& req) noexcept;
        };

        class EntityImpl :public IEntity
        {
            long long m_key = -1;
            std::vector<std::unique_ptr<IComponentBase>> m_components;
        public:
            EntityImpl() = default;
            explicit EntityImpl(long long key,std::vector<std::unique_ptr<IComponentBase>>&& components)
                :IEntity{},m_key(key),m_components(std::move(components))
            {};

            long long key() const noexcept {
                return m_key;
            }

            HashedStringLiteral implTypeCode() const noexcept  override {
                return HashedTypeNameOf<EntityImpl>();
            }

            IComponentBase* component(HashedStringLiteral typeCode) noexcept override {
                for (auto& o : m_components) {
                    if (o->typeCode() == typeCode) {
                        return o.get();
                    }
                }
                return nullptr;
            }

            explicit operator bool() const noexcept {
                return (m_key != -1) && (!m_components.empty());
            }
        };

        class EntitysImpl :public IEntitys
        {
            std::vector<EntityImpl> m_entitys;
        public:
            explicit EntitysImpl(std::vector<EntityImpl>&& items)
                :m_entitys(std::move(items)) {};

            HashedStringLiteral implTypeCode() const noexcept override {
                return HashedTypeNameOf<EntitysImpl>();
            }

            std::vector<IEntity*> entitys() noexcept override {
                std::vector<IEntity*> results;
                results.reserve(m_entitys.size());
                for (auto& e : m_entitys) {
                    results.emplace_back(&e);
                }
                return results;
            }
        };

        class Repository:public IRepository
        {
            template<typename T>
            friend class VirtualComponent;
            template<typename T>
            friend class VirtualComponentHandler;

            long long m_nextId = 0;
            std::set<long long>  m_idRepo;
            std::map<HashedStringLiteral, std::unique_ptr<IValue>> m_voRepos;
            std::map<HashedStringLiteral, std::function<std::unique_ptr<IComponentBase>(long long)>> m_componentCreators;
            std::map<HashedStringLiteral, std::function<bool(Repository*, long long, Require const&)>> m_componentHandlers;
            std::map<HashedStringLiteral, std::function<void(IComponentBase*, Value const&)>> m_componentWriters;
        public:
            Repository() = default;

            template<typename T>
            void registerComponentRepo() noexcept {
                auto typeCode = HashedTypeNameOf<T>();
                {
                    auto repo = std::map<long long, T>();
                    auto impl = std::make_unique<ValueImpl<std::map<long long, T>>>(std::move(repo));
                    m_voRepos[typeCode] = std::move(impl);
                }
                auto base = m_voRepos.at(typeCode).get();
                auto impl = static_cast<ValueImpl<std::map<long long, T>>*>(base);
                auto repoPointer = &(impl->value());
                m_componentCreators[typeCode] = [=](long long key)->std::unique_ptr<IComponentBase> {
                    return std::make_unique<Component<T>>(repoPointer, key);
                };
            }

            template<typename T>
            void registerVirualComponent() noexcept {
                auto typeCode = HashedTypeNameOf<T>();
                m_componentCreators[typeCode] = [=](long long key)->std::unique_ptr<IComponentBase> {
                    return std::make_unique<VirtualComponent<T>>(this, key);
                };
                m_componentWriters[typeCode] = VirtualComponent<T>::write;
                m_componentHandlers[typeCode] = VirtualComponentHandler<T>::accept;
            }

            HashedStringLiteral implTypeCode() const  noexcept {
                return HashedTypeNameOf<Repository>();
            }
        protected:
            std::unique_ptr<IEntity> createImpl(TypeCodes typeCodes, Value const& argument)  override {
                if (argument) {
                    if (!typeCodes.has(argument.typeCode())) {
                        throw std::invalid_argument("demo implement cann't process create argument");
                    }
                }
                m_idRepo.insert(m_nextId);

                std::vector<std::unique_ptr<IComponentBase>> components;
                components.reserve(typeCodes.size());
                typeCodes.visit([&](HashedStringLiteral typeCode) {
                    components.emplace_back(m_componentCreators.at(typeCode)(m_nextId));
                    });
                m_nextId++;
                auto result = std::make_unique<EntityImpl>(m_nextId - 1, std::move(components));
                //如果有参数则写入
                if (argument) {
                    auto typeCode = argument.typeCode();
                    auto h = result->component(typeCode);
                    m_componentWriters.at(typeCode)(h, argument);
                }
                return result;
            }

            std::size_t destoryImpl(TypeCodes typeCodes, Require const& req)  override {
                auto reqTypeCode = req.typeCode();
                if (reqTypeCode == HashedTypeNameOf<long long>()) {
                    //目前只演示根据ID销毁
                    auto key = req.as<long long>();
                    auto remark = req.remark(""_hashed);
                    if (remark == "=="_hashed) {
                        //删除等价于key的
                        return m_idRepo.erase(key);
                    }
                    else if (remark == "!="_hashed) {
                        auto n = m_idRepo.size();
                        auto b = m_idRepo.erase(key);
                        m_idRepo.clear();
                        if (b > 0) {
                            m_idRepo.insert(key);
                            return n - 1;
                        }
                        return n;
                    }
                }
                else if(!reqTypeCode)//默认不传条件就是清空
                {
                    auto n = m_idRepo.size();
                    m_idRepo.clear();
                    return n;
                }
                else if (typeCodes.has(reqTypeCode)) {
                    std::size_t n = 0;
                    auto fn = m_componentHandlers.at(reqTypeCode);
                    for (auto key : m_idRepo) {
                        if (fn(this, key, req)) {
                            n += m_idRepo.erase(key);
                        }
                    }
                    return n;
                }
                //其它场景下处理不演示了,无法识别的命令要抛出异常
                throw std::invalid_argument("demo implement can't process destory require");
                return 0;
            }
            std::size_t destoryImpl(TypeCodes typeCodes, std::unique_ptr<IEntity>&& entity) noexcept override {
                if (entity && entity->implTypeCode() == HashedTypeNameOf<EntityImpl>()) {
                    auto key = static_cast<EntityImpl*>(entity.get())->key();
                    return m_idRepo.erase(key);
                }
                return 0;
            }

            std::size_t destoryImpl(TypeCodes typeCodes, std::unique_ptr<IEntitys>&& entitys) noexcept override {
                if (entitys && entitys->implTypeCode() == HashedTypeNameOf<EntitysImpl>()) {
                    std::size_t n = 0;
                    for (auto e : entitys->entitys()) {
                        auto impl = static_cast<EntityImpl*>(e);
                        if (impl) {
                            n += m_idRepo.erase(impl->key());
                        }
                    }
                    return n;
                }
                return 0;
            }

            std::unique_ptr<IEntity> findImpl(TypeCodes typeCodes, Require const& req) const override {
                if (m_idRepo.empty()) return nullptr;
                auto reqTypeCode = req.typeCode();
                if (reqTypeCode == HashedTypeNameOf<long long>()) {
                    auto key = req.as<long long>();
                    auto it = m_idRepo.find(key);
                    if (it != m_idRepo.end()) {
                        std::vector<std::unique_ptr<IComponentBase>> components;
                        components.reserve(typeCodes.size());
                        typeCodes.visit([&](HashedStringLiteral typeCode) {
                            components.emplace_back(m_componentCreators.at(typeCode)(key));
                            });

                        return std::make_unique<EntityImpl>(key, std::move(components));
                    }
                    else
                    {
                        return nullptr;
                    }
                }
                else if (typeCodes.has(reqTypeCode)) {
                    auto fn = m_componentHandlers.at(reqTypeCode);
                    for (auto key : m_idRepo) {
                        if (!fn(const_cast<Repository*>(this), key, req))
                            continue;

                        std::vector<std::unique_ptr<IComponentBase>> components;
                        components.reserve(typeCodes.size());
                        typeCodes.visit([&](HashedStringLiteral typeCode) {
                            components.emplace_back(m_componentCreators.at(typeCode)(key));
                            });

                        return std::make_unique<EntityImpl>(key, std::move(components));
                    }
                    return nullptr;
                }
                throw std::invalid_argument("demo implement can't process find require");
                return nullptr;
            }
            std::unique_ptr<IEntitys> searchImpl(TypeCodes typeCodes, Require const& req) const override {
                auto reqTypeCode = req.typeCode();
                if (!reqTypeCode) {
                    std::vector<EntityImpl> entitys;
                    entitys.reserve(m_idRepo.size());
                    for (auto key : m_idRepo)
                    {
                        std::vector<std::unique_ptr<IComponentBase>> components;
                        components.reserve(typeCodes.size());
                        typeCodes.visit([&, key](HashedStringLiteral typeCode) {
                            components.emplace_back(m_componentCreators.at(typeCode)(key));
                            });
                        entitys.emplace_back(EntityImpl(key, std::move(components)));
                    }
                    return std::make_unique<EntitysImpl>(std::move(entitys));
                }
                else if (typeCodes.has(reqTypeCode)) {
                    std::vector<EntityImpl> entitys;
                    entitys.reserve(m_idRepo.size());

                    auto fn = m_componentHandlers.at(reqTypeCode);
                    for (auto key : m_idRepo) {
                        if (!fn(const_cast<Repository*>(this), key, req))
                            continue;

                        std::vector<std::unique_ptr<IComponentBase>> components;
                        components.reserve(typeCodes.size());
                        typeCodes.visit([&, key](HashedStringLiteral typeCode) {
                            components.emplace_back(m_componentCreators.at(typeCode)(key));
                            });
                        entitys.emplace_back(EntityImpl(key, std::move(components)));
                    }
                    return std::make_unique<EntitysImpl>(std::move(entitys));
                }
                throw std::invalid_argument("demo implement can't process search require");
                return nullptr;
            }
        };
    }
}
