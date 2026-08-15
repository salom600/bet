#include "ui/monitor/MonitorManager.h"
#include "ui/bin/ClipMonitorWidget.h"
#include "ui/monitor/ProjectMonitor.h"
#include "model/ClipModel.h"
#include "model/BinClip.h"

namespace ve {

MonitorManager::MonitorManager(ClipMonitorWidget* clip, ProjectMonitor* project, QObject* parent)
    : QObject(parent)
    , clipMonitor_(clip)
    , projectMonitor_(project)
{
}

void MonitorManager::onTimelineSelectionChanged(ClipModel* clip) {
    // When a clip is selected on the timeline, also load it in the Clip Monitor
    // for source scrubbing (Kdenlive behavior).
    if (clip && clip->binClip()) {
        clipMonitor_->loadBinClip(clip->binClip()->id());
    }
}

} // namespace ve
