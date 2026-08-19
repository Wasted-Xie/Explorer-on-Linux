cmake_minimum_required(VERSION 3.25)
project(explorer-notification-center VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt 组件
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# 源文件
set(SOURCES
    src/main.cpp
    src/NotificationItem.cpp
    src/NotificationModel.cpp
    src/NotificationDelegate.cpp
    src/NotificationCenter.cpp
)

# 头文件（用于 IDE 支持）
set(HEADERS
    src/NotificationItem.h
    src/NotificationModel.h
    src/NotificationDelegate.h
    src/NotificationCenter.h
)

# 可执行文件
add_executable(notification-center ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(notification-center PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
    explorer-core
    explorer-utils
    explorer-ui
    explorer-layer
    explorer-ipc
)

# 编译选项
target_compile_options(notification-center PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# 安装
install(TARGETS notification-center
    RUNTIME DESTINATION bin
)

# 桌面文件安装
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/notification-center.desktop.in
               ${CMAKE_CURRENT_BINARY_DIR}/notification-center.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/notification-center.desktop
        DESTINATION share/applications
        RENAME explorer-notification-center.desktop)

# 图标安装（如果有）
# install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/notification-center.svg
#         DESTINATION share/icons/hicolor/scalable/apps
#         RENAME explorer-notification-center.svg)

# 添加到组件目标（用于统一构建）
add_custom_target(notification-center-component DEPENDS notification-center)