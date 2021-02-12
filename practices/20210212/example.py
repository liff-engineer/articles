from abc import ABCMeta, abstractmethod
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


pokedex = Factory(identify_name='key')


@pokedex.register('fire')
class Charizard(object):
    pass


@pokedex.register
class Squirtle(object):
    key: str = 'water'


for k, v in pokedex.items():
    print(f"{k},{v}")

spark = pokedex.make('fire')
assert isinstance(spark, Charizard)

commands = Factory('name')


class ICommand(metaclass=AutoRegister(commands)):
    @abstractmethod
    def execute(self):
        raise NotImplementedError()


class PrintCommand(ICommand):
    name: str = "Print"

    def execute(self):
        print(f"print!")


print(list(commands.items()))
for k, v in commands.items():
    print(f"{k},{v}")
