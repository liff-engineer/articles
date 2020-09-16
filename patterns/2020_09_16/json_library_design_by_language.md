[TOC]

## 基于类型的`json`库设计

`C++`是强类型语言,在语言层面支持开发者自定义的类型,且自定义类型与语言内置类型具有一样的处理方式.由此来降低开发时的心智成本.而类型天然具备的可组合性也使得开发者能够做出非常好的设计.下面将使用`C++`、`Python`、`GO`语言及相应的`json`库来展示基于类型设计的魅力.

### 需求场景

假设有个雇员,需要具备如下信息:

- ID
- 姓名
- 技能水平等级:分为初级、中级、高级
- 所掌握的语言列表
- 车辆

车辆则具备如下信息:

- 制造商
- 制造年份
- 颜色
- 型号

希望在应用程序中表示雇员,并且能够将雇员信息存储到`json`格式的文件中,也能够从文件中加载.

### 采用`C++`的实现

这里采用`nlohmann`的`json`库-[JSON for Modern C++](https://nlohmann.github.io/json/).该库提供了非常方便易用的接口,为STL中的容器以及C++基础类型提供了`json`格式转换处理,使得开发者能够用很少量的代码即可操作`json`.

#### 业务类型定义

首先定义车辆的类型`Car`:

```C++
struct Car
{
    std::string makeModel = u8"宝马";
    int makeYear = 2020;
    std::string color = "black";
    std::string modelType = "X7";
};
```

然后是技能水平,这里以枚举表示:

```C++
enum class SkillLevel
{
    junior,
    midlevel,
    senior,
};
```

最后是雇员类:

```C++
struct Employee
{
    int id = 1;
    std::string name = u8"长不胖的Garfield";
    SkillLevel level = SkillLevel::midlevel;
    std::vector<std::string> languages = {"C++", "Python", "Go"};
    Car car;
};
```

#### 基于类型提供`json`操作

以上定义了两个结构体一个枚举,分别为其提供`json`互转实现.首先看一下枚举,`json`库提供了宏来避免开发者书写过多重复代码:

```C++
NLOHMANN_JSON_SERIALIZE_ENUM(SkillLevel, 
    {{SkillLevel::junior, "junior"},
     {SkillLevel::midlevel, "mid-level"},
     {SkillLevel::senior, "senior"}}
)
```

然后为车辆类型提供转换操作:

```C++
void from_json(json const &j, Car &obj)
{
    j.at("makeModel").get_to(obj.makeModel);
    j.at("makeYear").get_to(obj.makeYear);
    j.at("color").get_to(obj.color);
    j.at("modelType").get_to(obj.modelType);
}

void to_json(json &j, Car const &obj)
{
    j = json{
        {"makeModel", obj.makeModel},
        {"makeYear", obj.makeYear},
        {"color", obj.color},
        {"modelType", obj.modelType}};
}
```

从上可以看到,`from_json`完成解析,`to_json`完成转换,`std::string`类型的成员变量已被自动处理.

接下来是雇员类:

```C++
void from_json(json const &j, Employee &obj)
{
    j.at("id").get_to(obj.id);
    j.at("name").get_to(obj.name);
    j.at("level").get_to(obj.level);
    j.at("languages").get_to(obj.languages);
    j.at("car").get_to(obj.car);
}

void to_json(json &j, Employee const &obj)
{
    j = json{
        {"id", obj.id},
        {"name", obj.name},
        {"level", obj.level},
        {"languages", obj.languages},
        {"car", obj.car}};
}
```

从中可以看到,如果为成员变量的类型提供了对应的`from_json`和`to_json`实现,那么书写方式完全一致.无论是容器还是自定义类型均被自动处理.

#### 使用示例

以下构造一个雇员并将其转换为`json`格式字符串,然后再从字符串中解析出雇员信息.

```C++
int main(int argc, char **argv)
{
    Employee me;
    json j1;
    to_json(j1, me);

    auto jsonBuffer = j1.dump(4);
    std::cout << jsonBuffer << "\n";

    Employee result;
    auto j2 = json::parse(jsonBuffer);
    from_json(j2, result);
    std::cout << "Employee:" << result.name << "\n";

    return 0;
}
```

#### `C++`实现的总结

从上述场景可以看到,这里充分利用了类型的组合特征,为每种类型提供`json`操作,那么由这些类型组合而成的新类型实现`json`操作时也是同样的模式,非常便捷.

### 采用`Python`的实现

`Python`是动态类型、解释型语言,由于其灵活性,转换成`json`容易,解析则比较困难.而语言也不断吸取"教训",推出了一些"静态"类型特性,譬如如果要表达纯数据类型,则可以使用在`Python 3.7`中引入的`dataclass`装饰器,下面将配合`Python`的`json`库以及`dataclass`装饰器来展示如何实现.

#### 业务类型定义

首先是车辆信息`Car`:

```python
from dataclasses import dataclass

@dataclass
class Car:
    makeModel: str = "宝马"
    makeYear: int = 2020
    color: str = "black"
    modelType: str = "X7"
```

注意这里为每个成员提供了类型信息.

由于`Python`中的枚举在做转换时无法融入这里描述的方案,技能水平将以字符串形式表示,那么雇员类可以实现如下:

```python
from dataclasses import dataclass, field
from typing import List

@dataclass
class Employee:
    id: int = 1
    name: str = "长不胖的Garfield"
    level: str = "mid-level"
    languages: List[str] = field(default_factory=lambda:  [
                                 "C++", "Python", "Go"])
    car: Car = Car()
```



#### 基于类型的`json`操作

`Python`会为每种对象提供`dict`,尤其是`dataclass`,会根据其定义为其生成合适的转换为`dict`的函数支持,即`dataclasses`包中的`asdict`函数,譬如如下示例:

```python
if __name__ == "__main__":
    me = Employee()

    jsonBuffer = json.dumps(asdict(me), ensure_ascii=False, indent=4)
    print(jsonBuffer)
```

其中`asdict(me)`就会将类型转换为字典,可以直接生成`json`.

而从`json`解析并转换为具体的类型实例则稍微麻烦一些,需要提供类似于`asdict`的实现,一种可能实现如下:

```python
from dataclasses import asdict, fields

def from_dict(klass, d):
    try:
        fieldtypes = {f.name: f.type for f in fields(klass)}
        return klass(**{f: from_dict(fieldtypes[f], d[f]) for f in d})
    except:
        return d
```

这个`from_dict`函数可以将字典`d`转换为类型`klass`的实例.

基于上述描述,前文的`C++`示例以`python`实现是如下形式:

```python
if __name__ == "__main__":
    me = Employee()

    jsonBuffer = json.dumps(asdict(me), ensure_ascii=False, indent=4)
    print(jsonBuffer)

    result = from_dict(Employee, json.loads(jsonBuffer))
    print(result)
```

#### `Python`实现的总结

`Python`语言有其特点,也有其优势,可以看到,之前`C++`实现需要为每种类型提供`json`操作,在`python`中只需要利用好`dataclass`修饰器和类型标识,即可以非常廉价的成本实现`json`操作. 

而从展示的示例中也不难看出,只有具备满足需要的类型信息,基于类型组合的特定,才能达到这种效果.

### 采用`Go`的实现

`Go`提供的`json`操作则更具吸引力,简单而且强大.下面使用`Go`自身的`json`库来展示下其设计.

#### 业务类型定义

首先定义车辆信息如下:

```go
//Car 汽车
type Car struct {
	MakeModel string `json:"makeModel"`
	MakeYear  int    `json:"makeYear"`
	Color     string `json:"color"`
	ModelType string `json:"modelType"`
}
```

由于`Go`语言要求可导出的符号必须首字母大写,为了保证和其它语言生成`json`的互操作性,这里添加`tag`,每个成员后面的`json:`追加的即是其输出`json`是对应的键名.

然后是枚举:

```go
//SkillLevel 技能水平
type SkillLevel int

//技能水平枚举
const (
	Junior SkillLevel = iota
	Midlevel
	Senior
)
```

最后是雇员信息:

```go
//Employee 雇员
type Employee struct {
	ID        int        `json:"id"`
	Name      string     `json:"name"`
	Level     SkillLevel `json:"level"`
	Languages []string   `json:"languages"`
	MyCar     Car        `json:"car"`
}
```

#### 基于类型的`json`操作

对于技能水平的枚举,需要为其提供`json`操作:

```go
//UnmarshalJSON 技能水平枚举从json解析
func (level *SkillLevel) UnmarshalJSON(b []byte) error {
	var buffer string
	if err := json.Unmarshal(b, &buffer); err != nil {
		return err
	}
	switch strings.ToLower(buffer) {
	default:
		*level = Junior
	case "mid-level":
		*level = Midlevel
	case "senior":
		*level = Senior
	}
	return nil
}

//MarshalJSON 技能水平枚举转换为json
func (level SkillLevel) MarshalJSON() ([]byte, error) {
	var buffer string
	switch level {
	default:
		buffer = "junior"
	case Midlevel:
		buffer = "mid-level"
	case Senior:
		buffer = "senior"
	}
	return json.Marshal(buffer)
}
```

可以看到,基本上是将枚举和字符串进行互相处理,而区分使用的就是枚举类型`SkillLevel`.

`Go`语言具备反射特性,可以分析类型的构成,所以它能够自动处理类型到`json`的转换动作,即,除非如技能水平这种枚举,基本上不需要提供别的操作.

下面是`C++`示例的`Go`版本:

```go
func main() {
	me := Employee{
		ID:        1,
		Name:      "长不胖的Garfield",
		Level:     Midlevel,
		Languages: []string{"C++", "Python", "Go"},
		MyCar: Car{
			MakeModel: "宝马",
			MakeYear:  2020,
			Color:     "black",
			ModelType: "X7",
		},
	}

	jsonBuffer, _ := json.MarshalIndent(me, "", "    ")
	fmt.Println(string(jsonBuffer))

	var result Employee
	if err := json.Unmarshal(jsonBuffer, &result); err == nil {
		fmt.Println("%v", result)
		fmt.Println("%s", result.Name)
	}
}
```

#### `Go`实现的总结

可以看到`Go`实现`json`操作时最为直接且方便,同时也具备自定义的能力.具备反射特性的强类型语言就是香.

### 总结

在数年之前曾经短暂地学习过`Go`,当时不太能接受`Go`解析`json`是必须解析到指定类型,觉得非常不灵活,接受不了.那时候使用的是`Qt`等库提供的`json`操作,习惯于可以以`json`格式结构去搜寻需要的数据,而不是将其转换为类型.

现在来看,如果以类型的视角来看问题域,是能够做出简单易用的设计的,类型是组合式的,如果基于类型看问题,就可以把大问题拆分成小问题,把大型结构拆分成以特定规律构成的小结构,这是处理问题一种非常优美的思路.









