cmake_minimum_required(VERSION 3.25)
project(explorer-startmenu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libs/libexplorer-core/src
    ${CMAKE_SOURCE_DIR}/libs/libexplorer-utils/src
    ${CMAKE_SOURCE_DIR}/libs/libexplorer-ipc/src
    ${CMAKE_SOURCE_DIR}/libs/libexplorer-layer/src
    ${CMAKE_SOURCE_DIR}/libs/libexplorer-ui/src
)

# 源文件
set(STARTMENU_SOURCES
    src/main.cpp
    src/core/StartMenuManager.cpp
    src/ui/StartMenuWindow.cpp
    src/ui/ApplicationItem.cpp
)

# 头文件
set(STARTMENU_HEADERS
    src/core/StartMenuManager.h
    src/ui/StartMenuWindow.h
    src/ui/ApplicationItem.h
)

# 可执行文件
add_executable(explorer-startmenu ${STARTMENU_SOURCES} ${STARTMENU_HEADERS})

# 链接库
target_link_libraries(explorer-startmenu PRIVATE
    libexplorer-core
    libexplorer-utils
    libexplorer-ipc
    libexplorer-layer
    libexplorer-ui
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
)

# 编译选项
target_compile_options(explorer-startmenu PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror=return-type>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

# 安装
install(TARGETS explorer-startmenu
    RUNTIME DESTINATION bin
)

# 安装桌面文件
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/explorer-startmenu.desktop
    DESTINATION share/applications
)

# 安装图标 (如果有)
# install(FILES
#     ${CMAKE_CURRENT_SOURCE_DIR}/icons/explorer-startmenu.svg
#     DESTINATION share/icons/hicolor/scalable/apps
# )

# 生成配置文件模板
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/startmenu.conf.in
    ${CMAKE_CURRENT_BINARY_DIR}/startmenu.conf
    @ONLY
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/startmenu.conf
    DESTINATION etc/explorer
    RENAME startmenu.conf
)