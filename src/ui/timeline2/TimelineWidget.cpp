#include "ui/timeline2/TimelineWidget.h"
#include "ui/timeline2/TimelineRuler.h"
#include "ui/timeline2/TrackHeadWidget.h"
#include "ui/timeline2/ClipItem.h"
#include "project/Project.h"
#include "model/TimelineModel.h"
#include "model/TrackModel.h"
#include "model/ClipModel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QPainter>
#include <QDebug>
#include <algorithm>

namespace ve {

TimelineWidget::TimelineWidget(Project* project, QWidget* parent)
    : QWidget(parent)
    , project_(project)
{
    setupUi();
    connect(project_->timeline().get(), &TimelineModel::structureChanged,
            this, [this]() { onStructureChanged(); });
    onStructureChanged();
}

void TimelineWidget::setupUi() {
    auto* h = new QHBoxLayout(this);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);

    leftHeader_ = new QWidget(this);
    leftHeader_->setFixedWidth(headerWidth_);
    leftHeaderLayout_ = new QVBoxLayout(leftHeader_);
    leftHeaderLayout_->setContentsMargins(0, 20, 0, 0);
    leftHeaderLayout_->setSpacing(2);

    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea_->setAcceptDrops(true);

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

    // Accept drops from the bin (mime type application/x-ve-binclip)
    scrollArea_->setAcceptDrops(true);
    tracksContainer_->setAcceptDrops(true);
    rightArea_->setAcceptDrops(true);

    h->addWidget(leftHeader_);
    h->addWidget(scrollArea_, 1);
    setMinimumHeight(220);
}

void TimelineWidget::setProject(Project* project) {
    if (project_) {
        disconnect(project_->timeline().get(), nullptr, this, nullptr);
    }
    project_ = project;
    connect(project_->timeline().get(), &TimelineModel::structureChanged,
            this, [this]() { onStructureChanged(); });
    onStructureChanged();
}

void TimelineWidget::onStructureChanged() {
    rebuildTracks();
    ensureTimelineWidthFits();
    update();
}

void TimelineWidget::rebuildTracks() {
    QLayoutItem* item;
    while ((item = leftHeaderLayout_->takeAt(0)) != nullptr) { delete item->widget(); delete item; }
    while ((item = tracksLayout_->takeAt(0)) != nullptr) { delete item->widget(); delete item; }

    auto tl = project_->timeline();
    for (ObjectId tid : tl->trackIds()) {
        auto t = tl->track(tid);
        if (!t) continue;

        auto* header = new TrackHeadWidget(t.get(), leftHeader_);
        header->setFixedHeight(trackHeight_);
        leftHeaderLayout_->addWidget(header);

        auto* trackWidget = new QWidget(tracksContainer_);
        trackWidget->setFixedHeight(trackHeight_);
        trackWidget->setStyleSheet("background-color: #1d1f24; border: 1px solid #2a2d33;");
        trackWidget->setAcceptDrops(true);

        auto* clipLayout = new QHBoxLayout(trackWidget);
        clipLayout->setContentsMargins(0, 0, 0, 0);
        clipLayout->setSpacing(0);
        clipLayout->addStretch(1);

        for (ObjectId cid : t->clipsSorted()) {
            auto c = tl_model_clip(cid);
            if (!c) continue;
            auto* ci = new ClipItem(c, this, trackWidget);
            ci->setFixedHeight(trackHeight_ - 4);
            int x = timeToX(project_->timeline()->framesToSeconds(c->getPosition()));
            int w = std::max(8, timeToX(project_->timeline()->framesToSeconds(c->getPlaytime())));
            ci->setGeometry(x, 2, w, trackHeight_ - 4);
            ci->show();
            connect(ci, &ClipItem::selected, this, [this](int clipId) {
                emit clipSelected(clipId);
            });
        }
        tracksLayout_->addWidget(trackWidget);
    }
    tracksLayout_->addStretch(1);
}

// Helper to avoid variable shadowing of `tl`
ClipModel* TimelineWidget::tl_model_clip(ObjectId cid) {
    return project_->timeline()->clip(cid).get();
}

void TimelineWidget::ensureTimelineWidthFits() {
    auto tl = project_->timeline();
    double dur = std::max(60.0, tl->framesToSeconds(tl->duration()) + 10.0);
    int w = timeToX(dur);
    rightArea_->setMinimumWidth(w);
    tracksContainer_->setMinimumWidth(w);
}

double TimelineWidget::pixelsPerSecond() const { return pps_; }

void TimelineWidget::setPixelsPerSecond(double pps) {
    pps_ = pps;
    rebuildTracks();
    ensureTimelineWidthFits();
    ruler_->update();
}

void TimelineWidget::zoomIn()  { setPixelsPerSecond(std::min(500.0, pps_ * 1.25)); }
void TimelineWidget::zoomOut() { setPixelsPerSecond(std::max(5.0,   pps_ / 1.25)); }

int TimelineWidget::timeToX(double t) const {
    return static_cast<int>(t * pps_);
}

double TimelineWidget::xToTime(int x) const {
    return pps_ > 0 ? x / pps_ : 0.0;
}

double TimelineWidget::snap(double t) const {
    if (!project_) return t;
    int frame = project_->timeline()->secondsToFrames(t);
    int snapped = project_->timeline()->snap(frame, static_cast<int>(snapTolerance() * project_->timeline()->fps()));
    return project_->timeline()->framesToSeconds(snapped);
}

void TimelineWidget::setPlayhead(double t) {
    emit playheadChanged(t);
    ruler_->setPlayhead(t);
}

void TimelineWidget::deleteSelectedClip() {
    auto tl = project_->timeline();
    ObjectId sel = tl->selectedClipId();
    if (sel == INVALID_ID) return;
    tl->requestItemDeletion(sel);
    emit clipSelected(INVALID_ID);
}

QUndoStack* TimelineWidget::undoStack() const {
    return project_ ? project_->undoStack() : nullptr;
}

} // namespace ve
