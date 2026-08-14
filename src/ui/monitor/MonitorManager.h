#pragma once

#include <QObject>

namespace ve {

class ClipMonitorWidget;
class ProjectMonitor;

/// Manages focus / sync between the two monitors (Kdenlive-style).
class MonitorManager : public QObject {
    Q_OBJECT
public:
    MonitorManager(ClipMonitorWidget* clip, ProjectMonitor* project, QObject* parent = nullptr);

public slots:
    void onTimelineSelectionChanged(int clipId);

private:
    ClipMonitorWidget* clipMonitor_;
    ProjectMonitor*    projectMonitor_;
};

} // namespace ve
