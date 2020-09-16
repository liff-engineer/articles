from enum import Enum
from dataclasses import dataclass, field, asdict, fields
from typing import List
import json


@dataclass
class Car:
    makeModel: str = "宝马"
    makeYear: int = 2020
    color: str = "black"
    modelType: str = "X7"


@dataclass
class Employee:
    id: int = 1
    name: str = "长不胖的Garfield"
    level: str = "mid-level"
    languages: List[str] = field(default_factory=lambda:  [
                                 "C++", "Python", "Go"])
    car: Car = Car()


def from_dict(klass, d):
    try:
        fieldtypes = {f.name: f.type for f in fields(klass)}
        return klass(**{f: from_dict(fieldtypes[f], d[f]) for f in d})
    except:
        return d


if __name__ == "__main__":
    me = Employee()

    jsonBuffer = json.dumps(asdict(me), ensure_ascii=False, indent=4)
    print(jsonBuffer)

    result = from_dict(Employee, json.loads(jsonBuffer))
    print(result)
