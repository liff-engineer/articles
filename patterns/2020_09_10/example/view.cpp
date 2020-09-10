#include "view.hpp"
#include <QtWidgets/QBoxLayout>
#include <QtCore/QVariant>
#include <algorithm>
#include "action.hpp"
#include "store.hpp"

CounterItemView::CounterItemView(QWidget* parent)
    :QWidget{parent}
{
    m_incBtn = new QPushButton("+");
    m_decBtn = new QPushButton("-");
    m_valueLabel = new QLabel();

    auto layout = new QHBoxLayout;
    layout->addWidget(m_decBtn);
    layout->addWidget(m_valueLabel);
    layout->addWidget(m_incBtn);
    setLayout(layout);

    QObject::connect(m_decBtn, &QPushButton::clicked, [=]() {
        if (!m_dispatcher) return;
        auto id = m_valueLabel->property("id").toInt();
        ActionCreator{ m_dispatcher }.dec(id);
        });

    QObject::connect(m_incBtn, &QPushButton::clicked, [=]() {
        if (!m_dispatcher) return;
        auto id = m_valueLabel->property("id").toInt();
        ActionCreator{ m_dispatcher }.inc(id);
        });
}

void CounterItemView::refresh(StoreItem* item)
{
    m_valueLabel->setText(QString::number(item->value));
    m_valueLabel->setProperty("id", item->id);
}

CounterView::CounterView(QWidget* parent)
    :QWidget{parent}
{
    m_addBtn = new QPushButton("add");

    auto layout = new QHBoxLayout;
    m_layout = new QVBoxLayout;
    layout->addWidget(m_addBtn);
    layout->addLayout(m_layout);
    setLayout(layout);

    QObject::connect(m_addBtn, &QPushButton::clicked, [=]() {
        if (!m_dispatcher) return;
        ActionCreator{ m_dispatcher }.add();
        });
}

void CounterView::refresh()
{
    auto m = m_itemViews.size();
    auto n = m_store->items.size();
    //已创建的要刷新
    for (auto i = 0ul; i < std::min(m,n); i++) {
        m_itemViews.at(i)->refresh(&(m_store->items.at(i)));
    }
    //不使用的要隐藏
    for (auto i = n; i < m; i++)
    {
        m_itemViews.at(i)->setVisible(false);
    }
    //没有的要创建
    for (auto i = m; i < n; i++)
    {
        auto view = new CounterItemView{nullptr};
        view->bind(m_dispatcher);
        view->refresh(&(m_store->items.at(i)));
        m_layout->addWidget(view);
        m_itemViews.push_back(view);
    }
}
