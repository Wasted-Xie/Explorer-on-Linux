#include "ExplorerModel.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace explorer::core {

class ExplorerModel::Impl {
public:
    Impl() {
        root = std::make_shared<ModelItem>("root", "Root");
        initializeCategories();
    }

    void initializeCategories() {
        const struct { RootCategory cat; QString id; QString name; QString icon; } categories[] = {
            {Desktop, "desktop", "桌面", "user-desktop"},
            {Computer, "computer", "此电脑", "computer"},
            {Network, "network", "网络", "network-workgroup"},
            {ControlPanel, "controlpanel", "控制面板", "preferences-system"},
            {Applications, "applications", "应用程序", "applications-other"}
        };

        for (auto& cat : categories) {
            auto item = std::make_shared<ModelItem>(cat.id, cat.name);
            item->icon = cat.icon;
            item->data = QVariant::fromValue(static_cast<int>(cat.cat));
            item->parent = root;
            root->children.append(item);
            categoryItems[cat.cat] = item;
            idIndex[cat.id] = item;
        }
    }

    std::shared_ptr<ModelItem> root;
    QMap<RootCategory, std::shared_ptr<ModelItem>> categoryItems;
    QMap<QString, std::shared_ptr<ModelItem>> idIndex;
};

ExplorerModel& ExplorerModel::instance() {
    static ExplorerModel inst;
    return inst;
}

ExplorerModel::ExplorerModel() : d(std::make_unique<Impl>()) {}
ExplorerModel::~ExplorerModel() = default;

std::shared_ptr<ModelItem> ExplorerModel::rootItem() const {
    return d->root;
}

std::shared_ptr<ModelItem> ExplorerModel::findItem(const QString& id) const {
    auto it = d->idIndex.find(id);
    return it != d->idIndex.end() ? *it : nullptr;
}

bool ExplorerModel::addItem(std::shared_ptr<ModelItem> parent, std::shared_ptr<ModelItem> item) {
    if (!parent || !item) return false;

    item->parent = parent;
    parent->children.append(item);

    if (!item->id.isEmpty()) {
        d->idIndex[item->id] = item;
    }

    emit itemAdded(parent, item);
    return true;
}

bool ExplorerModel::removeItem(const QString& id) {
    auto it = d->idIndex.find(id);
    if (it == d->idIndex.end()) return false;

    auto item = *it;
    auto parent = item->parent.lock();
    if (parent) {
        parent->children.removeOne(item);
    }

    d->idIndex.erase(it);
    emit itemRemoved(item);
    return true;
}

void ExplorerModel::clear() {
    d->root->children.clear();
    d->idIndex.clear();
    d->categoryItems.clear();
    d->initializeCategories();
    emit modelReset();
}

std::shared_ptr<ModelItem> ExplorerModel::getRootCategory(RootCategory category) const {
    auto it = d->categoryItems.find(category);
    return it != d->categoryItems.end() ? *it : nullptr;
}

std::shared_ptr<ModelItem> ExplorerModel::createCategory(RootCategory category, const QString& name, const QString& icon) {
    auto item = std::make_shared<ModelItem>(QString(), name);
    item->icon = icon;
    item->data = QVariant::fromValue(static_cast<int>(category));
    return item;
}

void ExplorerModel::refresh() {
    for (int i = 0; i <= static_cast<int>(Applications); ++i) {
        refreshCategory(static_cast<RootCategory>(i));
    }
}

void ExplorerModel::refreshCategory(RootCategory category) {
    // 子类实现具体刷新逻辑
    emit categoryRefreshed(category);
}

} // namespace explorer::core