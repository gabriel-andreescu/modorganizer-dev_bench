#pragma once
#include <QObject>
#include <memory>
#include <uibase/iplugin.h>

namespace Bench {
class Host;
}

class DevBenchPlugin final : public QObject, public MOBase::IPlugin {
    Q_OBJECT
    Q_INTERFACES(MOBase::IPlugin)
    Q_PLUGIN_METADATA(IID "org.gabonz.DevBench")
public:
    DevBenchPlugin();
    DevBenchPlugin(const DevBenchPlugin&) = delete;
    DevBenchPlugin(DevBenchPlugin&&) = delete;
    DevBenchPlugin& operator=(const DevBenchPlugin&) = delete;
    DevBenchPlugin& operator=(DevBenchPlugin&&) = delete;
    ~DevBenchPlugin() override;
    bool init(MOBase::IOrganizer* a_organizer) override;
    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString author() const override;
    [[nodiscard]] QString description() const override;
    [[nodiscard]] MOBase::VersionInfo version() const override;
    [[nodiscard]] QList<MOBase::PluginSetting> settings() const override;

private:
    std::unique_ptr<Bench::Host> _host;
};
