/*
 * VideoEditor - BinModel.cpp
 */
#include "model/BinModel.h"
#include "model/BinClip.h"
#include "model/BinFolder.h"
#include "media/MediaBackend.h"

#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QDebug>

namespace ve {

BinModel::BinModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    root_ = std::make_shared<BinFolder>("root", "Bin");
    folders_.insert(root_->id(), root_);
}

// ---------------------------------------------------------------------------
// QAbstractItemModel interface
// ---------------------------------------------------------------------------
QModelIndex BinModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) return {};
    std::shared_ptr<BinFolder> parentFolder;
    if (!parent.isValid()) {
        parentFolder = root_;
    } else {
        QString id = parent.data(IdRole).toString();
        parentFolder = folders_.value(id);
        if (!parentFolder) return {};
    }
    if (row < parentFolder->folders().size()) {
        auto f = parentFolder->folders().at(row);
        return createIndex(row, column, f.get());
    }
    int clipRow = row - parentFolder->folders().size();
    if (clipRow >= 0 && clipRow < parentFolder->clips().size()) {
        auto c = parentFolder->clips().at(clipRow);
        return createIndex(row, column, c.get());
    }
    return {};
}

QModelIndex BinModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) return {};
    // Try clip first
    auto* clipPtr = static_cast<BinClip*>(child.internalPointer());
    auto* folderPtr = static_cast<BinFolder*>(child.internalPointer());

    std::shared_ptr<BinFolder> parent;
    // We need to find which item this internalPointer refers to.
    // Walk the tree to find the parent.
    QString childId;
    bool isFolder = false;
    for (const auto& f : folders_) {
        if (f.get() == folderPtr) { childId = f->id(); isFolder = true; break; }
    }
    if (childId.isEmpty()) {
        for (const auto& c : clips_) {
            if (c.get() == clipPtr) { childId = c->id(); break; }
        }
    }
    if (childId.isEmpty()) return {};

    if (isFolder) {
        auto f = folders_.value(childId);
        if (!f) return {};
        parent = f->parent();
    } else {
        // For a clip, we need to find which folder contains it.
        for (const auto& f : folders_) {
            for (const auto& c : f->clips()) {
                if (c->id() == childId) { parent = f; break; }
            }
            if (parent) break;
        }
    }
    if (!parent || parent == root_) return {};
    auto grand = parent->parent();
    if (!grand) return {};
    int row = grand->folders().indexOf(parent);
    return createIndex(row, 0, parent.get());
}

int BinModel::rowCount(const QModelIndex& parent) const {
    if (!parent.isValid()) return root_->totalChildren();
    QString id = parent.data(IdRole).toString();
    auto f = folders_.value(id);
    if (f) return f->totalChildren();
    return 0; // clips have no children
}

int BinModel::columnCount(const QModelIndex&) const { return 1; }

QVariant BinModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    auto* ptr = index.internalPointer();
    // Check if it's a folder
    for (const auto& f : folders_) {
        if (f.get() == ptr) {
            switch (role) {
                case NameRole:      return f->name();
                case IdRole:        return f->id();
                case TypeRole:      return QStringLiteral("folder");
                case TooltipRole:   return QStringLiteral("Folder: ") + f->name();
                default:            return {};
            }
        }
    }
    // Else a clip
    for (const auto& c : clips_) {
        if (c.get() == ptr) {
            switch (role) {
                case NameRole:      return c->name();
                case IdRole:        return c->id();
                case TypeRole:      return QStringLiteral("clip");
                case PathRole:      return c->sourcePath();
                case DurationRole:  return c->duration();
                case ThumbnailRole: {
                    QImage t = c->thumbnail();
                    return t.isNull() ? QVariant() : QVariant(t);
                }
                case TooltipRole:   return c->sourcePath();
                default:            return {};
            }
        }
    }
    return {};
}

Qt::ItemFlags BinModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    auto flags = QAbstractItemModel::flags(index) | Qt::ItemIsDragEnabled;
    QString type = index.data(TypeRole).toString();
    if (type == "folder") flags |= Qt::ItemIsDropEnabled;
    return flags;
}

QStringList BinModel::mimeTypes() const {
    return { "application/x-ve-binclip" };
}

QMimeData* BinModel::mimeData(const QModelIndexList& indexes) const {
    auto* mime = new QMimeData;
    QStringList ids;
    for (const QModelIndex& ix : indexes) {
        QString id = ix.data(IdRole).toString();
        QString type = ix.data(TypeRole).toString();
        if (type == "clip" && !id.isEmpty()) ids << id;
    }
    mime->setData("application/x-ve-binclip", ids.join(",").toUtf8());
    return mime;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
QString BinModel::newClipId()   { return QString("clip%1").arg(nextClipNum_++); }
QString BinModel::newFolderId() { return QString("folder%1").arg(nextFolderNum_++); }
QString BinModel::rootFolderId() const { return root_->id(); }

QString BinModel::addClip(const QString& sourcePath, const QString& folderId) {
    // Don't re-add the same file
    for (const auto& c : clips_) {
        if (c->sourcePath() == sourcePath) return c->id();
    }
    // Probe the file
    auto backend = createDefaultBackend();
    MediaInfo info = backend->probe(sourcePath);
    ClipType type = ClipType::Unknown;
    if (info.hasVideo && info.hasAudio)      type = ClipType::AV;
    else if (info.hasVideo)                  type = ClipType::Video;
    else if (info.hasAudio)                  type = ClipType::Audio;
    else if (sourcePath.section('.', -1).toLower() == "png" ||
             sourcePath.section('.', -1).toLower() == "jpg" ||
             sourcePath.section('.', -1).toLower() == "jpeg" ||
             sourcePath.section('.', -1).toLower() == "bmp" ||
             sourcePath.section('.', -1).toLower() == "webp") {
        type = ClipType::Image;
        // For images we still want a usable duration
        if (info.duration <= 0) info.duration = 5.0;
    }

    auto clip = std::make_shared<BinClip>(newClipId(), sourcePath, type, info);
    clips_.insert(clip->id(), clip);

    auto parent = folderId.isEmpty() ? root_ : folders_.value(folderId, root_);
    int row = parent->totalChildren();
    beginInsertRows(indexForFolder(parent), row, row);
    parent->addClip(clip);
    endInsertRows();
    emit clipAdded(clip->id());
    emit structureChanged();
    return clip->id();
}

QString BinModel::addFolder(const QString& name, const QString& parentId) {
    auto folder = std::make_shared<BinFolder>(newFolderId(),
        name.isEmpty() ? "New Folder" : name);
    folders_.insert(folder->id(), folder);

    auto parent = parentId.isEmpty() ? root_ : folders_.value(parentId, root_);
    int row = parent->totalChildren();
    beginInsertRows(indexForFolder(parent), row, row);
    parent->addFolder(folder);
    endInsertRows();
    emit structureChanged();
    return folder->id();
}

std::shared_ptr<BinClip> BinModel::clip(const QString& id) const {
    return clips_.value(id);
}

std::shared_ptr<BinFolder> BinModel::folder(const QString& id) const {
    return folders_.value(id);
}

bool BinModel::removeItem(const QString& id) {
    if (clips_.contains(id)) {
        auto clip = clips_.take(id);
        // Find parent folder
        for (const auto& f : folders_) {
            if (f->clips().contains(clip)) {
                int row = f->folders().size() + f->clips().indexOf(clip);
                beginRemoveRows(indexForFolder(f), row, row);
                f->removeClip(id);
                endRemoveRows();
                break;
            }
        }
        emit clipRemoved(id);
        emit structureChanged();
        return true;
    }
    if (folders_.contains(id) && id != root_->id()) {
        auto folder = folders_.take(id);
        auto parent = folder->parent();
        if (parent) {
            int row = parent->folders().indexOf(folder);
            beginRemoveRows(indexForFolder(parent), row, row);
            parent->removeFolder(id);
            endRemoveRows();
        }
        emit structureChanged();
        return true;
    }
    return false;
}

QList<std::shared_ptr<BinClip>> BinModel::allClips() const {
    return clips_.values();
}

QModelIndex BinModel::indexForFolder(const std::shared_ptr<BinFolder>& f) const {
    if (!f || f == root_) return {};
    auto parent = f->parent();
    if (!parent) return {};
    int row = parent->folders().indexOf(f);
    if (row < 0) return {};
    return createIndex(row, 0, f.get());
}

void BinModel::notifyFolderChanged(const std::shared_ptr<BinFolder>& f) {
    QModelIndex ix = indexForFolder(f);
    if (ix.isValid()) emit dataChanged(ix, ix);
}

} // namespace ve
