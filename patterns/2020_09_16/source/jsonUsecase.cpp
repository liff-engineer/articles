#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <iostream>

using json = nlohmann::json;

struct Car
{
    std::string makeModel = u8"宝马";
    int makeYear = 2020;
    std::string color = "black";
    std::string modelType = "X7";
};

enum class SkillLevel
{
    junior,
    midlevel,
    senior,
};

struct Employee
{
    int id = 1;
    std::string name = u8"长不胖的Garfield";
    SkillLevel level = SkillLevel::midlevel;
    std::vector<std::string> languages = {"C++", "Python", "Go"};
    Car car;
};

NLOHMANN_JSON_SERIALIZE_ENUM(SkillLevel, {{SkillLevel::junior, "junior"},
                                          {SkillLevel::midlevel, "mid-level"},
                                          {SkillLevel::senior, "senior"}})

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