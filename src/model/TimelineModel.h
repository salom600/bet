/*
 * VideoEditor - TimelineModel.h
 * The timeline: owns tracks + clips, ID-based registry, request* API.
 *
 * Adapted from Kdenlive's src/timeline2/model/timelinemodel.hpp.
 *
 * Design:
 *   - All clip mutations go through request* methods, which generate
 *     undo/redo lambdas (Fun) and push a FunctionalUndoCommand on the
 *     QUndoStack.
 *   - Clips are referenced by integer ObjectId, never by pointer.
 *   - The model maintains a SnapModel of all clip edges for snapping.
 *   - Tracks are also ID-based; the model keeps an ordered list.
 *   - Bin clips are looked up by string id via the BinModel pointer.
 *
 * The QAbstractItemModel interface is provided so a QTreeView could show
 * the timeline tree (Kdenlive uses QML; we use custom widgets but expose
 * the model for future flexibility).
 */
#pragma once

#include "../definitions.h"
#include "../undohelper.h"
#include "../utils/GenTime.h"
#include "SnapModel.h"
#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <memory>
#include <functional>

namespace ve {

class BinModel;
class BinClip;
class TrackModel;
class ClipModel;
class GroupsModel;
class EffectStackModel;

class TimelineModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit TimelineModel(std::shared_ptr<BinModel> bin, QObject* parent = nullptr);
    ~TimelineModel() override;

    // --- Bin access ---
    void setBin(std::shared_ptr<BinModel> bin) { m_bin = bin; }
    std::shared_ptr<BinModel> bin() const { return m_bin; }
    std::shared_ptr<BinClip> binClip(const QString& binClipId) const;

    // --- Profile / fps ---
    void setFps(double fps);
    double fps() const { return m_fps; }

    // --- Lookup by ID ---
    std::shared_ptr<TrackModel> track(ObjectId id) const;
    std::shared_ptr<ClipModel>  clip(ObjectId id)  const;

    const QList<ObjectId>& trackIds() const { return m_trackIds; }

    // --- QAbstractItemModel interface (for QTreeView of tracks→clips) ---
    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    // --- Snap ---
    /// Snap a frame position to the nearest clip edge / playhead within
    /// `tolerance` frames. Returns the snapped position, or `position`
    /// unchanged if no snap point is close enough.
    int snap(int position, int tolerance, int playheadPos = -1) const;

    // --- Request API (all generate undo/redo) ---
    /// Add a track. Returns the new track id.
    ObjectId requestAddTrack(TrackType type);

    /// Remove a track by id.
    bool requestRemoveTrack(ObjectId trackId);

    /// Insert a clip referencing binClipId at (trackId, position).
    /// Returns the new clip's id, or INVALID_ID on failure.
    ObjectId requestClipInsertion(const QString& binClipId, ObjectId trackId,
                                  int position, int in = 0, int out = -1,
                                  ClipState state = ClipState::Unknown);

    /// Move a clip to a new track/position.
    bool requestClipMove(ObjectId clipId, ObjectId newTrackId, int newPosition);

    /// Resize clip (trim). `fromStart` true = trim left edge, false = right.
    bool requestClipResize(ObjectId clipId, int newSize, bool fromStart);

    /// Delete a clip.
    bool requestItemDeletion(ObjectId clipId);

    /// Get total duration in frames.
    int duration() const;

    /// Convert frames <-> seconds.
    double framesToSeconds(int frames) const { return m_fps > 0 ? frames / m_fps : 0.0; }
    int    secondsToFrames(double s) const { return m_fps > 0 ? static_cast<int>(s * m_fps) : 0; }

    // --- Selection ---
    void setSelected(ObjectId clipId);
    ObjectId selectedClipId() const { return m_selectedClipId; }

    // --- Groups (forward to GroupsModel) ---
    bool groupItems(const QList<ObjectId>& clipIds);
    bool ungroupItem(ObjectId clipId);

signals:
    void structureChanged();
    void clipMoved(ObjectId clipId);
    void clipResized(ObjectId clipId);
    void clipAdded(ObjectId clipId);
    void clipRemoved(ObjectId clipId);
    void selectionChanged(ObjectId clipId);

private:
    ObjectId nextId();
    void registerClip(std::shared_ptr<ClipModel> clip);
    void deregisterClip(ObjectId id);
    void registerTrack(std::shared_ptr<TrackModel> track);
    void deregisterTrack(ObjectId id);

    void addClipSnaps(std::shared_ptr<ClipModel> clip);
    void removeClipSnaps(std::shared_ptr<ClipModel> clip);

    std::shared_ptr<BinModel> m_bin;
    QHash<ObjectId, std::shared_ptr<TrackModel>> m_tracks;
    QHash<ObjectId, std::shared_ptr<ClipModel>>  m_clips;
    QList<ObjectId> m_trackIds; // ordered
    ObjectId m_nextId = 1;
    ObjectId m_selectedClipId = INVALID_ID;

    double m_fps = 25.0;
    SnapModel m_snap;
};

} // namespace ve
