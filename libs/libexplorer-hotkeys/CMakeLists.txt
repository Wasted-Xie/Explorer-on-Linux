cmake_minimum_required(VERSION 3.25)
project(libexplorer-hotkeys LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core X11Extras)

# 尝试查找 KGlobalAccel (KDE) 或其他热键库
# 这里我们实现一个基于 Qt 的简单热键系统，实际项目中可能需要集成 KGlobalAccel 或 libkeybinder3

set(CMAKE_AUTOMOC ON)

add_library(libexplorer-hotkeys
    src/HotkeyManager.cpp
    src/HotkeyRegistry.cpp
    src/Hotkey.cpp
    src/GlobalShortcut.cpp
)

target_include_directories(libexplorer-hotkeys PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/libexplorer-hotkeys>
)

target_link_libraries(libexplorer-hotkeys PUBLIC Qt6::Core Qt6::X11Extras)

# 如果可用，链接到 KGlobalAccel
# find_package(KF5GlobalAccel REQUIRED)
# if(TARGET KF5::GlobalAccel)
#     target_link_libraries(libexplorer-hotkeys PUBLIC KF5::GlobalAccel)
# endif()

install(TARGETS libexplorer-hotkeys
    EXPORT libexplorer-hotkeys-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY src/
    DESTINATION include/libexplorer-hotkeys
    FILES_MATCHING PATTERN "*.h"
    PATTERN ".gitkeep" EXCLUDE
)

install(EXPORT libexplorer-hotkeys-targets
    FILE libexplorer-hotkeys-config.cmake
    DESTINATION lib/cmake/libexplorer-hotkeys
)