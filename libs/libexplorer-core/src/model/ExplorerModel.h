#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QMap>
#include <memory>

namespace explorer::core {

// 基础模型项
struct ModelItem {
    QString id;
    QString name;
    QString icon;
    QVariant data;
    QMap<QString, QVariant> roles;
    QVector<std::shared_ptr<ModelItem>> children;
    std::weak_ptr<ModelItem> parent;

    ModelItem() = default;
    ModelItem(const QString& id, const QString& name) : id(id), name(name) {}
};

// 抽象模型接口
class IModel : public QObject {
    Q_OBJECT
public:
    explicit IModel(QObject* parent = nullptr) : QObject(parent) {}
    ~IModel() override = default;

    virtual std::shared_ptr<ModelItem> rootItem() const = 0;
    virtual std::shared_ptr<ModelItem> findItem(const QString& id) const = 0;
    virtual bool addItem(std::shared_ptr<ModelItem> parent, std::shared_ptr<ModelItem> item) = 0;
    virtual bool removeItem(const QString& id) = 0;
    virtual void clear() = 0;

signals:
    void itemAdded(std::shared_ptr<ModelItem> parent, std::shared_ptr<ModelItem> item);
    void itemRemoved(std::shared_ptr<ModelItem> item);
    void itemChanged(std::shared_ptr<ModelItem> item, const QString& role);
    void modelReset();
};

// 探索器主模型 - 管理桌面、文件系统、应用等层级数据
class ExplorerModel : public IModel {
    Q_OBJECT
public:
    static ExplorerModel& instance();

    ExplorerModel(const ExplorerModel&) = delete;
    ExplorerModel& operator=(const ExplorerModel&) = delete;

    std::shared_ptr<ModelItem> rootItem() const override;
    std::shared_ptr<ModelItem> findItem(const QString& id) const override;
    bool addItem(std::shared_ptr<ModelItem> parent, std::shared_ptr<ModelItem> item) override;
    bool removeItem(const QString& id) override;
    void clear() override;

    // 预定义根分类
    enum RootCategory {
        Desktop,
        Computer,
        Network,
        ControlPanel,
        Applications,
        Custom
    };

    std::shared_ptr<ModelItem> getRootCategory(RootCategory category) const;
    std::shared_ptr<ModelItem> createCategory(RootCategory category, const QString& name, const QString& icon = {});

    // 刷新
    void refresh();
    void refreshCategory(RootCategory category);

signals:
    void categoryRefreshed(RootCategory category);

private:
    ExplorerModel();
    ~ExplorerModel() override;

    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace explorer::core