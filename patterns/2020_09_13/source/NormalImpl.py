from typing import Any, Iterable
import sys


class MyClass:

    def draw(self, os, position) -> None:
        print(f"{' '*position}MyClass", file=os)


def draw(document: Iterable[Any], os=sys.stdout, position=0):
    print(f"{' '*position}<document>", file=os)
    for e in document:
        e.draw(os, position+2)
    print(f"{' '*position}</document>", file=os)


if __name__ == "__main__":
    document = [
        MyClass()
    ]
    draw(document)
