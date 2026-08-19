cmake_minimum_required(VERSION 3.25)
project(explorer-linux VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus)

# libs
add_subdirectory(libs/libexplorer-core)
add_subdirectory(libs/libexplorer-utils)
add_subdirectory(libs/libexplorer-ui)
add_subdirectory(libs/libexplorer-layer)
add_subdirectory(libs/libexplorer-ipc)
add_subdirectory(libs/libexplorer-hotkeys)

# components
add_subdirectory(components/filemanager)
add_subdirectory(components/taskbar)
add_subdirectory(components/startmenu)
add_subdirectory(components/desktop)
add_subdirectory(components/run-dialog)
add_subdirectory(components/search)
add_subdirectory(components/lock-screen)
add_subdirectory(components/notification-center)

# daemon
add_subdirectory(daemon/shell-daemon)

enable_testing()
add_subdirectory(tests)
