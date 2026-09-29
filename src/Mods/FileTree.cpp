#include "Mods/FileTree.h"
#include "Json.h"
#include "Mods/InfoAccess.h"
#include "Native/TreeMenu.h"
#include "Tools/Registry.h"
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QItemSelectionModel>
#include <QObject>
#include <QString>
#include <QTreeView>
#include <algorithm>
#include <qnamespace.h>

namespace Bench {
namespace {
    QTreeView* FileTree() {
        const auto* dialog = RequireModInfo();
        SelectModInfoTab(dialog, "tabFiles");
        return dialog->findChild<QTreeView*>("filetree");
    }

    QString ModPath(const QDir& a_root, const QString& a_path) {
        const auto relative = QDir::cleanPath(QDir::fromNativeSeparators(a_path));
        if (QDir::isAbsolutePath(relative) || relative == ".." || relative.startsWith("../")) {
            throw ToolError(400, "Use a path relative to the mod directory");
        }
        return a_root.absoluteFilePath(relative);
    }

    bool HiddenPath(const QString& a_path) {
        return std::ranges::any_of(a_path.split('/'), [](const QString& a_component) {
            return a_component.endsWith(".mohidden", Qt::CaseInsensitive);
        });
    }

    void SelectFiles(const QTreeView* a_tree, const Json& a_paths) {
        const auto* model = qobject_cast<QFileSystemModel*>(a_tree->model());
        const QDir root(model->rootPath());
        QItemSelection selection;
        for (const auto& path : a_paths) {
            const auto absolute = ModPath(root, Text(path));
            const auto index = model->index(absolute);
            if (!index.isValid() || QDir::cleanPath(absolute) == QDir::cleanPath(root.absolutePath())) {
                throw ToolError(404, "Unknown mod file or directory: " + path.get<std::string>());
            }
            selection.select(index, index);
        }
        a_tree->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
}

Json ReadModDirectory(const Json& a_arguments) {
    const auto* tree = FileTree();
    const auto* model = qobject_cast<QFileSystemModel*>(tree->model());
    const QDir root(model->rootPath());
    const QDir directory(ModPath(root, Text(a_arguments.value("path", Json("")))));
    if (!directory.exists()) {
        throw ToolError(404, "Mod directory does not exist");
    }
    const auto entries = directory.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase
    );
    const auto offset = a_arguments.value("offset", 0);
    const auto limit = a_arguments.value("limit", 100);
    const auto total = static_cast<int>(entries.size());
    const auto end = offset >= total ? total : offset + std::min(limit, total - offset);
    Json files = Json::array();
    for (int index = offset; index < end; ++index) {
        const auto& entry = entries[index];
        const auto relative = root.relativeFilePath(entry.absoluteFilePath());
        files.push_back({
            {"path", Utf8(relative)},
            {"directory", entry.isDir()},
            {"hidden", HiddenPath(relative)},
            {"size", entry.size()},
        });
    }
    return {
        {"name", Utf8(RequireModInfo()->windowTitle())},
        {"root", Utf8(root.absolutePath())},
        {"path", Utf8(root.relativeFilePath(directory.absolutePath()))},
        {"files", files},
        {"total", total},
        {"offset", offset},
        {"nextOffset", end < total ? Json(end) : Json(nullptr)},
    };
}

Json ModFileVisibility(const Json& a_arguments) {
    auto* tree = FileTree();
    SelectFiles(tree, a_arguments.at("paths"));
    const bool hide = a_arguments.at("action") == "hideFiles";
    const auto label = QCoreApplication::translate("FileTreeTab", hide ? "&Hide" : "&Unhide");
    InvokeTreeMenu(tree, label);
    return ReadModDirectory(a_arguments);
}
}
