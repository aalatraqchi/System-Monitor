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
    src/CpuUsageTracker.h
    src/CpuUsageTracker.cpp
    src/SystemSnapshot.h
    src/CpuHistory.h
    src/CpuHistory.cpp
    src/CpuHistoryGraph.h
    src/CpuHistoryGraph.cpp
    src/ProcessMonitor.h
    src/ProcessMonitor.cpp
    src/ProcessTableModel.h
    src/ProcessTableModel.cpp
)
target_link_libraries(SystemMonitor PRIVATE Qt6::Widgets psapi)
if(MINGW)
    target_compile_options(SystemMonitor PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Verify CPU arithmetic, live Windows readings, and application startup.
include(CTest)
if(BUILD_TESTING)
    add_executable(SystemMonitorTests tests/SystemMonitorTests.cpp src/SystemMonitor.cpp src/CpuUsageTracker.cpp)
    target_include_directories(SystemMonitorTests PRIVATE src)
    add_test(NAME live-system-samples COMMAND SystemMonitorTests)
    set_tests_properties(live-system-samples PROPERTIES TIMEOUT 10)

    add_executable(CpuUsageTrackerTests tests/CpuUsageTrackerTests.cpp src/CpuUsageTracker.cpp)
    target_include_directories(CpuUsageTrackerTests PRIVATE src)
    add_test(NAME cpu-usage-calculation COMMAND CpuUsageTrackerTests)
    set_tests_properties(cpu-usage-calculation PROPERTIES TIMEOUT 10)

    add_executable(ProcessTests tests/ProcessTests.cpp src/ProcessMonitor.cpp src/ProcessTableModel.cpp)
    target_include_directories(ProcessTests PRIVATE src)
    target_link_libraries(ProcessTests PRIVATE Qt6::Widgets psapi)
    add_test(NAME process-collection-and-sorting COMMAND ProcessTests)
    set_tests_properties(process-collection-and-sorting PROPERTIES TIMEOUT 15)

    add_executable(CpuHistoryTests tests/CpuHistoryTests.cpp src/CpuHistory.cpp)
    target_include_directories(CpuHistoryTests PRIVATE src)
    add_test(NAME cpu-history-retention COMMAND CpuHistoryTests)
    set_tests_properties(cpu-history-retention PROPERTIES TIMEOUT 10)

    add_test(NAME application-startup COMMAND SystemMonitor --smoke-test)
    set_tests_properties(application-startup PROPERTIES TIMEOUT 15)
endif()
