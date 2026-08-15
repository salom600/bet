#pragma once

#include <QWidget>
#include <QUndoStack>
#include <QVBoxLayout>
#include <QScrollArea>
#include <unordered_map>
#include "definitions.h"

namespace ve {

class Project;
class TimelineModel;
class ClipModel;
class TimelineRuler;
class TrackHeadWidget;
class ClipItem;

class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    TimelineWidget(Project* project, QWidget* parent = nullptr);
    void setProject(Project* project);

    double pixelsPerSecond() const;
    void setPixelsPerSecond(double pps);

    void zoomIn();
    void zoomOut();

    int timeToX(double t) const;
    double xToTime(int x) const;
    double snap(double t) const;
    double snapTolerance() const { return 8.0 / pixelsPerSecond(); }

    Project* project() const { return project_; }
    QUndoStack* undoStack() const;

public slots:
    void setPlayhead(double t);
    void deleteSelectedClip();

signals:
    void clipSelected(ClipModel* clip);
    void playheadChanged(double t);

protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

private slots:
    void onStructureChanged();

private:
    void setupUi();
    void rebuildTracks();
    void ensureTimelineWidthFits();
    ClipModel* tl_model_clip(int cid);

    Project* project_ = nullptr;

    double pps_ = 50.0;
    int    headerWidth_ = 150;
    int    trackHeight_ = 80;

    QWidget*      leftHeader_       = nullptr;
    QVBoxLayout*  leftHeaderLayout_ = nullptr;
    QWidget*      rightArea_        = nullptr;
    QVBoxLayout*  rightLayout_      = nullptr;
    TimelineRuler* ruler_           = nullptr;
    QScrollArea*  scrollArea_       = nullptr;
    QWidget*      tracksContainer_  = nullptr;
    QVBoxLayout*  tracksLayout_     = nullptr;
};

} // namespace ve
