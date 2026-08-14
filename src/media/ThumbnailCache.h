/*
 * VideoEditor - ThumbnailCache.h
 * Disk-backed thumbnail cache.
 *
 * Adapted from Kdenlive's src/utils/thumbnailcache.hpp.
 *
 * Thumbnails are keyed by a hash of the source file path + mtime, and stored
 * as PNG files under the user's cache directory. This makes thumbnails
 * survive across editor launches and avoids re-decoding them.
 */
#pragma once

#include <QImage>
#include <QString>
#include <QHash>
#include <QMutex>

namespace ve {

class ThumbnailCache {
public:
    static ThumbnailCache& self();

    /// Returns cached thumbnail for the given source file (or null QImage).
    QImage get(const QString& sourcePath, double atSeconds = 0.5);

    /// Stores a thumbnail for the given source file.
    void put(const QString& sourcePath, double atSeconds, const QImage& img);

    /// Computes a stable hash for the given file (path + size + mtime).
    static QString hashFor(const QString& sourcePath);

private:
    ThumbnailCache();
    QString cacheDir() const;

    QHash<QString, QImage> m_memCache;
    QMutex m_mutex;
};

} // namespace ve
