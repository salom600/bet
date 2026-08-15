#include "media/ThumbnailCache.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDebug>

namespace ve {

ThumbnailCache& ThumbnailCache::self() {
    static ThumbnailCache instance;
    return instance;
}

ThumbnailCache::ThumbnailCache() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (base.isEmpty()) {
        base = QDir::homePath() + "/.cache/VideoEditor";
    }
    QDir().mkpath(base);
}

QString ThumbnailCache::cacheDir() const {
    QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (base.isEmpty()) base = QDir::homePath() + "/.cache/VideoEditor";
    return base + "/thumbnails";
}

QString ThumbnailCache::hashFor(const QString& sourcePath) {
    QFileInfo fi(sourcePath);
    QString key = sourcePath + "|" + QString::number(fi.size()) + "|" +
                  QString::number(fi.lastModified().toMSecsSinceEpoch());
    return QString::fromLatin1(
        QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex());
}

QImage ThumbnailCache::get(const QString& sourcePath, double atSeconds) {
    QMutexLocker locker(&m_mutex);
    QString memKey = sourcePath + "|" + QString::number(atSeconds, 'f', 3);
    if (m_memCache.contains(memKey)) return m_memCache.value(memKey);

    QString hash = hashFor(sourcePath);
    QString path = cacheDir() + "/" + hash + "_" +
                   QString::number(static_cast<int>(atSeconds * 1000)) + ".png";
    QFileInfo fi(path);
    if (fi.exists()) {
        QImage img(path);
        if (!img.isNull()) {
            m_memCache.insert(memKey, img);
            return img;
        }
    }
    return QImage();
}

void ThumbnailCache::put(const QString& sourcePath, double atSeconds, const QImage& img) {
    if (img.isNull()) return;
    QMutexLocker locker(&m_mutex);
    QString memKey = sourcePath + "|" + QString::number(atSeconds, 'f', 3);
    m_memCache.insert(memKey, img);

    QDir().mkpath(cacheDir());
    QString hash = hashFor(sourcePath);
    QString path = cacheDir() + "/" + hash + "_" +
                   QString::number(static_cast<int>(atSeconds * 1000)) + ".png";
    img.save(path, "PNG");
}

} // namespace ve
