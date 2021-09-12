#include <algorithm>
#include "Broker.hpp"
#include "JsonIO.hpp"

struct GeometryId {
    int v;
};

bool operator==(GeometryId const& lhs, GeometryId const& rhs) {
    return lhs.v == rhs.v;
}

bool operator!=(GeometryId const& lhs, GeometryId const& rhs) {
    return lhs.v != rhs.v;
}


struct Point {
    double x;
    double y;

    void move(Point v) {
        x += v.x;
        y += v.y;
    }
};
struct Line {
    GeometryId id;
    Point  pt1;
    Point  pt2;
};

class LineRepo
{
    std::vector<Line> m_objects;
    GeometryId nextId(int* restart = nullptr) {
        static int id = 0;
        if (restart) {
            id = *restart;
        }
        return GeometryId{ id++ };
    }
public:
    LineRepo() = default;

    void reboot() {
        int restart = -1;
        nextId(&restart);
        m_objects.clear();
    }

    GeometryId create(Point pt1, Point pt2) {
        Line result;
        result.id = nextId();
        result.pt1 = pt1;
        result.pt2 = pt2;
        m_objects.emplace_back(std::move(result));
        return m_objects.back().id;
    }

    void destory(GeometryId id) {
        m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(),
            [&](auto& obj) {
                return obj.id == id;
            }), m_objects.end());
    }

    Line* find(GeometryId id) noexcept {
        for (auto& obj : m_objects) {
            if (obj.id == id) {
                return std::addressof(obj);
            }
        }
        return nullptr;
    }
};


struct LineOprService
{
    LineRepo* repo;

    Line* create(Point p1, Point p2) {
        return repo->find(repo->create(p1, p2));
    }

    void  destory(Line* line) {
        if (line) {
            return repo->destory(line->id);
        }
    }

    void  move(Line* line, Point v) {
        line->pt1.move(v);
        line->pt2.move(v);
    }
};

struct CreateLine {
    Point p1;
    Point p2;
};

struct DestoryLine {
    GeometryId id;
};

struct MoveLine {
    GeometryId id;
    Point v;
};

struct LineOprProxy
{
    abc::Broker* broker;
    LineRepo* repo;

    void on(CreateLine const& e, GeometryId& result) {
        result = repo->create(e.p1, e.p2);
    }

    void on(DestoryLine const& e) {
        repo->destory(e.id);
    }

    void on(MoveLine const& e) {
        auto line = repo->find(e.id);
        if (line) {
            line->pt1.move(e.v);
            line->pt2.move(e.v);
        }
    }

    Line* create(Point p1, Point p2) {
        auto id = broker->dispatch<GeometryId>(CreateLine{ p1,p2 });
        return repo->find(id);
    }

    void  destory(Line* line) {
        if (line) {
            broker->dispatch(DestoryLine{ line->id });
        }
    }

    void  move(Line* line, Point v) {
        if (line) {
            broker->dispatch(MoveLine{ line->id,v });
        }
    }
};

//事件及结果类型的序列化/反序列化实现
void to_json(nlohmann::json& j, GeometryId const& o) {
    j = {
        {"__entity_identify_type__",typeid(GeometryId).name()},
        {"v",o.v}
    };
}

void from_json(nlohmann::json const& j, GeometryId& o) {
    j.at("v").get_to(o.v);
}

void to_json(nlohmann::json& j, Point const& o) {
    j = {
        {"x",o.x},
        {"y",o.y}
    };
}

void from_json(nlohmann::json const& j, Point& o) {
    j.at("x").get_to(o.x);
    j.at("y").get_to(o.y);
}


void to_json(nlohmann::json& j, CreateLine const& o) {
    j = {
        {"p1",o.p1},
        {"p2",o.p2}
    };
}
void from_json(nlohmann::json const& j, CreateLine& o) {
    j.at("p1").get_to(o.p1);
    j.at("p2").get_to(o.p2);
}

void to_json(nlohmann::json& j, DestoryLine const& o) {
    j = {
        {"id",o.id}
    };
}

void from_json(nlohmann::json const& j, DestoryLine& o) {
    j.at("id").get_to(o.id);
}

void to_json(nlohmann::json& j, MoveLine const& o) {
    j = {
        {"id",o.id},
        {"v",o.v}
    };
}

void from_json(nlohmann::json const& j, MoveLine& o) {
    j.at("id").get_to(o.id);
    j.at("v").get_to(o.v);
}


struct Uow {
    abc::Broker* broker;
    LineRepo* repo;
};

void InitUow(Uow uow)
{
    uow.broker->registerHandler(
        abc::MakeDispatchHandler(LineOprProxy{ uow.broker,uow.repo })
        .inject<CreateLine, GeometryId>()
        .inject<DestoryLine>()
        .inject<MoveLine>()
    );
}

void SomeActions(Uow uow)
{
    LineOprProxy opr{ uow.broker,uow.repo };

    std::vector<GeometryId> keys;
    {
        auto line = opr.create(Point{ 1,1 }, Point{ 0,0 });
        opr.move(line, Point{ 10,0 });
        keys.push_back(line->id);
    }
    {
        auto line = opr.create(Point{ 100,1 }, Point{ 0,40 });
        opr.move(line, Point{ -10,-5 });
    }

    for (auto& key : keys) {
        opr.destory(uow.repo->find(key));
    }
}


void registerWriters(JsonWriter& writer) {
    writer.registerWriter<CreateLine,GeometryId>();
    writer.registerWriter<DestoryLine>();
    writer.registerWriter<MoveLine>();
}

void registerReaders(JsonReader& reader) {
    reader.registerReader<CreateLine, GeometryId>();
    reader.registerReader<DestoryLine>();
    reader.registerReader<MoveLine>();
}

#include <iostream>
int main(int argc, char** argv) {
    LineRepo     repo;
    abc::Broker broker;

    //注册动作记录模块
    auto writer = std::make_shared<JsonWriter>();
    registerWriters(*writer);
    broker.setWriter(writer);

    //模块运行
    Uow  uow{ &broker,&repo };
    InitUow(uow);
    SomeActions(uow);

    //保存模块运行过程执行的动作
    auto json = writer->dump();
    SaveAndLoadEventHandler().on(writer->result(), "actions.json");


    //从动作记录中读取
    JsonReader reader;
    registerReaders(reader);
    auto actions = reader.read(writer->result());

    //禁止动作记录,并回放动作
    //这时回放结果应该是不匹配的
    broker.setWriter(nullptr);
    for (auto& action : actions) {
        broker.replay(*action);
    }

    //为了正确回放,需要将状态重置
    repo.reboot();
    for (auto& action : actions) {
        broker.replay(*action);
    }
#if 1
    broker.registerHandler(
        abc::MakeCallableHandler([](std::string const& arg, std::size_t& result) { result = arg.size(); })
        .inject<std::string, std::size_t>()
    );

    std::string str1 = "liff-b@glodon.com";
    std::cout << "str1(" << str1 << ") length:" << broker.dispatch<std::size_t>(str1) << "\n";

    std::string str2 = "dispatcher";
    std::cout << "str2(" << str2 << ") length:" << broker.dispatch<std::size_t>(str2) << "\n";
#endif
    return 0;
}
