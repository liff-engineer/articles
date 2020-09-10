#pragma once
#include <vector>
#include "action.hpp"

struct StoreItem
{
    int id;
    int value;
};

class Store:public QObject
{
    Q_OBJECT
public:
    explicit Store(QObject* parent)
        :QObject{ parent } {};

    StoreItem* getItemID(int id) noexcept {
        for (auto& item : items) {
            if (item.id == id) {
                return &item;
            }
        }
        return nullptr;
    }

public slots:
    void onDispatched(QString action, QMap<QString, QVariant> args) {
        if (action == ActionAdd)
        {
            items.emplace_back(StoreItem{ nextId++,0 });
        }
        else if (action == ActionInc)
        {
            auto id = args["id"].toInt();
            getItemID(id)->value++;
        }
        else if (action == ActionDec)
        {
            auto id = args["id"].toInt();
            getItemID(id)->value--;
        }
    }
public:
    std::vector<StoreItem> items{};
    int nextId = 1;
};
