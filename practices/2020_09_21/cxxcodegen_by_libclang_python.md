[TOC]

## 利用 libclang 与 python 生成 C++代码

[Clang](https://clang.llvm.org/docs/IntroductionToTheClangAST.html)编译器提供了`libclang`,开发者可以使用`libclang`将C++代码解析成抽象语法树,能够用来分析代码,也可以用来生成代码,以下将展示如何利用`libclang`的`Python`语言接口,来解析C++代码并为其生成字符串输出代码.

### 目标

在`example.hpp`这个头文件中定义了一些枚举和结构体,内容如下:

```C++
#pragma once
#include <string>

enum class MyType
{
    A,
    B,
    C
};

struct Address
{
    std::string zipcode;
    std::string detail;
};
```

这里希望为其提供`operator<<`重载,使得我们可以以如下方式使用:

```C++
int main(int argc, char **argv)
{
    MyType type = MyType::B;
    std::cout << "type example:" << type << "\n";
    Address address;
    address.zipcode = "123455";
    address.detail = "liff.engineer@gmail.com";
    std::cout << "address example:" << address << "\n";
    return 0;
}
```

预期输出如下:

```bash
type example:MyType{B}
address exampleAddress:{zipcode = 123455,detail = liff.engineer@gmail.com}
```

### 解决方案

1. 为枚举和结构体提供jinja2模板,用来生成对应的C++代码
2. 使用libclang解析C++代码,获取代码生成所需信息
3. 整合1，2两步形成代码生成器

#### jinja2模板

针对枚举`MyType`,生成的C++代码类似如下:

```C++
inline std::ostream  &operator<<(std::ostream &os, MyType v){
    os << "MyType{";
    switch(v){
        case MyType::A: os << "A";break;
        case MyType::B: os << "B";break;
        case MyType::C: os << "C";break;
    }
    os << "}";
    return os;
}
```

可以看到,需要得到枚举的名字,然后遍历枚举值生成`case`语句.这里为了更灵活,支持多个枚举定义实现,约定枚举信息`Python`存储格式如下:

```python
enums = [
    {
        'decl': "MyType",
        'values': ["A", "B", "C"]
    }
]
```

那么`jinja2`模板形式如下:

```jinja2
{% for e in enums %}
inline std::ostream  &operator<<(std::ostream &os, {{e.decl}} v){
    os << "{{e.decl}}{";
    switch(v){
        {% for v in e['values'] %}
        case {{e.decl}}::{{v}}: os << "{{v}}";break;
        {% endfor %}
    }
    os << "}";
    return os;
}
{% endfor %}
```

这里的`{% for %}`能够遍历所有枚举`enums`,从而得到枚举对象`e`,枚举的名字存储在`decl`中,包含的枚举值存储在`values`中.

结构体需要生成的C++代码类似如下:

```C++
inline std::ostream &operator<<(std::ostream &os, const Address &v) {
    os << "Address{";
    os << "zipcode = "<<v.zipcode;    
    os << ",detail = "<<v.detail;   
    os << "}";
    return os;  
}
```

需要得到结构体名称,包含的成员名称,注意其特殊之处在于`zipcode`前面不带`,`.这里约定结构体信息`Python`存储格式如下:

```python
structs = [
    {
        'decl': "Address",
        'members': [
            {
                'decl': "zipcode"
            },
            {
                'decl': "detail"
            }
        ]
    }
]
```

那么对应的`jinja2`模板形式如下:

```jinja2
{% for e in structs %}
inline std::ostream &operator<<(std::ostream &os, const {{e.decl}} &v) {
    os << "{{e.decl}}{";
    {% for m in e.members %}
    {% if loop.index == 1 %}
    os << "{{m.decl}} = "<<v.{{m.decl}};    
    {% else %}
    os << ",{{m.decl}} = "<<v.{{m.decl}};   
    {% endif %}
    {% endfor %}
    os << "}";
    return os;  
}
{% endfor %}
```

其复杂之处在于需要使用`if loop.index == 1`来判断是否是第一个成员变量,从而控制`,`是否生成.

在`jinja2`模板头部添加如下内容来添加所需头文件:

```jinja2
#pragma once

#include <ostream>
{% for e in headers %}
#include "{{e}}"
{% endfor %}
```

这时就可以利用该`jinja2`模板和`Python`脚本生成C++代码了. 假设上述`jinja2`模板保存为`ostream.jinja`,则此时的示例`codegen.py`内容如下:

```python
from jinja2 import Template

if __name__ == "__main__":
    headers = ["example.hpp"]
    enums = [
        {
            'decl': "MyType",
            'values': ["A", "B", "C"]
        }
    ]
    structs = [
        {
            'decl': "Address",
            'members': [
                {
                    'decl': "zipcode"
                },
                {
                    'decl': "detail"
                }
            ]
        }
    ]
    
    with open('ostream.jinja', encoding='utf-8') as file:
        template = Template(file.read(), trim_blocks=True, lstrip_blocks=True)
        template.stream(headers=headers, enums=enums,
                        structs=structs).dump(f'example_ostream.hpp')
```

上述代码运行需要使用`pip install jinja2`来安装`jinja2`库.

#### 利用libclang从源代码提取信息

`libclang`中包含以下核心概念:

- Index
- TranslationUnit
- Cursor

使用`Index`来管理全局库状态,`TranslationUnit`对应着某个C++文件生成的抽象语法树,`Cursor`表示抽象语法树上的某个节点.

首先我们通过`pip install libclang`来安装`libclang`库.然后以如下代码输出`example.hpp`中包含的信息:

```python
from clang.cindex import *

##创建index并将example.hpp解析为tu
idx = Index.create()
tu = idx.parse('example.hpp')

##使用深度优先遍历当前节点
for n in tu.cursor.walk_preorder():
    ##注意通过该方式屏蔽掉非本文件的抽象语法树节点
    if not n.location.file or n.location.file.name != 'example.hpp':
            continue    
    print(f"Cursor '{n.spelling}' of kind '{n.kind.name}'")
    if n.kind == CursorKind.ENUM_DECL:
        print(f"enum declare:{n.spelling}")
        for i in n.get_children():
            print(f"{n.type.spelling}::{i.spelling}")
    if n.kind == CursorKind.STRUCT_DECL:
        print(f"struct declare:{n.spelling}")
        for i, m in enumerate(n.get_children()):
            print(f"{m.spelling}- {i}")
```

在遍历语法树时,节点`Cursor`可以通过`spelling`获取其命名,通过`kind`得到节点类型,要获取子可以使用`get_children()`方法.

获取枚举信息可以比较节点的`kind`是否为`ENUM_DECL`,然后通过`get_children()`得到枚举的所有值节点,使用节点的`spelling`得到其名称.以下代码可以将其提取为上述要求的枚举格式:

```python
# 如果是枚举定义则获取枚举类型名和值列表
if n.kind == CursorKind.ENUM_DECL:
    values = []
    for i in n.get_children():
        values.append(i.spelling)
    
    enums.append({
            'decl': n.spelling,
            'values': values
        })
```

要获取结构体定义也是类似的方法:

```python
# 如果是结构体定义则获取结构体类型名和成员变量名
if n.kind == CursorKind.STRUCT_DECL:
    members = []
    for i, m in enumerate(n.get_children()):
        members.append({'decl': m.spelling})

    structs.append({
        'decl': n.spelling,
        'members': members
    })
```

#### `CodeGen`整合实现

这里提供`extract_enum_and_struct`方法从头文件读取枚举和结构体信息,替换到步骤1中的示例信息:

```python
from clang.cindex import *
from typing import List, Dict, Any

def extract_enum_and_struct(file: str, enums: List[Dict[str, Any]], structs: List[Dict[str, Any]]):
    idx = Index.create()
    tu = idx.parse(file)
    for n in tu.cursor.walk_preorder():
        # 移除非本文件的定义
        if not n.location.file or n.location.file.name != file:
            continue
        print(f"Cursor '{n.spelling}' of kind '{n.kind.name}'")
        # 如果是枚举定义则获取枚举类型名和值列表
        if n.kind == CursorKind.ENUM_DECL:
            values = []
            for i in n.get_children():
                values.append(i.spelling)
            enums.append({
                'decl': n.spelling,
                'values': values
            })
        # 如果是结构体定义则获取结构体类型名和成员变量名
        if n.kind == CursorKind.STRUCT_DECL:
            members = []
            for i, m in enumerate(n.get_children()):
                members.append({'decl': m.spelling})

            structs.append({
                'decl': n.spelling,
                'members': members
            })
```

然后调整`codegen.py`剩余部分实现:

```python
from jinja2 import Template

if __name__ == "__main__":
    headers = ["example.hpp"]
    enums = []
    structs = []
    for header in headers:
        extract_enum_and_struct(header, enums, structs)

    with open('ostream.jinja', encoding='utf-8') as file:
        template = Template(file.read(), trim_blocks=True, lstrip_blocks=True)
        template.stream(headers=headers, enums=enums,
                        structs=structs).dump(f'example_ostream.hpp')
```

运行`python codegen.py`生成的`example_ostream.hpp`内容如下:

```C++
#pragma once

#include <ostream>
#include "example.hpp"

inline std::ostream  &operator<<(std::ostream &os, MyType v){
    os << "MyType{";
    switch(v){
        case MyType::A: os << "A";break;
        case MyType::B: os << "B";break;
        case MyType::C: os << "C";break;
    }
    os << "}";
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const Address &v) {
    os << "Address{";
    os << "zipcode = "<<v.zipcode;    
    os << ",detail = "<<v.detail;   
    os << "}";
    return os;  
}
```

### 总结

利用`libclang`和`Python`的`jinja2`库,可以在C++标准没有反射支持的情况下生成一些复杂且有规律的C++代码,以此来提升工作效率.可以想象的应用场景有自动生成流化、反流化代码,为C++代码提供`Python`接口等.当然,还有更多的应用场景可以探索.

注意`libclang`没有提供良好的语法提示,某些接口需要查阅安装到`Python`第三方库目录下的`clang/cindex.py`,其上有较为详细的注释.

- [Introduction to the Clang AST](https://clang.llvm.org/docs/IntroductionToTheClangAST.html)
- [Using libclang to Parse C++](https://shaharmike.com/cpp/libclang/)
- [How to automatically generate std::ostream &operator<< for C++ enums and structs](https://rigtorp.se/generating-ostream-operator/)
- [[Implementing a code generator with libclang](http://szelei.me/code-generator/)](http://szelei.me/code-generator/)















