#include "ui/TimelineWidget.h"
#include "ui/TimelineRuler.h"
#include "ui/TrackHeaderWidget.h"
#include "ui/ClipItem.h"
#include "core/Project.h"
#include "core/Timeline.h"
#include "core/Track.h"
#include "core/Clip.h"
#include "core/Command.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDebug>
#include <algorithm>

namespace ve {

TimelineWidget::TimelineWidget(Project* project, QUndoStack* undoStack, QWidget* parent)
    : QWidget(parent)
    , project_(project)
    , undoStack_(undoStack)
{
    setupUi();
    connect(project_->timeline(), &Timeline::structureChanged, this, &TimelineWidget::onStructureChanged);
    connect(project_->timeline(), &Timeline::layoutChanged,   this, &TimelineWidget::onStructureChanged);
    onStructureChanged();
}

void TimelineWidget::setupUi() {
    auto* h = new QHBoxLayout(this);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);

    // Left: track headers column (fixed width)
    leftHeader_ = new QWidget(this);
    leftHeader_->setFixedWidth(headerWidth_);
    leftHeaderLayout_ = new QVBoxLayout(leftHeader_);
    leftHeaderLayout_->setContentsMargins(0, 20, 0, 0); // offset for ruler
    leftHeaderLayout_->setSpacing(2);
    leftHeader_->setObjectName("trackHeaderColumn");

    // Right: scrollable area (ruler + tracks)
    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    tracksContainer_ = new QWidget;
    tracksLayout_ = new QVBoxLayout(tracksContainer_);
    tracksLayout_->setContentsMargins(0, 0, 0, 0);
    tracksLayout_->setSpacing(2);

    rightArea_ = new QWidget;
    auto* rightLayoutV = new QVBoxLayout(rightArea_);
    rightLayoutV->setContentsMargins(0, 0, 0, 0);
    rightLayoutV->setSpacing(0);

    ruler_ = new TimelineRuler(this);
    ruler_->setFixedHeight(20);
    rightLayoutV->addWidget(ruler_);
    rightLayoutV->addWidget(tracksContainer_, 1);

    scrollArea_->setWidget(rightArea_);
    scrollArea_->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // Sync vertical scroll of left header column with right area
    connect(scrollArea_->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int v) {
        leftHeader_->move(leftHeader_->x(), -v + 20);
    });

    h->addWidget(leftHeader_);
    h->addWidget(scrollArea_, 1);

    setMinimumHeight(220);
}

void TimelineWidget::setProject(Project* project) {
    if (project_) disconnect(project_->timeline(), nullptr, this, nullptr);
    project_ = project;
    connect(project_->timeline(), &Timeline::structureChanged, this, &TimelineWidget::onStructureChanged);
    connect(project_->timeline(), &Timeline::layoutChanged,   this, &TimelineWidget::onStructureChanged);
    onStructureChanged();
}

void TimelineWidget::onStructureChanged() {
    rebuildTracks();
    ensureTimelineWidthFits();
    update();
}

void TimelineWidget::rebuildTracks() {
    // Clear left headers
    QLayoutItem* item;
    while ((item = leftHeaderLayout_->takeAt(0)) != nullptr) { delete item->widget(); delete item; }
    // Clear right tracks
    while ((item = tracksLayout_->takeAt(0)) != nullptr) { delete item->widget(); delete item; }
    headers_.clear();
    clipItems_.clear();

    for (Track* t : project_->timeline()->tracks()) {
        auto* header = new TrackHeaderWidget(t, leftHeader_);
        header->setFixedHeight(trackHeight_);
        leftHeaderLayout_->addWidget(header);
        headers_[t] = header;

        auto* trackWidget = new QWidget(tracksContainer_);
        trackWidget->setFixedHeight(trackHeight_);
        trackWidget->setStyleSheet("background-color: #1d1f24; border: 1px solid #2a2d33;");
        auto* tl = new QHBoxLayout(trackWidget);
        tl->setContentsMargins(0, 0, 0, 0);
        tl->setSpacing(0);
        tl->addStretch(1);

        for (Clip* c : t->clips()) {
            auto* ci = new ClipItem(c, this, trackWidget);
            ci->setFixedHeight(trackHeight_ - 4);
            int x = timeToX(c->timelineStart());
            int w = std::max(8, timeToX(c->duration()));
            ci->setGeometry(x, 2, w, trackHeight_ - 4);
            ci->show();
            connect(ci, &ClipItem::selected, this, [this, c](Clip* clip) {
                setSelectedClip(clip);
            });
            connect(ci, &ClipItem::moved, this, [this](Clip* clip, double newStart) {
                ensureTimelineWidthFits();
            });
            connect(ci, &ClipItem::trimmed, this, [this](Clip*, double, double) {
                ensureTimelineWidthFits();
            });
            clipItems_[c] = ci;
        }
        tracksLayout_->addWidget(trackWidget);
    }
    tracksLayout_->addStretch(1);
}

void TimelineWidget::ensureTimelineWidthFits() {
    double dur = std::max(60.0, project_->timeline()->duration() + 10.0);
    int w = timeToX(dur);
    rightArea_->setMinimumWidth(w);
    tracksContainer_->setMinimumWidth(w);
}

double TimelineWidget::pixelsPerSecond() const {
    return project_ ? project_->timeline()->pixelsPerSecond() : pps_;
}

void TimelineWidget::setPixelsPerSecond(double pps) {
    pps_ = pps;
    if (project_) project_->timeline()->setPixelsPerSecond(pps);
    // geometry update happens via layoutChanged → onStructureChanged
}

void TimelineWidget::zoomIn()  { setPixelsPerSecond(std::min(500.0, pixelsPerSecond() * 1.25)); }
void TimelineWidget::zoomOut() { setPixelsPerSecond(std::max(5.0,   pixelsPerSecond() / 1.25)); }

int TimelineWidget::timeToX(double t) const {
    return static_cast<int>(t * pixelsPerSecond());
}

double TimelineWidget::xToTime(int x) const {
    return x / pixelsPerSecond();
}

double TimelineWidget::snap(double t) const {
    if (!project_) return t;
    return project_->timeline()->snap(t, snapTolerance(), playhead_);
}

void TimelineWidget::setSelectedClip(Clip* clip) {
    selectedClip_ = clip;
    emit clipSelected(clip);
    update();
}

void TimelineWidget::setPlayhead(double t) {
    playhead_ = t;
    emit playheadChanged(t);
    ruler_->setPlayhead(t);
}

void TimelineWidget::deleteSelectedClip() {
    if (!selectedClip_) return;
    Clip* clip = selectedClip_;
    selectedClip_ = nullptr;
    emit clipSelected(nullptr);

    // Find owning track and remember state for undo
    Track* owningTrack = nullptr;
    for (Track* t : project_->timeline()->tracks()) {
        if (t->clips().contains(clip)) { owningTrack = t; break; }
    }
    if (!owningTrack) return;

    double oldStart = clip->timelineStart();
    int oldIndex = owningTrack->clips().indexOf(clip);

    undoStack_->push(new LambdaCommand(
        "Delete clip",
        [this, owningTrack, clip]() { owningTrack->removeClip(clip); },
        [this, owningTrack, clip, oldStart, oldIndex]() {
            owningTrack->insertClip(oldIndex, clip);
            clip->setTimelineStart(oldStart);
        }
    ));
}

} // namespace ve
