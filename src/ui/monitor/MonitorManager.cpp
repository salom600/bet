#include "ui/monitor/MonitorManager.h"
#include "ui/bin/ClipMonitorWidget.h"
#include "ui/monitor/ProjectMonitor.h"

namespace ve {

MonitorManager::MonitorManager(ClipMonitorWidget* clip, ProjectMonitor* project, QObject* parent)
    : QObject(parent)
    , clipMonitor_(clip)
    , projectMonitor_(project)
{
}

void MonitorManager::onTimelineSelectionChanged(int clipId) {
    Q_UNUSED(clipId);
    // For now: switching focus is automatic via user interaction.
    // A future improvement: when a clip is selected on the timeline, also
    // load it in the Clip Monitor for source scrubbing.
}

} // namespace ve
