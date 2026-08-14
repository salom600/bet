/*
 * VideoEditor - BinModel.h
 * Tree of BinFolder + BinClip, with ID-based registry.
 *
 * Adapted from Kdenlive's src/bin/projectitemmodel.h + bin.h.
 *
 * The Bin is the project's library of source media. It is logically a tree
 * (folders can contain clips and sub-folders). Every bin item (folder or
 * clip) has a unique string ID. The model exposes a QAbstractItemModel
 * interface so QTreeView can render it directly.
 */
#pragma once

#include "../definitions.h"
#include <QAbstractItemModel>
#include <QHash>
#include <QMap>
#include <memory>

namespace ve {

class BinClip;
class BinFolder;

class BinModel : public QAbstractItemModel {
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        ThumbnailRole = Qt::DecorationRole,
        TooltipRole = Qt::ToolTipRole,
        IdRole = Qt::UserRole + 1,
        TypeRole,
        DurationRole,
        PathRole,
    };

    explicit BinModel(QObject* parent = nullptr);

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    // Public API
    /// Add a media file to the bin. Returns the new BinClip's id (or existing).
    QString addClip(const QString& sourcePath, const QString& folderId = {});

    /// Create a folder. Returns its id.
    QString addFolder(const QString& name, const QString& parentId = {});

    /// Look up a clip by id.
    std::shared_ptr<BinClip> clip(const QString& id) const;

    /// Look up a folder by id.
    std::shared_ptr<BinFolder> folder(const QString& id) const;

    /// Remove an item (clip or folder) by id. Returns true on success.
    bool removeItem(const QString& id);

    /// All clips in the bin (depth-first).
    QList<std::shared_ptr<BinClip>> allClips() const;

    /// Root folder id.
    QString rootFolderId() const;

signals:
    void clipAdded(const QString& clipId);
    void clipRemoved(const QString& clipId);
    void structureChanged();

private:
    struct Item;
    std::shared_ptr<BinFolder> root_;
    QHash<QString, std::shared_ptr<BinClip>>   clips_;
    QHash<QString, std::shared_ptr<BinFolder>> folders_;

    int nextFolderNum_ = 1;
    int nextClipNum_   = 1;

    QString newClipId();
    QString newFolderId();
    QModelIndex indexForFolder(const std::shared_ptr<BinFolder>& f) const;
    void notifyFolderChanged(const std::shared_ptr<BinFolder>& f);
};

} // namespace ve
