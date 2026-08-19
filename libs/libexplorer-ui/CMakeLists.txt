cmake_minimum_required(VERSION 3.25)
project(libexplorer-ui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(libexplorer-ui
    src/Button.cpp
    src/Label.cpp
    src/TextEdit.cpp
    src/LineEdit.cpp
    src/ComboBox.cpp
    src/ListView.cpp
    src/TreeView.cpp
    src/TabWidget.cpp
    src/MenuBar.cpp
    src/StatusBar.cpp
    src/Dialog.cpp
    src/ThemeManager.cpp
    src/IconProvider.cpp
)

target_include_directories(libexplorer-ui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/libexplorer-ui>
)

target_link_libraries(libexplorer-ui PUBLIC
    Qt6::Core Qt6::Gui Qt6::Widgets
)

install(TARGETS libexplorer-ui
    EXPORT libexplorer-ui-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY src/
    DESTINATION include/libexplorer-ui
    FILES_MATCHING PATTERN "*.h"
    PATTERN ".gitkeep" EXCLUDE
)

install(EXPORT libexplorer-ui-targets
    FILE libexplorer-ui-config.cmake
    DESTINATION lib/cmake/libexplorer-ui
)