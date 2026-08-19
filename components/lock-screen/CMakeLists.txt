cmake_minimum_required(VERSION 3.25)
project(lock-screen LANGUAGES CXX)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Source files
set(SOURCES
    src/main.cpp
    src/LockScreenWindow.cpp
)

# Headers (for AUTOMOC)
set(HEADERS
    src/LockScreenWindow.h
)

# Create executable
add_executable(lock-screen ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(lock-screen PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
    libexplorer-core
    libexplorer-ui
    libexplorer-layer
    libexplorer-ipc
    libexplorer-utils
)

# Set target properties
set_target_properties(lock-screen PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
    AUTOUIC ON
)

# Install target
install(TARGETS lock-screen
    RUNTIME DESTINATION bin
)

# Install desktop file (optional)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lock-screen.desktop
    DESTINATION share/applications
)