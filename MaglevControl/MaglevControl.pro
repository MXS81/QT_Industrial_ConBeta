QT       += core gui serialbus widgets

TARGET = MagControl
TEMPLATE = app

INCLUDEPATH += include

# 对于Windows平台
win32 {
    RC_ICONS = mcs.ico
}

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/modbusmanager.cpp \
    src/basecontroller.cpp \
    src/logmanager.cpp \
    src/logwindow.cpp \
    src/stylemanager.cpp \
    src/thememanager.cpp \
    src/recipemanager.cpp \
    src/recipewidget.cpp \
    src/TrackWidget.cpp \
    src/controlpanel.cpp \
    src/globalparametersetting.cpp \
    src/singlemovercontrol.cpp

HEADERS += \
    include/mainwindow.h \
    include/modbusmanager.h \
    include/basecontroller.h \
    include/logmanager.h \
    include/logwindow.h \
    include/stylemanager.h \
    include/thememanager.h \
    include/recipemanager.h \
    include/recipewidget.h \
    include/TrackWidget.h \
    include/MoverData.h \
    include/controlpanel.h \
    include/globalparametersetting.h \
    include/singlemovercontrol.h

# 确保中文显示正常
win32: {
    msvc: QMAKE_CXXFLAGS += /utf-8
}

RESOURCES += \
    Resource/Icon.qrc




