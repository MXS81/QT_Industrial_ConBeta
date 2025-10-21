cmake_minimum_required(VERSION 3.16)

project(ControlSystemUI VERSION 0.1 LANGUAGES CXX)

# 设置自动处理UI、MOC和RCC
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 定义include和src文件夹路径变量
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 查找QT相关组件
find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    SerialBus
    LinguistTools
)

set(TS_FILES ControlSystemUI_zh_CN.ts)

# 修改源文件和头文件路径
set(PROJECT_SOURCES
    ${SRC_DIR}/Main.cpp
    ${SRC_DIR}/MainWindow.cpp
    ${INCLUDE_DIR}/MainWindow.h
    MainWindow.ui
    ${TS_FILES}
    ${INCLUDE_DIR}/JogControlPage.h
    ${SRC_DIR}/JogControlPage.cpp
    ${INCLUDE_DIR}/LoginDialog.h
    ${SRC_DIR}/LoginDialog.cpp
    ${INCLUDE_DIR}/MoverData.h
    ${INCLUDE_DIR}/Moverwidget.h
    ${SRC_DIR}/Moverwidget.cpp
    ${INCLUDE_DIR}/Overviewpage.h
    ${SRC_DIR}/Overviewpage.cpp
    ${INCLUDE_DIR}/Recipemanagerpage.h
    ${SRC_DIR}/Recipemanagerpage.cpp
    ${INCLUDE_DIR}/Trackwidget.h
    ${SRC_DIR}/Trackwidget.cpp
    ${INCLUDE_DIR}/ModbusManager.h
    ${SRC_DIR}/ModbusManager.cpp
    ${INCLUDE_DIR}/ModbusConfigDialog.h
    ${SRC_DIR}/ModbusConfigDialog.cpp
    ${INCLUDE_DIR}/LogWidget.h
    ${SRC_DIR}/LogWidget.cpp
)
# 创建可执行文件
if(Qt6_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(ControlSystemUI
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        include/ConnectionStatusWidget.h
        src/ConnectionStatusWidget.cpp
        include/DebugHelper.h
        src/DebugHelper.cpp
        include/LogWidget.h
        src/LogWidget.cpp
        src/StyleManager.cpp
        src/StyleUtils.cpp
        include/StyleUtils.h
        include/StyleManager.h
        include/AnimatedButton.h
        src/AnimatedButton.cpp
    )
else()
    if(ANDROID)
        add_library(ControlSystemUI SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(ControlSystemUI
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(ControlSystemUI PRIVATE ${INCLUDE_DIR})

target_link_libraries(ControlSystemUI PRIVATE
    Qt6::Widgets
    Qt6::SerialBus
)

# 设置编译定义 - 启用Modbus支持
target_compile_definitions(ControlSystemUI PRIVATE
    MODBUS_SUPPORT_ENABLED
)

# 完成可执行文件配置
if(Qt6_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ControlSystemUI)
endif()


# 添加调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ControlSystemUI PRIVATE
        DEBUG_MODE
    )
endif()
