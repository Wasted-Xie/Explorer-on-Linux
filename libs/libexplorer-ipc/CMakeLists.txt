cmake_minimum_required(VERSION 3.25)
project(libexplorer-ipc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core DBus)

set(CMAKE_AUTOMOC ON)

add_library(libexplorer-ipc
    src/DBusService.cpp
    src/DBusInterface.cpp
    src/DBusObject.cpp
    src/MessageBus.cpp
    src/ServiceWatcher.cpp
)

target_include_directories(libexplorer-ipc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/libexplorer-ipc>
)

target_link_libraries(libexplorer-ipc PUBLIC Qt6::Core Qt6::DBus)

install(TARGETS libexplorer-ipc
    EXPORT libexplorer-ipc-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY src/
    DESTINATION include/libexplorer-ipc
    FILES_MATCHING PATTERN "*.h"
    PATTERN ".gitkeep" EXCLUDE
)

install(EXPORT libexplorer-ipc-targets
    FILE libexplorer-ipc-config.cmake
    DESTINATION lib/cmake/libexplorer-ipc
)