cmake_minimum_required(VERSION 3.25)
project(run-dialog LANGUAGES CXX)

# 运行对话框可执行文件
add_executable(run-dialog
    src/main.cpp
    src/RunDialog.cpp
    src/RunDialog.h
)

target_compile_features(run-dialog PRIVATE cxx_std_20)

# 链接所需的库
target_link_libraries(run-dialog PRIVATE
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

# 安装目标
install(TARGETS run-dialog
    RUNTIME DESTINATION bin
)

# 安装桌面文件（通常不显示在菜单中，但提供完整性）
install(FILES run-dialog.desktop DESTINATION share/applications)