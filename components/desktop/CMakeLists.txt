cmake_minimum_required(VERSION 3.25)
project(desktop-component LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Desktop executable
add_executable(explorer-desktop
    src/main.cpp
    src/core/DesktopWindow.cpp
    src/ui/DesktopView.cpp
)

target_include_directories(explorer-desktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(explorer-desktop PRIVATE
    libexplorer-core
    libexplorer-utils
    libexplorer-ui
    libexplorer-layer
    libexplorer-ipc
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
)

# 安装
install(TARGETS explorer-desktop
    RUNTIME DESTINATION bin
)

# 安装 desktop 文件
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/explorer-desktop.desktop
    DESTINATION share/applications
)