#pragma once

#include <QObject>
#include <QString>
#include <QSize>
#include "core/Timeline.h"

namespace ve {

/// Project state: timeline + render settings + project file path.
class Project : public QObject {
    Q_OBJECT
public:
    explicit Project(QObject* parent = nullptr);

    Timeline* timeline() { return timeline_; }

    // File path of the .veproj on disk
    const QString& filePath() const { return filePath_; }
    void setFilePath(const QString& p) { filePath_ = p; emit dirtied(); }

    bool isDirty() const { return dirty_; }
    void setDirty(bool v) { dirty_ = v; emit dirtied(); }

    // Render / export settings
    int exportWidth()  const { return exportWidth_; }
    int exportHeight() const { return exportHeight_; }
    int exportFps()    const { return exportFps_; }
    int exportBitrateKbps() const { return exportBitrateKbps_; }
    QString exportFormat() const { return exportFormat_; }

    void setExportWidth(int v)  { exportWidth_  = v; emit dirtied(); }
    void setExportHeight(int v) { exportHeight_ = v; emit dirtied(); }
    void setExportFps(int v)    { exportFps_    = v; emit dirtied(); }
    void setExportBitrateKbps(int v) { exportBitrateKbps_ = v; emit dirtied(); }
    void setExportFormat(const QString& v) { exportFormat_ = v; emit dirtied(); }

signals:
    void dirtied();

private:
    Timeline* timeline_;
    QString   filePath_;
    bool      dirty_ = false;

    int     exportWidth_       = 1920;
    int     exportHeight_      = 1080;
    int     exportFps_         = 30;
    int     exportBitrateKbps_ = 8000;
    QString exportFormat_      = "mp4";
};

} // namespace ve
