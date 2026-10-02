#include "Transport/StaleRecords.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <memory>
#include <windows.h>

namespace Bench {
namespace {
    bool Exited(DWORD a_pid) {
        auto* const opened = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, a_pid);
        if (opened == nullptr) {
            // Any other failure, such as denied access, names a process that may still run.
            return GetLastError() == ERROR_INVALID_PARAMETER;
        }
        const std::unique_ptr<void, decltype(&CloseHandle)> process {opened, &CloseHandle};
        DWORD exitCode = 0;
        return GetExitCodeProcess(process.get(), &exitCode) != 0 && exitCode != STILL_ACTIVE;
    }
}

void RemoveStaleRecords(const QString& a_directory) {
    const QDir directory(a_directory);
    for (const auto& file : directory.entryList({"*.json"}, QDir::Files)) {
        bool named = false;
        const auto pid = QFileInfo(file).completeBaseName().toULong(&named);
        if (named && Exited(pid)) {
            QFile::remove(directory.filePath(file));
        }
    }
}
}
