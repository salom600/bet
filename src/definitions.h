/*
 * VideoEditor - definitions.h
 * Common types and enums shared across model and UI layers.
 * Modeled on Kdenlive's src/definitions.h.
 */
#pragma once

#include <QString>
#include <QUuid>
#include <QHash>
#include <cstdint>

namespace ve {

// Unique identifier for any model object (clip, track, composition, ...).
// IDs are integers assigned monotonically by the owning model.
// -1 / 0 are reserved as "invalid".
using ObjectId = int;

constexpr ObjectId INVALID_ID = -1;

enum class ObjectType {
    NoItem = 0,
    TimelineClip,
    TimelineComposition,
    TimelineTrack,
    BinClip,
    BinFolder,
};

struct ObjectRef {
    ObjectType type = ObjectType::NoItem;
    ObjectId   id   = INVALID_ID;
    QUuid      uuid;
    bool operator==(const ObjectRef& o) const { return type == o.type && id == o.id; }
    bool operator!=(const ObjectRef& o) const { return !(*this == o); }
    bool valid() const { return id != INVALID_ID && type != ObjectType::NoItem; }
};

// Clip type as reported by the media backend after probing a file.
enum class ClipType {
    Unknown  = 0,
    Audio    = 1,
    Video    = 2,
    AV       = 3,   // both audio and video
    Image    = 4,
    Color    = 5,
    Text     = 6,
    Playlist = 7,   // nested sequence
};

// Which tracks of a clip are active on the timeline.
enum class ClipState {
    VideoOnly = 1,
    AudioOnly = 2,
    Disabled  = 3,
    Unknown   = 4,
};

// Track type. Note: Kdenlive splits video tracks into "VideoOnly" + a hidden
// audio pair; we collapse them into one Track for simplicity.
enum class TrackType {
    Video = 0,
    Image = 1,
    Audio = 2,
};

// What kind of operation is the user currently performing on the timeline?
// Used by ClipItem to choose cursor / drag behavior.
enum class OperationType {
    NoOperation    = 0,
    MoveOperation  = 1,
    ResizeStart    = 2,
    ResizeEnd      = 3,
    RollingStart   = 4,
    RollingEnd     = 5,
    RippleStart    = 6,
    RippleEnd      = 7,
    FadeIn         = 8,
    FadeOut        = 9,
    TransitionStart = 10,
    TransitionEnd  = 11,
    Spacer         = 12,
    Seek           = 13,
};

// Group types in the GroupsModel tree.
enum class GroupType {
    Normal,    // user-created group
    Selection, // transient selection group
    AVSplit,   // links audio+video halves of one source clip
    Leaf       // a clip or composition (terminal node)
};

// Timeline edit modes (Kdenlive-style).
enum class EditMode {
    NormalEdit = 0,
    OverwriteEdit = 1,
    InsertEdit = 2,
};

} // namespace ve
