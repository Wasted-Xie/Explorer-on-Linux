cmake_minimum_required(VERSION 3.25)
project(filemanager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus)

# 链接到我们的核心库
add_subdirectory(../../../libs/libexplorer-core)
add_subdirectory(../../../libs/libexplorer-utils)
add_subdirectory(../../../libs/libexplorer-ipc)
add_subdirectory(../../../libs/libexplorer-hotkeys)
add_subdirectory(../../../libs/libexplorer-layer)
add_subdirectory(../../../libs/libexplorer-ui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(filemanager
    src/main.cpp
    src/FileManagerWindow.cpp
    src/FileManagerModel.cpp
    src/FileManagerView.cpp
)

target_include_directories(filemanager PRIVATE
    ../../../libs/libexplorer-core/src
    ../../../libs/libexplorer-utils/src
    ../../../libs/libexplorer-ipc/src
    ../../../libs/libexplorer-hotkeys/src
    ../../../libs/libexplorer-layer/src
    ../../../libs/libexplorer-ui/src
)

target_link_libraries(filemanager PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus
    libexplorer-core
    libexplorer-utils
    libexplorer-ipc
    libexplorer-hotkeys
    libexplorer-layer
    libexplorer-ui
)

install(TARGETS filemanager
    RUNTIME DESTINATION bin
)

install(FILES src/filemanager.desktop
    DESTINATION share/applications
)