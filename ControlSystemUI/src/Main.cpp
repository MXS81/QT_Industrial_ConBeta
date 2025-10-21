#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QDebug>
#include "MainWindow.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用信息
    app.setApplicationName("SKZR轨道控制系统");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("SKZR Tech");

        // 设置深色主题
        app.setStyle(QStyleFactory::create("Fusion"));

        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(31, 41, 55));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(22, 33, 62));
        darkPalette.setColor(QPalette::AlternateBase, QColor(15, 52, 96));
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(15, 52, 96));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, QColor(233, 69, 96));
        darkPalette.setColor(QPalette::Link, QColor(59, 130, 246));
        darkPalette.setColor(QPalette::Highlight, QColor(83, 52, 131));
        darkPalette.setColor(QPalette::HighlightedText, Qt::white);

        // 设置禁用状态颜色
        darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
        darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
        darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));

        app.setPalette(darkPalette);

        //app.setStyleSheet(StyleManager::getGlobalStyleSheet());
        // 创建并显示主窗口
        MainWindow window;
        window.show();
        return app.exec();

}
