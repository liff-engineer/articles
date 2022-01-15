# 通用内存模型(Entity-Component)的易用性增强

在之前《ECS架构模式中的EC实现及应用》一文中,展示了采用`EC`的通用内存模型设计与实现.实际使用下来体验比较差,这里展示以下易用性调整的方案,以及参考设计和实现.

## 为何使用体验较差

使用体验较差的原因是多方面的,关键的几点因素如下:

- `API`设计问题:由于遍历存储库中内容均采用`lambda`表达式,而且参数可变,导致开发者书写代码较为困难,例如

  ```C++
  //遍历所有包含double,std::string两种类型数据的实体
  //注意其回调函数接口依次是标识符、double、std::string
  //不需要再通过entity_view读取
  repo.foreach<double,std::string>([&](int id,double dv,auto&& v) {
      ss.emplace_back(v);
  });
  ```

- 类型驱动导致的代码书写繁琐:由于实体包含的数据都被拆分成具体类型,很多类型仅仅包含一个数据成员,访问代码较长且不易读,例如:

  ```C++
  //描述组件
  struct Description{
      std::string value;
  };
  
  //代号组件
  struct Code{
      std::string value;
  };
  
  void usage(entity_view<> e){
      if(e.exist<Description>()){
          //通过以下方式才能获取具体的信息
          auto description = e.view<Description>().value;
      }
      if(e.exist<Code>()){
          auto code = e.view<Code>().value;
      }
  }
  ```

- 过于通用,导致代码脱离业务,难写难读,不如定义好的业务类型易用,例如:

  ```C++
  //常规业务类型
  class MyObject{
    	std::string m_description;//描述可能不存在
  public:
      std::string code;//代号一定存在,所以设置为public
      
      bool hasDescription(){
          return !m_description.empty();
      }
      const std::string& description() const noexcept{
          return m_description;
      }
      void setDescription(std::string value){
          m_description = value;
      }
  };
  
  void normal_usage(MyObject& obj){
  	if(obj.hasDescription()){
          auto&& desc = obj.description();
      }
  }
  
  //接口不能见名知意,且没有有效性限制,造成不必要的通用性
  void usage(entity_view<>& e){
      if(e.exist<Description>()){
          auto&& desc = e.view<Description>().value;
      }
      //一旦要访问的类型多起来,代码很难受
  }
  ```

  当然,还有其它使用不便的地方,譬如不支持值语义,无法复制存储库;当需要拷贝多个数据组件类型时代码异常繁琐等等.

## 可以通过哪些方式调整

遍历、查找确实是非常高频的代码操作,在处理复杂逻辑时,`lambda`书写不易,能否通过`range for`等方式遍历,甚至是使用`STL`算法,将会降低代码书写难度及复杂度,例如:

```C++
for(auto& e:repo){//遍历所有实体
    //原始写法
    if(e.exist<std::string>()){
		ss.emplace_back(e.view<std::string>());
    }
    //调整为指针形式:减少std::string类型书写
    if(auto vp = e.view<std::string>()){
        ss.emplace_back(*vp);
    }
}
```

很多类型仅仅是包裹了普通的数据类型,为了在实体上做区分而定义成新类型,针对这种,是否可以在访问时直接访问到包装的类型信息?例如:

```C++
//描述组件
struct Description{
    std::string value;
};

//代号组件
struct Code{
    std::string value;
};

//通过业务类型与常规类型映射达成以下效果
void usage(entity_view<> e){
    if(auto vp = e.view<Description>()){
        //这里的vp类型为std::string*
        //虽然不如std::string好用
        //但是基本上可以使用了
        std::string description = *vp;
    }
    //如果不需要判断是否存在,则可以提供默认值
    std::string code = e.value_or<Code>(std::string{});
}
```

复杂且相对确定的业务场景,依然需要开发者提供业务类型,通过少量处理,能够让存储库直接作为业务类型容器使用,实体和业务类型也可以无缝转换,以实现确定性的业务代码,例如:

```C++
struct MyObject{
  	std::string *description{};//描述
    std::string code;//代号
};

void usage(MyObject& obj){
    //常规的业务代码逻辑
}

void somewhere(repository& repo){
    //可以作为MyObject的容器使用,进行遍历
    for(auto& o:repo.views<MyObject>()){
        usage(o);
    }
}
```

## `API`设计调整

考虑到上述问题及可以调整的方向,这里的`API`设计根据如下基本思路进行了调整:

- 存储库是实体的容器,实体是数据组件的容器,可以将`API`调整为类似`STL`容器的方式,保证开发者使用习惯的一致性.
- 提供类型投射/映射机制,让开发者可以将数据组件映射为内含的数据成员,也可以将实体映射为具体的业务类型,并在`API`层面提供访问一致性,譬如实体访问数据组件时返回的是映射出来的类型,存储库可以作为影射出来的业务类型的容器,进行遍历等动作.

### 投射机制设计

提供如下扩展点,供开发者进行类型投射:

```C++
template<typename T, typename E = void>
struct project : std::false_type {};
```

针对数据组件类型,假设开发者定义了如下类型:

```C++
struct Code{
  	std::string value;  
};
```

则需要提供`project`模板的特化,类似如下:

```C++
template<>
struct abc::project<Code> : std::true_type{
    using type = std::string;
    
    static type& to(Code& o) noexcept{
        return o.value;
    } 
    static const type& to(const Code& o) noexcept{
        return o.value;
    } 
}
```

即,提供以下要素:

1. 特化/偏特化类型要派生自`std::true_type`;
2. 提供`using type`类型别名,来指定投射出来的类型;
3. 提供静态的`to`函数来完成原始类型到投射类型的转换.

针对业务类型实体,假设开发者定义了如下实体类型:

```C++
//注意只是用来访问实体信息,所以用了指针,不要持久化
struct TestObject{
    int iV{};
    double* dV;
}
```

则同样需要提供`project`模板的特化,类似如下:

```C++
template<>
struct abc::project<TestObject> : std::true_type {
    using type = typename abc::entity_view<int, double>;

    static void to(type& src, examples::TestObject& dst) {
        //将实体投射到指定数据上,支持值和指针
        //指针可以绑定到原始指针,从而能够修改原始值
        src.project(dst.iV, dst.dV);
    }
};
```

即,提供如下要素:

1. 特化/偏特化类型要派生自`std::true_type`;
2. 提供`using type`类型别名,来指定对应的实体原型,指定参数列表是为了增强性能表现;
3. 提供静态的`to`函数来完成实体类型到业务类型的投射.

### `API`设计

 存储库的`API`分为实体视图`entity_view`和存储库`repository`两大部分,还有一些辅助性的`API`,目前的`API`设计如下:

#### 存储库`API`

| `API`                                        | 说明                                               |
| -------------------------------------------- | -------------------------------------------------- |
| `bool contains(std::size_t key)`             | 是否包含指定的实体                                 |
| `std::size_t size()`                         | 实体个数,注意不是真实的,是最大个数                 |
| `bool empty()`                               | 存储库是否为空                                     |
| `void resize(std::size_t n)`                 | 调整大小,可能会创建或者删除                        |
| `entity_view<> emplace_back(Args&&... args)` | 追加实体,并赋值                                    |
| `void erase(std::size_t key)`                | 移除实体                                           |
| `entity_view<> at(std::size_t key)`          | 获取实体,不保证有效性,需检测后使用                 |
| `entity_view<> operator[](std::size_t key)`  | 获取实体,不保证有效性,需检测后使用                 |
| `auto views<Ts...>()`                        | 获取实体视图容器,其中的实体为`entity_view<Ts...>`  |
| `begin/end/rbegin/rend`                      | 容器默认迭代器,实体类型为`entity_view<>`           |
| `auto project_views<T>()`                    | 获取业务类型视图容器,其中的内容为`T`               |
| `auto values<T>()`                           | 获取指定数据组件类型的容器,其中内容包含`key/value` |
| `entity_view<> find(const T& v)`             | 根据数据组件值查找实体,便捷类                      |

其中`views`、`project_views`、`values`均返回临时的容器类,包含迭代器,可以使用`range for`或有限的`STL`算法.

`values`返回了一种自定义结构`item<T>`,其`API`定义如下:

```C++
template<typename T>
class item {
public:
    std::size_t key() const noexcept;
    const T& value()  const noexcept;
    T& value() noexcept;
};
```

#### 实体视图`API`

| `API`                           | 说明                                                |
| ------------------------------- | --------------------------------------------------- |
| `std::size_t key()`             | 实体的键                                            |
| `void bind(std::size_t key)`    | 根据键替换实体                                      |
| `repository* container()`       | 获取容器,即所属存储库                               |
| `bool contains<Us...>()`        | 检测是否包含指定要求的数据组件类型                  |
| `void emplace(Args&&... args)`  | 设置所包含的数据组件值                              |
| `void merge<Us...>(const E& e)` | 从实体`e`中读取数据组件并写入当前实体               |
| `T* view()`                     | 获取指定的数据组件指针                              |
| `T value_or(const T& v)`        | 获取指定的数据组件,如果不存在则返回指定默认值       |
| `void erase<Us...>()`           | 移除指定的数据组件                                  |
| `void project(Args&... args)`   | 投射数据组件到指定的引用参数上,支持值和指针两种形式 |
| `T as()`                        | 根据当前实体构造并获取指定的业务类型                |
| `void project(T& v)`            | 投射当前实体到已有的业务类型                        |

有以下几点需要说明:

1. `view/value_or`能够正确处理数据组件类型投射,并返回要求的类型信息;
2. `entity_view<Ts...>`之间虽然类型参数不同,但是可以自由转换;
3. `entit_view<Ts...>`可以作为普通值对待.

#### 其它`API`

考虑到有可能从实体读取数据组件值到某个变量,这里提供了`abc::tie`,和`entity_view<>::project`搭配使用:

- 形式为`abc::tie<T>(U& v)`和`abc::tie<T>(U*& vp)`,用来支持值或指针的绑定投射;
- 通过`project`接口可以从实体读取类型`T`的数据,并写入到类型`U`中;
- 只需要在`T`可以投射为`U`的场景下使用.

假设有类型`Description`,定义如下:

```C++
struct Description{
  	std::string value;  
};
```

为其实现了类型投射,可以在读取其值时将其转换为`std::string`:

```C++
template<>
struct abc::project<Description>:std::true_type{
    using type = std::string;
  	static std::string& to(Description& o){ return o.v;}  
};
```

当开发者定义了`std::string`变量,希望从`entit_view`中读取`Description`并写入到该变量,则可以:

```C++
std::string description;
e.project(abc::tie<Description>(description));
```

同样可以用在业务类型映射上,例如:

```C++
struct TestObject {
    int* iV{};
    double* dV{};
    std::string sV{};
    std::string description{};

    static TestObject1 Build(entity_view<> e) {
        TestObject1 result{};
        e.project(result.iV, result.dV, result.sV,
            tie<Description>(result.description));
        return result;
    }
};
```

那么它的投射动作可以这样写:

```C++
static void to(type& src, TestObject& dst) {
    src.project(dst.iV, dst.dV, dst.sV,
            abc::tie<Description>(dst.description));
}
```

从而更好地将特定的业务代码与通用的存储库实现融合.

## 参考实现和使用示例

参考实现位于[https://gitee.com/liff-engineer/articles/tree/master/20211215](https://gitee.com/liff-engineer/articles/tree/master/20211215/README.md).

部分示例代码如下:

```C++
int main(int argc, char** argv) {
    repository repo{};
    {//创建
        auto e = repo.emplace_back();
        e.emplace(1024,1.414, std::string{ "liff.engineer@gmail.com" });
    }
    {//创建+实体操作
        entity_view<double, std::string> e = repo.emplace_back();
        e.emplace(456,3.1415926, std::string{ "liff.cpplang@gmail.com" });
        e.emplace(Description{ "just description" });
		
        //读取,vp类型为std::string*
        if (auto vp = e.view<Description>()) {
            std::cout << *vp << "\n";
        }
		
        //读取或使用默认值,desc类型为std::string
        auto desc = e.value_or<Description>(std::string{});
		
        //将e转换为业务类型
        auto to1 = e.as<abc::examples::TestObject>();

        //project的方式
        abc::examples::TestObject to2;
        e.project(to2);
		
        //project使用示例
        auto obj = TestObject1::Build(e);
        if (obj.sV.empty()) {

        }
    }

    //投射出来的业务类型容器操作
    std::vector<abc::examples::TestObject> values;
    for (auto&& o : repo.project_views<abc::examples::TestObject>()) {
        values.emplace_back(o);
        o.dV = o.iV * 2.0;
    }

    //遍历特定的数据组件
    auto strings = repo.values<std::string>();
    //使用stl算法操作repo创建出来的虚拟容器
    auto it = std::any_of(strings.begin(), strings.end(), [](const auto& v) {  return v.value() == std::string{"liff.cpplang@gmail.com"}; });

    //便捷的find接口
    auto e = repo.find(std::string{ "liff.cpplang@gmail.com" });

    //普通的存储库遍历
    std::vector<int> is{};
    std::vector<std::string> ss{};
    for (auto&& e : repo) {
        if (auto v = e.view<int>()) {
            is.emplace_back(*v);
        }
        if (auto v = e.view<std::string>()) {
            ss.emplace_back(*v);
        }
        auto string = e.value_or(std::string{});
    }
	
    //数据组件遍历
    for (auto&& v : repo.values<int>()) {
        is.emplace_back(v.value());
    }
	
    //指定了类型的实体遍历(不保证有效性)
    for (auto&& e : repo.views<double,std::string>()) {
        if (!e.contains<std::string>())
            continue;
        ss.emplace_back(*e.view<std::string>());
    }

    //存储库复制
    repository repo1 = repo;

    //序列化实现示例
    RepositorySerializer serializer{};
    serializer.AddSerializer<int>("intger")
        .AddSerializer<double>("number")
        .AddSerializer<std::string>("string");

    //存储库->json
    auto json = serializer.ToJson(repo1, "demo");
    std::string strJson = json.dump(4);
	
    //json->存储库
    auto repo2 = serializer.FromJson(json, "demo");
    return 0;
}
```

