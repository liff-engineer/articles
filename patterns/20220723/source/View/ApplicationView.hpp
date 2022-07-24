#pragma once

#include <QtWidgets/QMainWindow>
#include "Command/CommandRunner.hpp"
#include "UiService/UiContext.hpp"

class ApplicationView :public QMainWindow {
public:
    explicit ApplicationView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
private:
    UiContext  m_context;
    std::unique_ptr<CommandRunner> m_runner;
};
