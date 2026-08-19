cmake_minimum_required(VERSION 3.25)
project(libexplorer-core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

# 生成的头文件目录
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_library(libexplorer-core
    src/config/Config.cpp
    src/config/Settings.cpp
    src/signals/SignalDispatcher.cpp
    src/registry/Registry.cpp
    src/registry/RegistryWatcher.cpp
    src/model/ExplorerModel.cpp
    src/model/FileSystemModel.cpp
    src/fs/FileSystem.cpp
    src/fs/FileOperations.cpp
)

target_include_directories(libexplorer-core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/libexplorer-core>
)

target_link_libraries(libexplorer-core PUBLIC Qt6::Core)

# 安装
install(TARGETS libexplorer-core
    EXPORT libexplorer-core-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY src/
    DESTINATION include/libexplorer-core
    FILES_MATCHING PATTERN "*.h"
    PATTERN ".gitkeep" EXCLUDE
)

install(EXPORT libexplorer-core-targets
    FILE libexplorer-core-config.cmake
    DESTINATION lib/cmake/libexplorer-core
)