#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 确保中文显示正常
   // QApplication::setApplicationName("Modbus上位机");


    MainWindow w;
    w.setAttribute(Qt::WA_QuitOnClose,true);
    QIcon windowIcon(":/mcs.png");  // 注意这里的路径，":/" 开头表示资源文件
    w.setWindowIcon(windowIcon);    // 为主窗口设置图标
   // w.setWindowTitle("MCS V1.0");
    w.setWindowTitle("多功能磁浮作业配置系统");
    w.show();

    return a.exec();
}
