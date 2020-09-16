package main

import (
	"encoding/json"
	"fmt"
	"strings"
)

//Car 汽车
type Car struct {
	MakeModel string `json:"makeModel"`
	MakeYear  int    `json:"makeYear"`
	Color     string `json:"color"`
	ModelType string `json:"modelType"`
}

//SkillLevel 技能水平
type SkillLevel int

//技能水平枚举
const (
	Junior SkillLevel = iota
	Midlevel
	Senior
)

//Employee 雇员
type Employee struct {
	ID        int        `json:"id"`
	Name      string     `json:"name"`
	Level     SkillLevel `json:"level"`
	Languages []string   `json:"languages"`
	MyCar     Car        `json:"car"`
}

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
