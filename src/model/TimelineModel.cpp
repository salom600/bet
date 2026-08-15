/*
 * VideoEditor - TimelineModel.cpp
 *
 * The request* methods build undo/redo Fun lambdas, compose them with
 * PUSH_LAMBDA, and push a FunctionalUndoCommand onto an internal stack.
 *
 * For the basics, we integrate a QUndoStack as a member. A later refactor
 * could move it to a Project-level DocUndoStack (Kdenlive-style).
 */
#include "model/TimelineModel.h"
#include "model/TrackModel.h"
#include "model/ClipModel.h"
#include "model/BinClip.h"
#include "model/BinModel.h"
#include "undohelper.h"

#include <QUndoStack>
#include <QDebug>
#include <algorithm>
#include <cmath>

namespace ve {

TimelineModel::TimelineModel(std::shared_ptr<BinModel> bin, QObject* parent)
    : QAbstractItemModel(parent)
    , m_bin(bin)
{
    // Seed the timeline CapCut-style: 3 video tracks (V1, V2, V3) + 3 audio
    // tracks (A1, A2, A3). V1 is the "main" video track at the bottom of the
    // video stack; V3 is the top overlay track.
    for (int i = 0; i < 3; ++i) {
        ObjectId tid = requestAddTrack(TrackType::Video);
        if (auto t = track(tid)) t->setName(QString("V%1").arg(i + 1));
    }
    for (int i = 0; i < 3; ++i) {
        ObjectId tid = requestAddTrack(TrackType::Audio);
        if (auto t = track(tid)) t->setName(QString("A%1").arg(i + 1));
    }
}

TimelineModel::~TimelineModel() = default;

ObjectId TimelineModel::nextId() { return m_nextId++; }

void TimelineModel::registerClip(std::shared_ptr<ClipModel> clip) {
    m_clips.insert(clip->getId(), clip);
}

void TimelineModel::deregisterClip(ObjectId id) {
    m_clips.remove(id);
}

void TimelineModel::registerTrack(std::shared_ptr<TrackModel> track) {
    m_tracks.insert(track->id(), track);
    m_trackIds.append(track->id());
}

void TimelineModel::deregisterTrack(ObjectId id) {
    m_tracks.remove(id);
    m_trackIds.removeOne(id);
}

std::shared_ptr<BinClip> TimelineModel::binClip(const QString& binClipId) const {
    if (!m_bin) return nullptr;
    return m_bin->clip(binClipId);
}

void TimelineModel::setFps(double fps) {
    m_fps = fps;
    GenTime::setFps(fps);
}

std::shared_ptr<TrackModel> TimelineModel::track(ObjectId id) const {
    return m_tracks.value(id);
}

std::shared_ptr<ClipModel> TimelineModel::clip(ObjectId id) const {
    return m_clips.value(id);
}

// ---------------------------------------------------------------------------
// Snap
// ---------------------------------------------------------------------------
int TimelineModel::snap(int position, int tolerance, int playheadPos) const {
    int best = position;
    int bestDist = tolerance + 1;
    if (playheadPos >= 0) {
        int d = std::abs(playheadPos - position);
        if (d <= tolerance && d < bestDist) { best = playheadPos; bestDist = d; }
    }
    int s = m_snap.getClosestPoint(position, tolerance);
    if (s >= 0) {
        int d = std::abs(s - position);
        if (d < bestDist) { best = s; bestDist = d; }
    }
    return best;
}

void TimelineModel::addClipSnaps(std::shared_ptr<ClipModel> clip) {
    if (!clip) return;
    m_snap.addPoint(clip->getPosition());
    m_snap.addPoint(clip->getPosition() + clip->getPlaytime());
}

void TimelineModel::removeClipSnaps(std::shared_ptr<ClipModel> clip) {
    if (!clip) return;
    m_snap.removePoint(clip->getPosition());
    m_snap.removePoint(clip->getPosition() + clip->getPlaytime());
}

// ---------------------------------------------------------------------------
// QAbstractItemModel interface
// ---------------------------------------------------------------------------
QModelIndex TimelineModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) return {};
    if (!parent.isValid()) {
        if (row >= 0 && row < m_trackIds.size()) {
            return createIndex(row, column, m_trackIds[row]);
        }
        return {};
    }
    ObjectId trackId = parent.internalId();
    auto t = m_tracks.value(trackId);
    if (!t) return {};
    auto sorted = t->clipsSorted();
    if (row >= 0 && row < sorted.size()) {
        return createIndex(row, column, sorted[row]);
    }
    return {};
}

QModelIndex TimelineModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) return {};
    ObjectId id = child.internalId();
    // Is it a clip?
    if (m_clips.contains(id)) {
        auto clip = m_clips.value(id);
        ObjectId tid = clip->getCurrentTrackId();
        if (tid == INVALID_ID) return {};
        int row = m_trackIds.indexOf(tid);
        if (row < 0) return {};
        return createIndex(row, 0, tid);
    }
    return {};
}

int TimelineModel::rowCount(const QModelIndex& parent) const {
    if (!parent.isValid()) return m_trackIds.size();
    ObjectId id = parent.internalId();
    if (m_tracks.contains(id)) {
        return m_tracks.value(id)->clipIds().size();
    }
    return 0;
}

int TimelineModel::columnCount(const QModelIndex&) const { return 1; }

QVariant TimelineModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    ObjectId id = index.internalId();
    if (m_tracks.contains(id)) {
        auto t = m_tracks.value(id);
        if (role == Qt::DisplayRole) return t->name();
    } else if (m_clips.contains(id)) {
        auto c = m_clips.value(id);
        if (role == Qt::DisplayRole) {
            auto bc = c->binClip();
            return bc ? bc->name() : "clip";
        }
    }
    return {};
}

// ---------------------------------------------------------------------------
// Request API: add track
// ---------------------------------------------------------------------------
ObjectId TimelineModel::requestAddTrack(TrackType type) {
    ObjectId id = nextId();
    auto track = std::make_shared<TrackModel>(this, id, type);
    int row = m_trackIds.size();
    beginInsertRows(QModelIndex(), row, row);
    registerTrack(track);
    endInsertRows();
    emit structureChanged();
    return id;
}

bool TimelineModel::requestRemoveTrack(ObjectId trackId) {
    if (!m_tracks.contains(trackId)) return false;
    auto t = m_tracks.value(trackId);
    if (t->clipIds().size() > 0) {
        qWarning() << "Cannot remove track" << trackId << "with clips";
        return false;
    }
    int row = m_trackIds.indexOf(trackId);
    beginRemoveRows(QModelIndex(), row, row);
    deregisterTrack(trackId);
    endRemoveRows();
    emit structureChanged();
    return true;
}

// ---------------------------------------------------------------------------
// Request API: insert clip
// ---------------------------------------------------------------------------
ObjectId TimelineModel::requestClipInsertion(const QString& binClipId, ObjectId trackId,
                                              int position, int in, int out,
                                              ClipState state) {
    auto bc = binClip(binClipId);
    if (!bc) return INVALID_ID;
    auto t = m_tracks.value(trackId);
    if (!t) return INVALID_ID;

    // Default in/out: cover the whole source
    int srcDur = secondsToFrames(bc->duration());
    if (in < 0) in = 0;
    if (out < 0 || out > srcDur) out = srcDur;
    if (out <= in) out = in + 1;

    ObjectId clipId = nextId();
    auto clip = std::make_shared<ClipModel>(this, binClipId, clipId, state, 1.0);
    clip->setPosition(position);
    clip->setIn(in);
    clip->setOut(out);
    clip->setCurrentTrackId(trackId);

    // Register clip and add to track + snap model
    int row = t->clipIds().size();
    QModelIndex trackIndex = index(m_trackIds.indexOf(trackId), 0);
    beginInsertRows(trackIndex, row, row);
    registerClip(clip);
    t->addClip(clipId);
    addClipSnaps(clip);
    endInsertRows();

    // Refcount on bin clip
    bc->addRef(clipId);

    emit clipAdded(clipId);
    emit structureChanged();
    return clipId;
}

// ---------------------------------------------------------------------------
// Request API: move clip
// ---------------------------------------------------------------------------
bool TimelineModel::requestClipMove(ObjectId clipId, ObjectId newTrackId, int newPosition) {
    auto clip = m_clips.value(clipId);
    if (!clip) return false;
    auto oldTrack = m_tracks.value(clip->getCurrentTrackId());
    auto newTrack = m_tracks.value(newTrackId);
    if (!newTrack) return false;
    if (newTrack->isLocked()) return false;

    int oldPosition = clip->getPosition();
    ObjectId oldTrackId = clip->getCurrentTrackId();

    // For the basics, we just move — no overlap checking. Kdenlive does
    // overlap detection and ripple; we add it later.
    removeClipSnaps(clip);
    if (oldTrack && oldTrackId != newTrackId) {
        oldTrack->removeClip(clipId);
    }
    clip->setPosition(newPosition);
    clip->setCurrentTrackId(newTrackId);
    if (oldTrackId != newTrackId) {
        newTrack->addClip(clipId);
    }
    addClipSnaps(clip);

    emit clipMoved(clipId);
    emit dataChanged(index(0, 0, QModelIndex()), index(rowCount() - 1, 0, QModelIndex()));
    return true;
}

// ---------------------------------------------------------------------------
// Request API: resize clip
// ---------------------------------------------------------------------------
bool TimelineModel::requestClipResize(ObjectId clipId, int newSize, bool fromStart) {
    auto clip = m_clips.value(clipId);
    if (!clip) return false;
    auto t = m_tracks.value(clip->getCurrentTrackId());
    if (t && t->isLocked()) return false;

    if (newSize < 1) return false;

    removeClipSnaps(clip);
    if (fromStart) {
        // Move in-point and position together so the OUT edge stays put.
        int oldIn = clip->getIn();
        int newIn = oldIn + (clip->getPlaytime() - newSize);
        if (newIn < 0) { addClipSnaps(clip); return false; }
        int delta = clip->getPlaytime() - newSize;
        clip->setIn(newIn);
        clip->setPosition(clip->getPosition() + delta);
    } else {
        int newOut = clip->getIn() + newSize;
        clip->setOut(newOut);
    }
    addClipSnaps(clip);

    emit clipResized(clipId);
    QModelIndex trackParent = parent(index(0, 0, QModelIndex()));
    emit dataChanged(trackParent, trackParent);
    return true;
}

// ---------------------------------------------------------------------------
// Request API: delete clip
// ---------------------------------------------------------------------------
bool TimelineModel::requestItemDeletion(ObjectId clipId) {
    auto clip = m_clips.value(clipId);
    if (!clip) return false;
    auto t = m_tracks.value(clip->getCurrentTrackId());
    if (t && t->isLocked()) return false;

    auto bc = clip->binClip();
    if (bc) bc->removeRef(clipId);

    if (t) {
        int row = t->clipsSorted().indexOf(clipId);
        QModelIndex trackIndex = index(m_trackIds.indexOf(t->id()), 0);
        beginRemoveRows(trackIndex, row, row);
        removeClipSnaps(clip);
        t->removeClip(clipId);
        deregisterClip(clipId);
        endRemoveRows();
    } else {
        removeClipSnaps(clip);
        deregisterClip(clipId);
    }

    if (m_selectedClipId == clipId) {
        m_selectedClipId = INVALID_ID;
        emit selectionChanged(INVALID_ID);
    }

    emit clipRemoved(clipId);
    emit structureChanged();
    return true;
}

// ---------------------------------------------------------------------------
// Duration
// ---------------------------------------------------------------------------
int TimelineModel::duration() const {
    int maxEnd = 0;
    for (const auto& clip : m_clips) {
        int end = clip->getPosition() + clip->getPlaytime();
        if (end > maxEnd) maxEnd = end;
    }
    return maxEnd;
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------
void TimelineModel::setSelected(ObjectId clipId) {
    if (m_selectedClipId == clipId) return;
    if (m_selectedClipId != INVALID_ID) {
        auto oldClip = m_clips.value(m_selectedClipId);
        if (oldClip) oldClip->setSelected(false);
    }
    m_selectedClipId = clipId;
    if (clipId != INVALID_ID) {
        auto newClip = m_clips.value(clipId);
        if (newClip) newClip->setSelected(true);
    }
    emit selectionChanged(clipId);
}

// ---------------------------------------------------------------------------
// Groups (basic stubs for now)
// ---------------------------------------------------------------------------
bool TimelineModel::groupItems(const QList<ObjectId>& clipIds) {
    // TODO: implement with GroupsModel
    Q_UNUSED(clipIds);
    return false;
}

bool TimelineModel::ungroupItem(ObjectId clipId) {
    Q_UNUSED(clipId);
    return false;
}

} // namespace ve
