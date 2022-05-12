[TOC]

# 单调递增的类型ID生成

在之前的文章《类型ID的生成及应用》中曾经介绍过一些类型ID的生成方法,可以为类型生成全局唯一的ID.但是这种ID是开放式的,并非从`0`开始,也不紧凑,无法作为`std::vector`容器的索引使用,不能有更好的性能表现.

这里提供一种利用静态变量特性生成可以从`0`开始,单调递增且紧凑的类型`ID`生成方式,以及使用示例.

## 使用场景示例


## 实现方式

假设使用`typeid`来获取类型的代号,定义为`TypeCode`,那么可以以`std::vector<TypeCode>`来存储类型代号,并以其在`vector`中的位置作为类型`ID`:

- 当某类型的`TypeCode`存在于`vector`中时,返回索引值;
- 当某类型的`TypeCode`在`vector`中不存在,则追加到`vector`之后,再返回索引值.

定义类似如下:

```C++
using TypeCode = std::string;

class TypeRegistry{
public:
    int GetIndex(const TypeCode& code);
private:
    std::vector<TypeCode> m_codes;
};
```

局部静态变量只会进行一次初始化,可以利用这种特性:

- 第一次进入函数,局部静态变量通过`TypeRegistry::GetIndex`完成初始化;
- 之后再进入函数,则不会再初始化,而是使用初始化之后的值.

考虑到这会带来全局影响,`TypeRegistry`应当实现成单例,从而使得对应函数可以写:

```C++
class TypeRegistry{
public:
    static TypeRegistry* Get();
};

template<typename T>
void UserCode(){
    static const auto index = TypeRegistry::Get()->GetIndex(typeid(T).name());
}
```

这样`UserCode`在第一次调用时会初始化,相对慢一点,后续不会再有初始化动作,而且也获取了相对紧凑的类型`ID`. 




