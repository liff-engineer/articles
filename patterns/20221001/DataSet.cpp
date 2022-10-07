#include "DataSet.h"

namespace abc
{
    class JustObject{};

    class EntityRef {
    public:
        explicit EntityRef(Project* project, int id)
        {
            //找到Entity
            m_entity = project->entitys[id];
            m_obj = reinterpret_cast<JustObject*>(m_entity->pointer);
            m_description = reinterpret_cast<std::string*>(m_entity->pointers[0].second);
            m_number = reinterpret_cast<double*>(m_entity->pointers[1].second);
        }
    private:
        Entity* m_entity{};
        JustObject* m_obj{};
        std::string* m_description{};
        double* m_number{};
    };

    void test_project()
    {
        Project prj{};
        prj.datasets.add<Entity>();
        auto i = prj.datasets.add<JustObject>();
        auto i1 = prj.datasets.add<std::string>();
        auto i2 = prj.datasets.add<double>();

        //添加两个实体
        int e1{ -1 };
        int e2{ -1 };
        {
            auto entity = prj.datasets.find<Entity>()->insert(Entity{});
            entity->pointer = prj.append<JustObject>(JustObject{});
            entity->pointers.emplace_back(i1, prj.append<std::string>("Entity 1"));
            entity->pointers.emplace_back(i2, prj.append<double>(1.414));

            entity->id = prj.entitys.insert(entity);
            e1 = entity->id;
        }
        {
            auto entity = prj.datasets.find<Entity>()->insert(Entity{});
            entity->pointer = prj.append<JustObject>(JustObject{});
            entity->pointers.emplace_back(i1, prj.append<std::string>("Entity 2"));
            entity->pointers.emplace_back(i2, prj.append<double>(3.1415926));

            entity->id = prj.entitys.insert(entity);

            e2 = entity->id;
        }

        {
            auto& e = prj.entitys[e1];
            auto v1 = reinterpret_cast<JustObject*>(e->pointer);
            auto v2 = reinterpret_cast<std::string*>(e->pointers[0].second);
            auto v3 = reinterpret_cast<double*>(e->pointers[1].second);
            *v2 += "(liff.engineer@gmail.com)";
        }

        {
            auto& e = prj.entitys[e2];
            auto v1 = reinterpret_cast<JustObject*>(e->pointer);
            auto v2 = reinterpret_cast<std::string*>(e->pointers[0].second);
            auto v3 = reinterpret_cast<double*>(e->pointers[1].second);
            *v2 += "(liff.engineer@gmail.com)";
        }

        auto obj1 = EntityRef{ &prj,e1 };
        auto obj2 = EntityRef{ &prj,e2 };

    }
}
