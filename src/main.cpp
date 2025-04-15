// Copyright 2025 VALINET Solutions SRL
// Author(s): Valentin Radu (valentin.radu@valinet.ro)
//
#ifndef _WIN32
#error "Unsupported target"
#endif
#include "pch.hpp"

std::atomic<bool> running{ true };
HANDLE g_exitEvent = nullptr;

BOOL WINAPI CtrlHandler(DWORD ctrlType) noexcept {
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_CLOSE_EVENT) {
        if (g_exitEvent) SetEvent(g_exitEvent);
        running = false;
        return true;
    }
    return false;
}

int main() noexcept {
    DWORD waitStatus = WAIT_FAILED;
    HANDLE hDir = nullptr;
    char buffer[8192]{};
    DWORD bytesReturned = 0;
    OVERLAPPED overlapped = {};
    HANDLE hChangeEvent = nullptr;
    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argv == nullptr || argc < 2) {
        wprintf(L"Usage: dmwatcher.exe <directory_to_watch>.\n");
        if (argv) LocalFree(argv);
        return 1;
    }
    std::filesystem::path directoryToWatch = argv[1];
    LocalFree(argv);
    myassert(std::filesystem::exists(directoryToWatch), "");
    printf("Watching: %S\n", directoryToWatch.c_str());

    g_exitEvent = CreateEventW(nullptr, true, false, nullptr);
    myassert(g_exitEvent, "");
    myassert(SetConsoleCtrlHandler(CtrlHandler, true), "");
	
    myassert(hChangeEvent = CreateEventW(nullptr, true, false, nullptr), "");
    overlapped.hEvent = hChangeEvent;

    myassert(hDir = CreateFileW(directoryToWatch.wstring().c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr), "");

    HANDLE handles[2] = { g_exitEvent, hChangeEvent };
    while (running) {
        myassert(ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), true,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned, &overlapped, nullptr), "");
        myassert((waitStatus = WaitForMultipleObjects(2, handles, false, INFINITE)) != WAIT_FAILED, "");
        if (waitStatus == WAIT_OBJECT_0 + 0) {
            break;
        } else if (waitStatus == WAIT_OBJECT_0 + 1) {
            FILE_NOTIFY_INFORMATION* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
            while (running) {
                std::error_code ec;
                auto fullPath = std::filesystem::weakly_canonical(directoryToWatch / std::wstring(info->FileName, info->FileNameLength / sizeof(WCHAR)), ec);
                if (!ec) {
                    auto timestamp = std::filesystem::last_write_time(fullPath, ec);
                    if (ec) timestamp = std::filesystem::file_time_type::clock::now();
                    auto currentPath = fullPath;

                    printf("Change detected: \"%S\"@%lld.\n", fullPath.wstring().c_str(), std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count());
                    while (currentPath > directoryToWatch && currentPath.has_parent_path()) {
                        currentPath = currentPath.parent_path();
                        if (currentPath < directoryToWatch) break;
                        auto currentPathTimestamp = std::filesystem::last_write_time(currentPath, ec);
                        if (ec || currentPathTimestamp == timestamp) continue;
                        printf("Manipulating \"%S\"@%lld.\n", currentPath.c_str(), std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count());
                        std::filesystem::last_write_time(currentPath, timestamp, ec);
                        if (ec) printf("Error updating timestamp for \"%S\": %s\n", currentPath.c_str(), ec.message().c_str());
                    }
                } else {
                    printf("Change detected, but unable to process: \"%S\\%S\".\n", directoryToWatch.c_str(), info->FileName);
                }
                if (!info->NextEntryOffset) break;
                info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(info) + info->NextEntryOffset);
            }
            ResetEvent(hChangeEvent);
        }
    }

    CloseHandle(hDir);
    CloseHandle(hChangeEvent);
    CloseHandle(g_exitEvent);

    return 0;
}
