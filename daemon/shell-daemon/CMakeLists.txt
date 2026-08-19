cmake_minimum_required(VERSION 3.25)
project(shell-daemon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui DBus)

# 链接到我们的库
add_subdirectory(../../libs/libexplorer-core)
add_subdirectory(../../libs/libexplorer-utils)
add_subdirectory(../../libs/libexplorer-ipc)
add_subdirectory(../../libs/libexplorer-hotkeys)
add_subdirectory(../../libs/libexplorer-layer)
add_subdirectory(../../libs/libexplorer-ui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(shell-daemon
    src/main.cpp
    src/Daemon.cpp
    src/ServiceManager.cpp
    src/PluginLoader.cpp
    src/WindowManager.cpp
)

target_include_directories(shell-daemon PRIVATE
    ../../libs/libexplorer-core/src
    ../../libs/libexplorer-utils/src
    ../../libs/libexplorer-ipc/src
    ../../libs/libexplorer-hotkeys/src
    ../../libs/libexplorer-layer/src
    ../../libs/libexplorer-ui/src
)

target_link_libraries(shell-daemon PRIVATE
    Qt6::Core Qt6::Gui Qt6::DBus
    libexplorer-core
    libexplorer-utils
    libexplorer-ipc
    libexplorer-hotkeys
    libexplorer-layer
    libexplorer-ui
)

install(TARGETS shell-daemon
    RUNTIME DESTINATION bin
)

install(FILES src/shell-daemon.desktop
    DESTINATION share/applications
)