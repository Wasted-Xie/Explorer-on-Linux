cmake_minimum_required(VERSION 3.25)
project(libexplorer-utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

set(CMAKE_AUTOMOC ON)

add_library(libexplorer-utils
    src/StringUtils.cpp
    src/FileUtils.cpp
    src/ProcessUtils.cpp
    src/SystemUtils.cpp
    src/DateTimeUtils.cpp
    src/VariantUtils.cpp
)

target_include_directories(libexplorer-utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/libexplorer-utils>
)

target_link_libraries(libexplorer-utils PUBLIC Qt6::Core)

install(TARGETS libexplorer-utils
    EXPORT libexplorer-utils-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY src/
    DESTINATION include/libexplorer-utils
    FILES_MATCHING PATTERN "*.h"
    PATTERN ".gitkeep" EXCLUDE
)

install(EXPORT libexplorer-utils-targets
    FILE libexplorer-utils-config.cmake
    DESTINATION lib/cmake/libexplorer-utils
)