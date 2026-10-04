cmake_minimum_required(VERSION 3.21)
project(SystemMonitor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
enable_language(RC)
if(MINGW)
    # This icon-only resource needs no C++ definitions or Qt include paths.
    # Omitting them avoids windres splitting paths containing spaces.
    set(CMAKE_RC_COMPILE_OBJECT
        "<CMAKE_COMMAND> -E chdir \"${CMAKE_CURRENT_SOURCE_DIR}/resources\" <CMAKE_RC_COMPILER> --use-temp-file -O coff SystemMonitor.rc \"${CMAKE_CURRENT_BINARY_DIR}/<OBJECT>\"")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(MONITOR_SOURCES
    src/MainWindow.h
    src/MainWindow.cpp
    src/AppTheme.h
    src/AppTheme.cpp
    src/MetricCard.h
    src/MetricCard.cpp
    src/WindowsAccess.h
    src/WindowsAccess.cpp
    src/SystemMonitor.h
    src/SystemMonitor.cpp
    src/CpuUsageTracker.h
    src/CpuUsageTracker.cpp
    src/SystemSnapshot.h
    src/CpuHistory.h
    src/CpuHistory.cpp
    src/CpuHistoryGraph.h
    src/CpuHistoryGraph.cpp
    src/GpuMonitor.h
    src/GpuMonitor.cpp
    src/GpuUsageAggregator.h
    src/GpuUsageAggregator.cpp
    src/GpuDashboard.h
    src/GpuDashboard.cpp
    src/TemperatureSnapshot.h
    src/TemperatureSnapshot.cpp
    src/TemperatureMonitor.h
    src/TemperatureMonitor.cpp
    src/MonitoringService.h
    src/MonitoringService.cpp
    src/ProcessMonitor.h
    src/ProcessMonitor.cpp
    src/ProcessTableModel.h
    src/ProcessTableModel.cpp
)
qt_add_executable(SystemMonitor WIN32 src/main.cpp ${MONITOR_SOURCES}
    resources/app.qrc resources/SystemMonitor.rc)
target_link_libraries(SystemMonitor PRIVATE Qt6::Widgets psapi pdh dxgi advapi32)
target_include_directories(SystemMonitor PRIVATE resources)
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
    target_link_libraries(ProcessTests PRIVATE Qt6::Widgets psapi pdh dxgi)
    add_test(NAME process-collection-and-sorting COMMAND ProcessTests)
    set_tests_properties(process-collection-and-sorting PROPERTIES TIMEOUT 15)

    add_executable(CpuHistoryTests tests/CpuHistoryTests.cpp src/CpuHistory.cpp)
    target_include_directories(CpuHistoryTests PRIVATE src)
    add_test(NAME cpu-history-retention COMMAND CpuHistoryTests)
    set_tests_properties(cpu-history-retention PROPERTIES TIMEOUT 10)

    add_executable(MonitoringServiceTests tests/MonitoringServiceTests.cpp
        src/MonitoringService.cpp src/MonitoringService.h src/ProcessMonitor.cpp src/GpuMonitor.cpp src/GpuUsageAggregator.cpp
        src/TemperatureSnapshot.cpp src/TemperatureMonitor.cpp)
    target_include_directories(MonitoringServiceTests PRIVATE src)
    target_link_libraries(MonitoringServiceTests PRIVATE Qt6::Core psapi pdh dxgi)
    add_test(NAME background-process-lifecycle COMMAND MonitoringServiceTests)
    set_tests_properties(background-process-lifecycle PROPERTIES TIMEOUT 15)

    add_executable(GpuTests tests/GpuTests.cpp src/GpuMonitor.cpp src/GpuUsageAggregator.cpp)
    target_link_libraries(GpuTests PRIVATE pdh dxgi)
    target_include_directories(GpuTests PRIVATE src)
    add_test(NAME gpu-usage-and-collection COMMAND GpuTests)
    set_tests_properties(gpu-usage-and-collection PROPERTIES TIMEOUT 15)

    add_executable(TemperatureHelperFixture tests/TemperatureHelperFixture.cpp)
    add_executable(TemperatureTests tests/TemperatureTests.cpp src/TemperatureSnapshot.cpp src/TemperatureMonitor.cpp)
    target_include_directories(TemperatureTests PRIVATE src)
    target_link_libraries(TemperatureTests PRIVATE Qt6::Core)
    add_dependencies(TemperatureTests TemperatureHelperFixture)
    add_test(NAME temperature-protocol-and-lifecycle COMMAND TemperatureTests)
    set_tests_properties(temperature-protocol-and-lifecycle PROPERTIES TIMEOUT 15)

    add_executable(AppThemeTests tests/AppThemeTests.cpp ${MONITOR_SOURCES})
    target_include_directories(AppThemeTests PRIVATE src)
    target_link_libraries(AppThemeTests PRIVATE Qt6::Widgets psapi pdh dxgi advapi32)
    add_test(NAME theme-switching-and-persistence COMMAND AppThemeTests)
    set_tests_properties(theme-switching-and-persistence PROPERTIES TIMEOUT 20)

    add_test(NAME application-startup COMMAND SystemMonitor --smoke-test)
    set_tests_properties(application-startup PROPERTIES TIMEOUT 15)
endif()
