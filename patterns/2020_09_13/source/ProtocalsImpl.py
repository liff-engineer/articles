from typing import Protocol, Iterable
import sys


class Drawable(Protocol):
    def draw(self, os, positon) -> None:
        pass


class MyClass:

    def draw(self, os, position) -> None:
        print(f"{' '*position}MyClass", file=os)


def draw(document: Iterable[Drawable], os=sys.stdout, position=0):
    print(f"{' '*position}<document>", file=os)
    for e in document:
        e.draw(os, position+2)
    print(f"{' '*position}</document>", file=os)


if __name__ == "__main__":
    document = [
        MyClass()
    ]
    draw(document)
