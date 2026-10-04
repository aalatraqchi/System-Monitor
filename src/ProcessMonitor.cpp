#include "ProcessMonitor.h"
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

namespace {
// Each handle closes automatically, including when a sample fails early.
class WindowsHandle {
public:
    explicit WindowsHandle(HANDLE value) : value_(value) {}
    ~WindowsHandle() { if (valid()) CloseHandle(value_); }
    WindowsHandle(const WindowsHandle&) = delete;
    WindowsHandle& operator=(const WindowsHandle&) = delete;
    bool valid() const { return value_ && value_ != INVALID_HANDLE_VALUE; }
    HANDLE get() const { return value_; }
private:
    HANDLE value_;
};
}

std::optional<std::vector<ProcessInfo>> ProcessMonitor::sample(const std::function<bool()>& cancelled) const
{
    if (cancelled && cancelled()) return std::nullopt;
    WindowsHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snapshot.valid()) return std::nullopt;

    std::vector<ProcessInfo> processes;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (!Process32FirstW(snapshot.get(), &entry)) {
        if (GetLastError() == ERROR_NO_MORE_FILES) return processes;
        return std::nullopt;
    }

    do {
        // Let shutdown interrupt a long scan without leaking its snapshot handle.
        if (cancelled && cancelled()) return std::nullopt;
        ProcessInfo process{entry.th32ProcessID, entry.szExeFile, std::nullopt};
        WindowsHandle handle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                         FALSE, entry.th32ProcessID));
        PROCESS_MEMORY_COUNTERS memory{};
        memory.cb = sizeof(memory);
        if (handle.valid() && GetProcessMemoryInfo(handle.get(), &memory, sizeof(memory))) {
            process.workingSetBytes = static_cast<std::uint64_t>(memory.WorkingSetSize);
        }
        // Protected or recently exited processes keep their name and ID.
        processes.push_back(std::move(process));
    } while (Process32NextW(snapshot.get(), &entry));

    if (GetLastError() != ERROR_NO_MORE_FILES) return std::nullopt;
    return processes;
}
