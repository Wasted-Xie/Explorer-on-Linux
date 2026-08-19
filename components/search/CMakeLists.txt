cmake_minimum_required(VERSION 3.25)
project(search LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus)

# 生成的头文件目录
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 搜索组件可执行文件
add_executable(search
    src/main.cpp
    src/SearchWindow.cpp
    src/SearchWindow.h
    src/SearchModel.cpp
    src/SearchModel.h
    src/SearchDelegate.cpp
    src/SearchDelegate.h
    src/SearchResult.h
)

target_compile_features(search PRIVATE cxx_std_20)

# 链接所需的库
target_link_libraries(search PRIVATE
    explorer-core
    explorer-utils
    explorer-ui
    explorer-layer
    explorer-ipc
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
)

# 包含目录
target_include_directories(search PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 安装目标
install(TARGETS search
    RUNTIME DESTINATION bin
)

# 安装桌面文件
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/search.desktop
    DESTINATION share/applications
)

# 安装 DBus 服务文件（可选，用于系统集成）
# install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.explorer.Search.service
#     DESTINATION share/dbus-1/services
# )