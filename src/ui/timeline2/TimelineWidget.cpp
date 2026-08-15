#include "ui/timeline2/TimelineWidget.h"
#include "ui/timeline2/TimelineRuler.h"
#include "ui/timeline2/TrackHeadWidget.h"
#include "ui/timeline2/ClipItem.h"
#include "project/Project.h"
#include "model/TimelineModel.h"
#include "model/TrackModel.h"
#include "model/ClipModel.h"
#include "model/BinClip.h"
#include "model/BinModel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QPainter>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
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
    ruler_->setFixedHeight(28);
    rightLayoutV->addWidget(ruler_);
    rightLayoutV->addWidget(tracksContainer_, 1);

    scrollArea_->setWidget(rightArea_);
    scrollArea_->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // Accept drops from the bin (mime type application/x-ve-binclip)
    scrollArea_->setAcceptDrops(true);
    tracksContainer_->setAcceptDrops(true);
    rightArea_->setAcceptDrops(true);
    setAcceptDrops(true);

    h->addWidget(leftHeader_);
    h->addWidget(scrollArea_, 1);
    setMinimumHeight(280);
}

void TimelineWidget::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasFormat("application/x-ve-binclip")) {
        e->acceptProposedAction();
    }
}

void TimelineWidget::dropEvent(QDropEvent* e) {
    if (!e->mimeData()->hasFormat("application/x-ve-binclip")) return;
    QString binClipId = QString::fromUtf8(e->mimeData()->data("application/x-ve-binclip"));
    if (binClipId.isEmpty()) return;

    auto tl = project_->timeline();
    auto bin = project_->bin();
    auto bc = bin->clip(binClipId);
    if (!bc) return;

    // Determine target track based on clip type
    TrackType tt = TrackType::Video;
    if (bc->type() == ClipType::Audio) tt = TrackType::Audio;
    else if (bc->type() == ClipType::Image) tt = TrackType::Image;

    // Find first track of matching type
    ObjectId targetTrackId = INVALID_ID;
    for (ObjectId tid : tl->trackIds()) {
        auto t = tl->track(tid);
        if (t && t->type() == tt && !t->isLocked()) { targetTrackId = tid; break; }
    }
    if (targetTrackId == INVALID_ID) {
        // No matching track - create one
        targetTrackId = tl->requestAddTrack(tt);
    }

    // Compute drop position in seconds from drop x coordinate
    QPoint pos = e->position().toPoint();
    // Need to map to scrollArea contents
    QPoint mapped = scrollArea_->viewport()->mapFrom(this, pos);
    double dropTime = xToTime(mapped.x() + scrollArea_->horizontalScrollBar()->value());
    int dropFrame = tl->secondsToFrames(std::max(0.0, dropTime));

    // Snap to playhead / existing clip edges
    int snapped = tl->snap(dropFrame, static_cast<int>(snapTolerance() * tl->fps()));
    if (snapped >= 0) dropFrame = snapped;

    tl->requestClipInsertion(binClipId, targetTrackId, dropFrame);
    e->acceptProposedAction();
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

        // Track background color depends on type
        QString trackBg = "#1d1f24";
        QString trackBorder = "#2a2d33";
        switch (t->type()) {
            case TrackType::Video: trackBg = "#1a1f2a"; break;
            case TrackType::Image: trackBg = "#221a2a"; break;
            case TrackType::Audio: trackBg = "#1a2a22"; break;
        }

        auto* trackWidget = new QWidget(tracksContainer_);
        trackWidget->setFixedHeight(trackHeight_);
        trackWidget->setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;").arg(trackBg, trackBorder));
        trackWidget->setAcceptDrops(true);

        auto* clipLayout = new QHBoxLayout(trackWidget);
        clipLayout->setContentsMargins(0, 0, 0, 0);
        clipLayout->setSpacing(0);
        clipLayout->addStretch(1);

        for (ObjectId cid : t->clipsSorted()) {
            auto c = tl_model_clip(cid);
            if (!c) continue;
            auto* ci = new ClipItem(c, this, trackWidget);
            ci->setFixedHeight(trackHeight_ - 6);
            int x = timeToX(project_->timeline()->framesToSeconds(c->getPosition()));
            int w = std::max(12, timeToX(project_->timeline()->framesToSeconds(c->getPlaytime())));
            ci->setGeometry(x, 3, w, trackHeight_ - 6);
            ci->show();
            connect(ci, &ClipItem::selected, this, [this](int clipId) {
                auto c = project_->timeline()->clip(clipId);
                emit clipSelected(c.get());
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
    emit clipSelected(nullptr);
}

QUndoStack* TimelineWidget::undoStack() const {
    return project_ ? project_->undoStack() : nullptr;
}

} // namespace ve
