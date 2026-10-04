cmake_minimum_required(VERSION 3.21)
project(SystemMonitor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(SystemMonitor WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/SystemMonitor.h
    src/SystemMonitor.cpp
    src/SystemSnapshot.h
)
target_link_libraries(SystemMonitor PRIVATE Qt6::Widgets)
if(MINGW)
    target_compile_options(SystemMonitor PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Check that Qt can create the window and exit cleanly. This is not a feature test.
include(CTest)
if(BUILD_TESTING)
    add_executable(SystemMonitorTests tests/SystemMonitorTests.cpp src/SystemMonitor.cpp)
    target_include_directories(SystemMonitorTests PRIVATE src)
    add_test(NAME physical-memory-sample COMMAND SystemMonitorTests)
    set_tests_properties(physical-memory-sample PROPERTIES TIMEOUT 10)

    add_test(NAME application-startup COMMAND SystemMonitor --smoke-test)
    set_tests_properties(application-startup PROPERTIES TIMEOUT 15)
endif()
