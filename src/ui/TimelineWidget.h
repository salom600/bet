#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <unordered_map>
#include "core/Project.h"

class QUndoStack;
class QVBoxLayout;
class QScrollBar;

namespace ve {

class ClipItem;
class Track;
class Clip;
class TimelineRuler;
class TrackHeaderWidget;

/// Composite widget that owns:
///  - left column of track headers (fixed width)
///  - right scrollable area containing the ruler + tracks + clips
class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    TimelineWidget(Project* project, QUndoStack* undoStack, QWidget* parent = nullptr);

    void setProject(Project* project);

    double pixelsPerSecond() const;
    void setPixelsPerSecond(double pps);

    void zoomIn();
    void zoomOut();

    /// Compute x coordinate for a given timeline time (seconds).
    int timeToX(double t) const;
    double xToTime(int x) const;

    /// Snap a time value to the nearest clip edge / playhead within tolerance.
    double snap(double t) const;

    /// Snap utility used by ClipItem while dragging.
    double snapTolerance() const { return 8.0 / pixelsPerSecond(); }

    Project* project() const { return project_; }
    QUndoStack* undoStack() const { return undoStack_; }

    void setSelectedClip(Clip* clip);
    Clip* selectedClip() const { return selectedClip_; }

    double playhead() const { return playhead_; }

public slots:
    void setPlayhead(double t);
    void deleteSelectedClip();

signals:
    void clipSelected(Clip* clip);
    void playheadChanged(double t);

private slots:
    void rebuildTracks();
    void onStructureChanged();

private:
    void setupUi();
    void ensureTimelineWidthFits();

    Project*    project_;
    QUndoStack* undoStack_;

    Clip* selectedClip_ = nullptr;
    double playhead_ = 0.0;
    double pps_ = 50.0;
    int    headerWidth_ = 130;
    int    trackHeight_ = 70;

    // Sub-widgets
    QWidget*           leftHeader_     = nullptr;
    QVBoxLayout*       leftHeaderLayout_ = nullptr;
    QWidget*           rightArea_      = nullptr;
    QVBoxLayout*       rightLayout_    = nullptr;
    TimelineRuler*     ruler_          = nullptr;
    QScrollArea*       scrollArea_     = nullptr;
    QWidget*           tracksContainer_ = nullptr;
    QVBoxLayout*       tracksLayout_   = nullptr;

    std::unordered_map<Track*, TrackHeaderWidget*> headers_;
    std::unordered_map<Clip*, ClipItem*>            clipItems_;
};

} // namespace ve
