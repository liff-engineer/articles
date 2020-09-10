#pragma once
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QMap>

class Dispatcher:public QObject
{
    Q_OBJECT
public:
    explicit Dispatcher(QObject* parent)
        :QObject{ parent } {};

public slots:
    Q_INVOKABLE void dispatch(QString action, QMap<QString, QVariant> args) {
        emit dispatched(action, args);
    }
signals:
    void dispatched(QString action, QMap<QString,QVariant> args);
};
