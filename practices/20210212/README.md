# 自注册工厂模式的Python实现

`Python`语言相比`C++`更加灵活,通过修饰器和`metaclass`均可以设计出更好用的工厂模式出来,下面根据`Python`包[ClassRegistry](https://github.com/todofixthis/class-registry)实现所采用的技术来看以下如何利用`Python`语言特性实现通用的自注册工厂.

实现方式与`C++`版本的思路类似:

- 标识和负载处理
- 工厂实现
- 自注册实现

## 标识和负载处理

定义`Creator`来保存类的标识、负载,以及原始类型:

```python
from typing import Optional

class Creator:
    def __init__(self, cls: type, identify: Optional[str] = None, payload: Optional[str] = None):
        self.identify = identify
        self.payload = payload
        self.creator = cls
```

然后提供`__call__`实现来模拟函数(可以直接使用`(参数)`作为函数调用),实现创建支持:

```python
class Creator:
    def __call__(self, *args, **kwargs):
        return self.creator(*args, **kwargs)
```

这里假设标识和负载信息取自类型的属性,变量名分别为`identidy_name`、`payload_name`:

```python
class Creator:
    ##从类型中提取标识和负载
    def extract(self, identify_name: Optional[str] = None, payload_name: Optional[str] = None):
        #如果开发者手动指定标识,则不需要再获取了
        if not self.identify and identify_name:
            self.identify = getattr(self.creator, identify_name)
        if payload_name:
            self.payload = getattr(self.creator, payload_name)
```

提供`__str__`方便调试:

```python
class Creator:  
    def __str__(self):
        return f"identidy:{self.identify},payload:{self.payload},creator:{self.creator}"
```

## 工厂实现

有了`Creator`,工厂实现就简单许多,首先定义标识、负载属性名:

```python
class Factory:
    def __init__(self, identify_name: Optional[str] = None, payload_name: Optional[str] = None):
        self._registry = {}
        self.identify_name = identify_name
        self.payload_name = payload_name
```

然后提供注册实现:

```python
from inspect import isclass as is_class
class Factory:
    ##...
    def register(self, key):
        ##如果是类型,则提取并构造出creator,然后存储起来
        if is_class(key):
            if self.identify_name:
                creator = Creator(key)
                creator.extract(self.identify_name, self.payload_name)
                self._registry[creator.identify] = creator
                return key
            else:
                raise ValueError(f"Factory need key.")
		##如果不是类型,则key是作为用户指定的标识,返回函数,来接收类型作为参数
        def _decorator(cls: type):
            creator = Creator(cls, key)
            creator.extract(self.identify_name, self.payload_name)
            self._registry[creator.identify] = creator
            return cls
        return _decorator

```

这样就可以以类似如下方式声明工厂、注册类:

```python
#无标识属性,必须手工指定
commands = Factory()

class PrintCommand:
	pass

commands.register('Print')(PrintCommand)

#有标识顺序,可以从类型提取
command1s = Factory('key','order')
class Print1Command:
    key:str="Print"
    order:int=1
        
command1s.register(PrintCommand)        
```

当然也可以以装饰器的方式书写:

```python
@commands.register('Print')
class PrintCommand:
	pass

@command1s.register
class Print1Command:
    key:str="Print"
    order:int=1
```

然后提供创建实现:

```python
class Factory:
	##...
    def make(self, key, *args, **kwargs):
        creator = self._registry.get(key)
        if not creator:
            raise ValueError(key)
        return creator(*args, **kwargs)
```

最后,为了外部可以访问工厂信息,提供相应接口:

```python
class Factory:
    def __len__(self):
        return len(self._registry)

    def items(self):
        return self._registry.items()
```

## 自注册实现

自注册采用`metaclass`,能够将某抽象类的实现全部自动注册到工厂中,实现如下:

```python
from abc import ABCMeta
from inspect import isabstract as isabstract

def AutoRegister(factory: Factory, base_type: type = ABCMeta) -> type:
    #自注册方式无法指定标识,工厂必须指定类型的标识属性
    if not factory.identify_name:
        raise ValueError(f"Missing `identidy_name` in {factory}")

    class _metaclass(base_type):
        def __init__(self, what, bases=None, attrs=None):
            super(_metaclass, self).__init__(what, bases, attrs)
            if not isabstract(self):
                ##创建class时自动注册
                factory.register(self)
    return _metaclass
```

接口定义方式如下:

```python
from abc import abstractmethod

commands = Factory('name')
class ICommand(metaclass=AutoRegister(commands)):
    @abstractmethod
    def execute(self):
        raise NotImplementedError()
```

使用方式如下:

```python
class PrintCommand(ICommand):
    name: str = "Print"

    def execute(self):
        print(f"print!")


print(list(commands.items()))
for k, v in commands.items():
    print(f"{k},{v}")

commands.make('Print').execute()    
```

## 完整实现

```python
from abc import ABCMeta
from typing import Optional
from inspect import isclass as is_class
from inspect import isabstract as isabstract

# https://github.com/todofixthis/class-registry
class Creator:
    def __init__(self, cls: type, identify: Optional[str] = None, payload: Optional[str] = None):
        self.identify = identify
        self.payload = payload
        self.creator = cls

    def __call__(self, *args, **kwargs):
        return self.creator(*args, **kwargs)

    def extract(self, identify_name: Optional[str] = None, payload_name: Optional[str] = None):
        if not self.identify and identify_name:
            self.identify = getattr(self.creator, identify_name)
        if payload_name:
            self.payload = getattr(self.creator, payload_name)

    def __str__(self):
        return f"identidy:{self.identify},payload:{self.payload},creator:{self.creator}"


class Factory:
    def __init__(self, identify_name: Optional[str] = None, payload_name: Optional[str] = None):
        self._registry = {}
        self.identify_name = identify_name
        self.payload_name = payload_name

    def __len__(self):
        return len(self._registry)

    def items(self):
        return self._registry.items()

    def make(self, key, *args, **kwargs):
        creator = self._registry.get(key)
        if not creator:
            raise ValueError(key)
        return creator(*args, **kwargs)

    def register(self, key):
        if is_class(key):
            if self.identify_name:
                creator = Creator(key)
                creator.extract(self.identify_name, self.payload_name)
                self._registry[creator.identify] = creator
                return key
            else:
                raise ValueError(f"Factory need key.")

        def _decorator(cls: type):
            creator = Creator(cls, key)
            creator.extract(self.identify_name, self.payload_name)
            self._registry[creator.identify] = creator
            return cls
        return _decorator


def AutoRegister(factory: Factory, base_type: type = ABCMeta) -> type:
    if not factory.identify_name:
        raise ValueError(f"Missing `identidy_name` in {factory}")

    class _metaclass(base_type):
        def __init__(self, what, bases=None, attrs=None):
            super(_metaclass, self).__init__(what, bases, attrs)
            if not isabstract(self):
                factory.register(self)
    return _metaclass
```

