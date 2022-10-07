#pragma once

#include <vector>
#include <memory>

namespace abcd
{
    //class IDataSet {
    //public:
    //    virtual ~IDataSet() = default;
    //    virtual const char* code() const noexcept = 0;
    //};

    //template<typename T>
    //class IValueSet :public IDataSet {
    //public:
    //    virtual T* add(unsigned dataset, unsigned id, T&& v) = 0;
    //    virtual void remove(unsigned dataset, unsigned id) = 0;
    //    virtual void remove(unsigned dataset) = 0;
    //    virtual void remove(const T* vp) = 0;
    //    virtual const T* find(unsigned dataset, unsigned id) const noexcept = 0;
    //    virtual T* find(unsigned dataset, unsigned id) noexcept = 0;
    //};


    //template<typename T>
    //class IEntitySet :public IDataSet {
    //public:
    //    virtual unsigned add() = 0;
    //    virtual void     remove(unsigned id) = 0;
    //    virtual bool     contains(unsigned id) const noexcept = 0;

    //    virtual void     assign(unsigned id, T* vp) = 0;
    //    virtual const T* find(unsigned id) const noexcept = 0;
    //    virtual T* find(unsigned id) noexcept = 0;
    //};

    class DataSetCluster {
        template<typename T>
        struct Value {
            unsigned dataset;
            unsigned id;
            std::unique_ptr<T> v;
        };

        template<typename T>
        struct Entity {
            unsigned id;
            std::unique_ptr<T> v;//TODO FIXME id合并到T的表达中去
        };
    public:
        //Entity/Value:Create/Destory/Query/Update
    private:
        //std::vector<std::unique_ptr<IDataSet>> m_datasets;
    };
}
