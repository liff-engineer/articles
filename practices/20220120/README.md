# 存储库模式与迭代器

存储库模式`Repository Pattern`是用来将业务模型与持久化实现进行隔离的一种实现模式,譬如下图:

![存储库、聚合、数据库表之间的关系](https://docs.microsoft.com/en-us/dotnet/architecture/microservices/microservice-ddd-cqrs-patterns/media/infrastructure-persistence-layer-design/repository-aggregate-database-table-relationships.png)

在领域驱动设计中,这种模式应用比较广泛. 从逻辑上讲,它是一个容器类,在C++的`STL`中,有丰富的算法,只需要容器类提供对应的迭代器即可重复利用. 这里将展示一下存储库怎么和迭代器融合,从而为`C++`开发者提供更自然、更丰富的使用体验.

## 存储库模式的核心抽象

存储库核心目标是提供一种抽象机制来操作持久化数据,以数据库为例,每一条数据由主键牵引,从而构成一个对象.所以它的`C#`定义类似如下:

```C#
public interface IRepository<T> where T : EntityBase
{
    T GetById(int id);
    IEnumerable<T> List();
    IEnumerable<T> List(Expression<Func<T, bool>> predicate);
    void Add(T entity);
    void Delete(T entity);
    void Edit(T entity);
}
public abstract class EntityBase
{
   public int Id { get; protected set; }
}
```

以`T`为要操作的对象类型,包含以下操作:

- 添加
- 删除
- 编辑
- 查找
- 遍历

对应的,`c++`开发者可能会将存储库定义为如下形式:

```C++
class Object{
    int Id() const noexcept;
};

class IObjectRepository{
public:
    virtual ~IObjectRepository()=default;
    
    //创建与销毁
    virtual int     Create() = 0;
	virtual void    Destory(int id) = 0;
    
    //查询
    virtual bool    Exist(int id) = 0;
	virtual	std::optional<Object> FindById(int id) const = 0;
    virtual std::vector<Object> All() const = 0;
    
    //更新
    virtual bool    Update(Object o) = 0;
};
```

如果从逻辑上来讲,存储库是个键值对容器,这样的接口设计,不能说有多大问题,只是并不复合`C++`语言里容器的常规操作方式,而且还有一定的性能隐患.

## 类似容器的存储库操作接口

可以基于`NVI`惯用法,将存储库接口调整成如下形式:

```C++
class IObjectRepository{
public:
    virtual ~IObjectRepository()=default;
    
protected:
    virtual int     createImpl() = 0;
    virtual void    eraseImpl(int id) = 0;
    
    virtual bool    existImpl(int id) const noexcept = 0;
    virtual int     sizeImpl() const noexcept = 0;
    virtual std::pair<int,int> idRangesImpl() const noexcept = 0;
    //查找到则从内部申请Object来管理
    virtual const Object* findImpl(int id) const noexcept = 0;
    virtual Object* findImpl(int id) noexcept = 0;    
    //使用该接口删除内部缓存的内存Object
    virtual void    destoryInnerCache(const Object* obj) noexcept = 0;
    
    virtual std::pair<int,int> idRangesImpl() const noexcept = 0;
};
```

然后,模拟`map\unordered_map`的接口为其提供基于`protected`接口的类容器实现:

```C++
class IObjectRepository{
public:
    std::size_t size() const noexcept;
    bool empty() const noexcept;
  	bool contains(int id);
    
    void erase(int id);
    
    template<typename.... Args>
    void emplace(int id,Args&&... args);
    
    template<typename.... Args>
    bool try_emplace(int id,Args&&... args);
    
    auto find(int id);
    
    auto begin() noexcept;
    auto end() noexcept;
    
    auto begin() const noexcept;
    auto end() const noexcept;
};
```

这样,就给开发者带来完全一样的体验,支持各种`STL`算法,以及`range for`:

```C++
void usage(IObjectRepository& repo){
    for(auto& [key,value]:repo){
        //遍历过程
    }
    
    bool result = std::any_of(repo.begin(),repo.end(),[&](auto v){
        //判断条件
    });
}
```

其它接口还好处理,但是迭代器实现稍微复杂一些,这里提供了一种迭代器辅助类,来降低工作量.

## `iterator_facade`

在`Boost`中有两个库:`Iterator`与`STLInterfaces`.其中`STLInterfaces`支持更新的`C++`标准,并提供了`C++20`的`ranges`支持.有条件的开发者可以使用`Boost`库来实现迭代器.

实际上开发者可以提供自己的迭代器辅助类,来约定如何实现迭代器,实现起来并不复杂. 这里采用如下约定来简化迭代器定义,并提供参考实现:

- 采用`CRTP`技术,开发者派生自`iterator_facade`实现迭代器类;
- 迭代器类需要提供`operator*()`操作符重载来支持通过迭代器访问容器值引用;
- 迭代器类需要提供`Iterator& advance(std::ptrdiff_t n)`来支持迭代器向前、向后移动,以及随机访问时的多步移动;
- 对于非随机访问类容器,需要提供`bool operator==(const Iterator& other)`,来提供迭代器相等比较;
- 对于随机访问类容器,需要提供`std::ptrdiff_t distance_to(const iterator& rhs) const`,来比较两个迭代器之间的距离。

例如,开发者有个`MyObject`类能够支持随机访问:

```C++
class MyObject
{
public:
    inline std::size_t size() const noexcept;
    double* at(std::size_t i) noexcept;
    const double* at(std::size_t i) const noexcept;
};
```

那么可以通过如下方式实现迭代器类:

```C++
class MyObject{
public:    
    //定义为随机访问迭代器,值类型为double
    struct iterator :abc::iterator_facade<iterator, std::random_access_iterator_tag, double>
    {
        iterator() = default;
        explicit iterator(MyObject* obj, std::size_t i) :m_obj(obj), m_index(i) {};
		
        //访问值引用(如果不希望支持修改,返回值即可)
        double& operator*() const noexcept { return *(m_obj->at(m_index)); }
		
        //移动迭代器:来生成++,--,++,-=等操作
        iterator& advance(std::ptrdiff_t n) noexcept { m_index += n; return *this; }
        //计算两个迭代器之间的距离,来生成迭代器的==,!=,>,>=,<,<=等操作
        std::ptrdiff_t distance_to(const iterator& rhs) const noexcept { return m_index - rhs.m_index; }
    private:
        MyObject* m_obj{};
        std::ptrdiff_t m_index{};
    };    
};
```

之后给`MyObject`提供`begin/end`函数对即可:

```C++
class MyObject
{
public:
    iterator begin() noexcept {
        return iterator{ this,0 };
    }
    
    iterator end() noexcept {
        return iterator{ this,this->size() };
    }
};
```

使用示例如下:

```C++
void usage(MyObject& obj){
    std::vector<double> results;
    //STL算法
    std::copy(obj.begin(), obj.end(), std::back_inserter(results));
	
    //for循环
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        results.push_back(*it);
    }
	
    //range for
    for (auto&& v1 : obj) {
        results.push_back(v1);
    }    
}
```

需要注意的是:

- 对于`const`访问,需要提供另外一种迭代器,例如`const_iterator`,差异在于构造时使用的`const MyObject*`,且`operator*()`返回的也应该是`const`;
- 实际上开发者还会使用迭代器的`operator->()`操作符,这个也需要开发者根据实际情况来实现.

## 扩展用法

之前的`IObjectRepository`定义中,返回的`Object*`是需要再释放掉的,因为它是内存申请的产物,开发者可以将返回值修改为`std::unique_ptr<Object>`,来避免复杂的构造与析构.同时迭代器内容也需要做出调整.

不过追求性能的开发者可能认为每次构造析构会代码性能损耗,因而可以提供缓存,来减少构造析构操作:

```C++
class ICachedObjectRepository{
    std::map<int,std::unique_ptr<Object>> m_caches;
public:
    
    struct iterator{
        Object& operator*() noexcept{
			//查询缓存,存在则返回
            auto it = m_obj->m_caches.find(m_index);
            if(it != m_obj->m_caches.end()){
                return *(it->second);
			}
            //提取,缓存,返回
            auto result = m_obj->at(m_index);
            auto pointer = result.get();
            m_obj->m_caches[m_index] = std::move(pointer);
            return *pointer;
        };
    };
    
    //清理缓存
    void shrink_cache(){
        m_caches.clear();
    }
};
```

## `iterator_facade`参考实现

```C++
#pragma once

#include <type_traits>
#include <iterator>

namespace abc
{
    template<
        typename T,
        typename IteratorCategory,
        typename ValueType,
        typename Reference = ValueType&,
        typename Pointer = ValueType*,
        typename DifferenceType = std::ptrdiff_t,
        typename E = std::enable_if_t<
        std::is_class_v<T>&&
        std::is_same_v<T, std::remove_cv_t<T>>
        >
    >
        struct iterator_facade;

    namespace detail {
        template<typename Iterator>
        constexpr auto is_ra_iter = std::is_base_of_v<std::random_access_iterator_tag,typename Iterator::iterator_category>;

        template<typename T, typename IteratorCategory, typename ValueType, typename Reference, typename Pointer, typename DifferenceType>
        void is_facade(iterator_facade<T, IteratorCategory, ValueType, Reference, Pointer, DifferenceType> const&);

        template<typename Iterator>
        auto distance(Iterator const& lhs, Iterator const& rhs) noexcept(noexcept(lhs.distance_to(rhs)))
            ->decltype(lhs.distance_to(rhs))
        {
            return lhs.distance_to(rhs);
        }
    }

    template<typename T, typename IteratorCategory, typename ValueType, typename Reference, typename Pointer, typename DifferenceType, typename E >
    struct iterator_facade {
    public:
        using difference_type = DifferenceType;
        using value_type = ValueType;
        using pointer = std::conditional_t<std::is_same<IteratorCategory, std::output_iterator_tag>::value, void, Pointer>;
        using reference = Reference;
        using iterator_category = IteratorCategory;

        template<typename U = T>
        U& operator++() noexcept(noexcept(std::declval<U&>().advance(1)))
        {
            return static_cast<U&>(*this).advance(1);
        }

        template<typename U = T>
        U& operator--() noexcept(noexcept(std::declval<U&>().advance(-1)))
        {
            return static_cast<U&>(*this).advance(-1);
        }

        template<typename U = T>
        U operator++(int) noexcept(noexcept(std::declval<U&>().advance(1)))
        {
            U result = static_cast<U&>(*this);
            static_cast<U&>(*this).advance(1);
            return result;
        }

        template<typename U = T>
        U operator--(int) noexcept(noexcept(std::declval<U&>().advance(-1)))
        {
            U result = static_cast<U&>(*this);
            static_cast<U&>(*this).advance(-1);
            return result;
        }

        template<typename U = T>
        U& operator+=(difference_type n) noexcept(noexcept(std::declval<U&>().advance(n)))
        {
            return static_cast<U&>(*this).advance(n);
        }

        template<typename U = T>
        U& operator-=(difference_type n) noexcept(noexcept(std::declval<U&>().advance(-n)))
        {
            return static_cast<U&>(*this).advance(-n);
        }

        template<typename U = T>
        U operator+(difference_type n) const noexcept(noexcept(std::declval<U&>().advance(n)))
        {
            U result = static_cast<const U&>(*this);
            result.advance(n);
            return result;
        }

        template<typename U = T>
        U operator-(difference_type n) const noexcept(noexcept(std::declval<U&>().advance(-n)))
        {
            U result = static_cast<const U&>(*this);
            result.advance(-n);
            return result;
        }

        friend T operator+(difference_type n, const T& it) {
            return it + n;
        }

        template<typename U = T>
        auto operator-(const U& other) const noexcept(noexcept(std::declval<U&>().distance_to(other)))
        {
            return static_cast<const U&>(*this).distance_to(other);
        }
    };

    template<typename Iter, typename E = std::enable_if_t<detail::is_ra_iter<Iter>>>
    auto operator==(const Iter& lhs, const Iter& rhs) noexcept(noexcept(detail::distance(lhs, rhs)))
        ->decltype(detail::is_facade(lhs), detail::distance(lhs, rhs) == 0) {
        return detail::distance(lhs, rhs) == 0;
    }

    template<typename Iter>
    auto operator!=(const Iter& lhs, const Iter& rhs) noexcept(noexcept(!(lhs == rhs)))
        ->decltype(detail::is_facade(lhs), !(lhs == rhs)) {
        return !(lhs == rhs);
    }

    template<typename Iter>
    auto operator<(const Iter& lhs, const Iter& rhs) noexcept(noexcept(detail::distance(lhs, rhs)))
        ->decltype(detail::is_facade(lhs), detail::distance(lhs, rhs) < 0) {
        return detail::distance(lhs, rhs) < 0;
    }

    template<typename Iter>
    auto operator<=(const Iter& lhs, const Iter& rhs) noexcept(noexcept(detail::distance(lhs, rhs)))
        ->decltype(detail::is_facade(lhs), detail::distance(lhs, rhs) <= 0) {
        return detail::distance(lhs, rhs) <= 0;
    }

    template<typename Iter>
    auto operator>(const Iter& lhs, const Iter& rhs) noexcept(noexcept(detail::distance(lhs, rhs)))
        ->decltype(detail::is_facade(lhs), detail::distance(lhs, rhs) > 0) {
        return detail::distance(lhs, rhs) > 0;
    }

    template<typename Iter>
    auto operator>=(const Iter& lhs, const Iter& rhs) noexcept(noexcept(detail::distance(lhs, rhs)))
        ->decltype(detail::is_facade(lhs), detail::distance(lhs, rhs) >= 0) {
        return detail::distance(lhs, rhs) >= 0;
    }
}
```



