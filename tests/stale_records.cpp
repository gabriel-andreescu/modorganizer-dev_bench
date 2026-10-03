#include "Harness.h"
#include "Transport/StaleRecords.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>

namespace {
qint64 ExitedProcess() {
    QProcess process;
    process.start("cmd.exe", {"/c", "exit"});
    process.waitForStarted();
    const auto pid = process.processId();
    process.waitForFinished();
    return pid;
}

QString Record(const QTemporaryDir& a_directory, qint64 a_pid) {
    const auto path = a_directory.filePath(QString::number(a_pid) + ".json");
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    return path;
}
}

int main(int a_argc, char** a_argv) {
    return Tests::Run([&] {
        const QCoreApplication application(a_argc, a_argv);
        const QTemporaryDir directory;
        const auto exited = ExitedProcess();
        if (exited == 0) {
            return 1;
        }
        const auto stale = Record(directory, exited);
        const auto running = Record(directory, QCoreApplication::applicationPid());
        Bench::RemoveStaleRecords(directory.path());
        if (QFile::exists(stale)) {
            return 2;
        }
        return QFile::exists(running) ? 0 : 3;
    });
}
