#include "Transport/Runtime.h"
#include "Json.h"
#include "Transport/StaleRecords.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QSaveFile>
#include <QUuid>
#include <qtenvironmentvariables.h>
#include <stdexcept>
#include <string>

namespace Bench {
Runtime::Runtime(const QString& a_instance, const QString& a_game) {
    const auto install = QDir(QCoreApplication::applicationDirPath()).canonicalPath();
    const auto session = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto pid = QCoreApplication::applicationPid();
    const auto directory = qEnvironmentVariable("LOCALAPPDATA") + "/devbench/mo2/instances";
    QDir().mkpath(directory);
    RemoveStaleRecords(directory);
    _record = directory + "/" + QString::number(pid) + ".json";
    _identity = {
        {"install", Utf8(install)},
        {"instance", Utf8(a_instance)},
        {"game", Utf8(a_game)},
        {"session", Utf8(session)},
        {"pid", pid},
        {"exe", Utf8(QCoreApplication::applicationFilePath())},
    };
}

void Runtime::Publish(int a_port) {
    _identity["port"] = a_port;
    QSaveFile record(_record);
    if (!record.open(QIODevice::WriteOnly)) {
        throw std::runtime_error("Cannot publish runtime discovery file");
    }
    record.write(QByteArray::fromStdString(_identity.dump(2)));
    if (!record.commit()) {
        throw std::runtime_error("Cannot commit runtime discovery file");
    }
    const auto setup = Text(_identity["install"]) + "/plugins/dev_bench/mcp-bridge.json";
    QSaveFile snippet(setup);
    if (snippet.open(QIODevice::WriteOnly)) {
        snippet.write(QByteArray::fromStdString(Bridge().dump(2)));
        snippet.commit();
    }
}

void Runtime::Remove() const {
    QFile::remove(_record);
}

Json Runtime::Identity() const {
    return _identity;
}

Json Runtime::Bridge() const {
    auto args = Json::array({"--install", _identity["install"]});
    if (!_identity["instance"].get<std::string>().empty()) {
        args.push_back("--instance");
        args.push_back(_identity["instance"]);
    }
    return {
        {
            "mcpServers",
            {
                {
                    "mo2-dev-bench",
                    {
                        {
                            "command",
                            _identity["install"].get<std::string>() + "/plugins/dev_bench/dev-bench-bridge.exe",
                        },
                        {"args", args},
                    },
                },
            },
        },
    };
}
}
