cmake_minimum_required(VERSION 3.25)
project(libexplorer-layer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

# 查找 wayland-client 和 wayland-protocols
# 这些通常通过 pkg-config 提供
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-protocols)
pkg_check_modules(WAYLAND_SCRIPTS REQUIRED wayland-scanner)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(libexplorer-layer
    src/LayerShell.cpp
    src/LayerSurface.cpp
    src/LayerShellManager.cpp
)

target_include_directories(libexplorer-layer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/libexplorer-layer>
    ${WAYLAND_INCLUDE_DIRS}
    ${WAYLAND_SCRIPTS_INCLUDE_DIRS}
)

target_link_libraries(libexplorer-layer PUBLIC
    Qt6::Core Qt6::Gui
    ${WAYLAND_LIBRARIES}
)

# 生成 wayland 协议头文件
find_program(WAYLAND_SCANNER wayland-scanner)
if(WAYLAND_SCANNER)
    # 这里应该生成 layer-shell 协议的头文件
    # 但为了简化，我们假设这些头文件已经可用
    # 实际项目中需要正确的 wayland-scanner 集成
endif()

install(TARGETS libexplorer-layer
    EXPORT libexplorer-layer-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY src/
    DESTINATION include/libexplorer-layer
    FILES_MATCHING PATTERN "*.h"
    PATTERN ".gitkeep" EXCLUDE
)

install(EXPORT libexplorer-layer-targets
    FILE libexplorer-layer-config.cmake
    DESTINATION lib/cmake/libexplorer-layer
)