/*
 * VideoEditor - BinFolder.h
 * A folder in the bin tree.
 */
#pragma once

#include "../definitions.h"
#include <QString>
#include <QList>
#include <memory>

namespace ve {

class BinClip;
class BinFolder;

class BinFolder {
public:
    BinFolder(QString id, QString name, std::shared_ptr<BinFolder> parent = nullptr)
        : id_(std::move(id)), name_(std::move(name)), parent_(parent) {}

    const QString& id()   const { return id_; }
    QString        name() const { return name_; }
    void           setName(const QString& n) { name_ = n; }

    std::shared_ptr<BinFolder> parent() const { return parent_.lock(); }

    const QList<std::shared_ptr<BinFolder>>& folders() const { return folders_; }
    const QList<std::shared_ptr<BinClip>>&   clips()   const { return clips_; }

    void addFolder(std::shared_ptr<BinFolder> f) { folders_.append(f); }
    void addClip(std::shared_ptr<BinClip> c)     { clips_.append(c); }
    bool removeFolder(const QString& id);
    bool removeClip(const QString& id);

    int totalChildren() const { return folders_.size() + clips_.size(); }

private:
    QString id_;
    QString name_;
    std::weak_ptr<BinFolder> parent_;
    QList<std::shared_ptr<BinFolder>> folders_;
    QList<std::shared_ptr<BinClip>>   clips_;
};

} // namespace ve
