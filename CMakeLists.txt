cmake_minimum_required(VERSION 3.16)
project(MidiKeyboardMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Enable Qt MOC processing
set(CMAKE_AUTOMOC ON)

# Find RtMidi
find_package(PkgConfig REQUIRED)
pkg_check_modules(RTMIDI REQUIRED rtmidi)

# Add executable
add_executable(midi-monitor
    main.cpp
    MidiKeyboardMonitor.cpp
    MidiKeyboardMonitor.h
)

# Link libraries
target_link_libraries(midi-monitor
    Qt6::Core
    Qt6::Widgets
    ${RTMIDI_LIBRARIES}
)

# Include directories
target_include_directories(midi-monitor PRIVATE
    ${RTMIDI_INCLUDE_DIRS}
)

# Set Qt6 properties
set_target_properties(midi-monitor PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)