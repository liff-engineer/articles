from abc import ABC, abstractmethod
from typing import List
import sys


class Drawable(ABC):
    @abstractmethod
    def draw(self, os, position):
        pass


class MyClass(Drawable):

    def draw(self, os, position):
        print(f"{' '*position}MyClass", file=os)


def draw(document: List[Drawable], os=sys.stdout, position=0):
    print(f"{' '*position}<document>", file=os)
    for e in document:
        e.draw(os, position+2)
    print(f"{' '*position}</document>", file=os)


if __name__ == "__main__":
    document = [
        MyClass()
    ]
    draw(document)
