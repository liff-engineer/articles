#pragma once

#include "dispatcher.hpp"

constexpr char* ActionAdd = "add";
constexpr char* ActionInc = "inc";
constexpr char* ActionDec = "dec";

class ActionCreator
{
    Dispatcher* m_dispatcher = nullptr;
public:
    explicit ActionCreator(Dispatcher* dispatcher)
        :m_dispatcher{ dispatcher } {};

    inline void add() {
        m_dispatcher->dispatch("add", {});
    }

    inline void inc(int id) {
        QMap<QString, QVariant> args;
        args["id"] = id;
        m_dispatcher->dispatch("inc", args);
    }

    inline void dec(int id) {
        QMap<QString, QVariant> args;
        args["id"] = id;
        m_dispatcher->dispatch("dec", args);
    }
};
