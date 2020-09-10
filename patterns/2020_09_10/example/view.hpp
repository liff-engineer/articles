#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>


struct StoreItem;
class  Dispatcher;
class  Store;
class CounterItemView :public QWidget
{
public:
    explicit CounterItemView(QWidget* parent);

    void bind(Dispatcher* dispatcher) {
        m_dispatcher = dispatcher;
    }
    void refresh(StoreItem* item);
private:
    QPushButton* m_incBtn = nullptr;
    QPushButton* m_decBtn = nullptr;
    QLabel* m_valueLabel = nullptr;
    Dispatcher* m_dispatcher = nullptr;
};


class QVBoxLayout;
class CounterView :public QWidget
{
public:
    explicit CounterView(QWidget* parent);

    void bind(Dispatcher* dispatcher, Store* store)
    {
        m_dispatcher = dispatcher;
        m_store = store;
    }

    void refresh();
private:
    Dispatcher* m_dispatcher = nullptr;
    Store* m_store = nullptr;
    std::vector<CounterItemView*> m_itemViews;
    QVBoxLayout* m_layout = nullptr;
    QPushButton* m_addBtn = nullptr;
};
