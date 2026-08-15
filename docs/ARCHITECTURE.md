# Kdenlive Architecture Analysis — Findings & Lessons Applied

After cloning and studying the Kdenlive source (~162 MB, ~900 source files),
here is what we extract and adapt to our `bet` project. We do **not** vendor
MLT (it is a heavy C++ framework unavailable in the GitHub Actions runners
without painful cross-compilation). Instead, we adopt Kdenlive's
**architectural patterns** and implement them on top of a thin FFmpeg wrapper
we already have.

## 1. What makes Kdenlive "professional"

Kdenlive separates concerns into distinct layers:

```
                  +------------------------------------+
                  |            GUI Layer               |
                  | MainWindow / Bin / Monitor /       |
                  | TimelineWidget / EffectStackView   |
                  +-----------------+------------------+
                                    |  (signals/slots, model/view)
                  +-----------------v------------------+
                  |          Model Layer               |
                  | KdenliveDoc  ProjectItemModel      |
                  | TimelineItemModel  TrackModel      |
                  | ClipModel  CompositionModel        |
                  | EffectStackModel  KeyframeModel    |
                  | GroupsModel  SnapModel  MarkerList |
                  +-----------------+------------------+
                                    |  (lambda undo, ID-based refs)
                  +-----------------v------------------+
                  |       Asset/Repository Layer       |
                  | EffectsRepository  TransitionsRepo |
                  | ProfileRepository  RenderPresets   |
                  | AbstractAssetsRepository<T>        |
                  +-----------------+------------------+
                                    |
                  +-----------------v------------------+
                  |         Media Backend Layer        |
                  | MLT: Producer / Playlist / Tractor |
                  |       / Filter / Transition        |
                  +------------------------------------+
```

The user is right: the previous version of `bet` skipped the **Asset** and
**Repository** layers entirely and used one big Timeline object that mixed
model + view + persistence. This redesign introduces those layers properly.

## 2. Key Kdenlive design patterns we adopt

### 2.1 ID-based references, not pointer-based

Kdenlive never hands out pointers to clips/tracks. Every model element gets
a unique integer ID at construction; the `TimelineModel` keeps a registry
of `id -> weak_ptr<Element>` and friends can call back into the timeline:

```cpp
int clipId = timeline->requestClipInsertion(binClipId, trackId, pos, ...);
timeline->requestClipMove(clipId, newTrackId, newPos);
timeline->requestItemDeletion(clipId);
```

**Why this matters:** undo/redo lambdas can capture IDs (trivially copyable)
instead of dangling pointers. When a clip is deleted and later restored via
undo, the new clip gets the same ID — every reference is automatically valid
again.

**We adopt:** `ObjectId` enum + `TimelineModel` registry + `requestXMove/Y/Z`
public API. All UI manipulation goes through the timeline's request methods,
not by reaching into clip pointers.

### 2.2 Lambda-based undo (the `Fun = std::function<bool()>` pattern)

Kdenlive's `undohelper.hpp` defines:

```cpp
using Fun = std::function<bool(void)>;
#define PUSH_LAMBDA(operation, lambda) ...
```

Every mutating request method takes two `Fun&` parameters (`undo, redo`) and
**appends** its own undo/redo step into them. A complex operation (e.g. move
a group of 5 clips) composes 5 simple lambdas. If any sub-step fails, the
caller runs the partial `undo` lambda to roll back. This is far more robust
than the classic "create a QUndoCommand subclass per operation" approach.

**We adopt:** Replace our `LambdaCommand` (which captures by value at push
time) with `FunctionalUndoCommand(Fun undo, Fun redo, ...)` and the
PUSH_LAMBDA composition pattern. Track IDs in lambdas, not pointers.

### 2.3 The `MoveableItem<Service>` CRTP base

Kdenlive's `MoveableItem<Mlt::Producer>` (clip) and `MoveableItem<Mlt::Transition>`
(composition) share a common base that provides: `getId()`, `getPosition()`,
`getInOut()`, `getCurrentTrackId()`, `setSelected()`, etc. The template
parameter is the underlying MLT service type.

**We adopt:** A `MoveableItem` base class parameterized by our `ProducerRef`
(shared_ptr to a MediaSource). Both `ClipModel` and `CompositionModel` derive
from it. This avoids duplicating position/in-out/track-id logic.

### 2.4 SnapModel as a separate concern

Snapping in Kdenlive is a separate `SnapModel` (in `snapmodel.hpp`), a thin
wrapper around a `std::map<int,int>` of position to refcount. It exposes
`addPoint / removePoint / getClosestPoint / getNextPoint / getPreviousPoint /
proposeSize`. Each clip also has its own `ClipSnapModel` for its internal
edit points, and the timeline aggregates them.

**We adopt:** Lift snapping out of `Timeline::snap()` (which currently
re-scans every clip on every query) into a dedicated `SnapModel` that
maintains a sorted point set incrementally. This is both cleaner and much
faster for large timelines.

### 2.5 Bin != Timeline (the big missing piece)

Kdenlive's `Bin` (project bin) is **completely separate** from the timeline.
A `ProjectClip` lives in the bin and represents the **source media file**.
When you drag a bin clip onto the timeline, a `ClipModel` instance is created
that *references* the bin clip by `binClipId`. One bin clip can be placed on
the timeline many times; trimming one timeline instance doesn't affect the
others.

Our previous version conflated these: a `Clip` was both the source file and
the timeline instance. This made multi-instance impossible.

**We adopt:** Split into `BinClip` (source media, lives in `BinModel`) and
`ClipModel` (timeline instance, references a `BinClip` by id). The Bin is a
tree (`AbstractProjectItem` -> `ProjectFolder` / `ProjectClip` /
`ProjectSubClip`) so users can organize clips into folders.

### 2.6 ProfileModel (resolution/fps/aspect — separate from project)

Kdenlive stores the render profile (1920x1080@25fps SAR 1:1 DAR 16:9
colorspace 709 progressive) on the project itself, not on individual clips.
Clips are scaled to fit at render time. This is the only sane way to mix
footage from different cameras.

**We adopt:** A `Profile` struct (width, height, fps_num, fps_den, sar_num,
sar_den, colorspace, progressive) on the project. All preview rendering and
export go through this profile.

### 2.7 MLT-style XML project format (`.kdenlive` is just MLT XML)

`.kdenlive` files are MLT XML: `<mlt>` root, `<profile>` element, `<producer>`
elements for source media, `<playlist>` for bin + each track, `<tractor>` to
group tracks. Kdenlive-specific metadata is stored as `<property
name="kdenlive:xxx">` attributes inside MLT elements — so MLT itself can
render a `.kdenlive` file directly via `melt`.

**We adopt:** Switch from ad-hoc JSON to a structured XML format that
mirrors MLT's structure. We keep the `.veproj` extension but the schema is
now `<veproject>` root with `<profile>`, `<bin>` (producers), `<timeline>`
(tractor+playlists). This makes future MLT integration trivial.

### 2.8 EffectStackModel (the second big missing piece)

A real editor's clips have **effects** (brightness, contrast, fade in,
transform, blur, ...) applied in a stack. Kdenlive models this as a tree
(`EffectStackModel : AbstractTreeModel`) of `EffectItemModel` instances,
each holding an `AssetParameterModel` with named parameters and keyframes.

Our previous version had only hard-coded transform properties (posX/posY/
scale/opacity). Professional editors expose effects as **plugins** loaded
from a registry at startup.

**We adopt:** An `EffectsRepository` (similar to Kdenlive's
`AbstractAssetsRepository<T>`) that scans for effect definitions at startup
(either built-in JSON manifests or, in future, MLT/Frei0r plugins). Each
clip owns an `EffectStackModel` to which effects can be appended / reordered
/ removed with full undo/redo. The built-in `transform` (posX/posY/scale/
opacity) and `volume`/`pan` become regular effects in the stack — not
hard-coded fields on Clip.

### 2.9 GroupsModel

Kdenlive lets users group clips so they move/resize together. Internally
this is a tree (GroupsModel) where leaves are clips/compositions and
internal nodes are groups. Moving one clip moves its whole group atomically
(with per-clip undo lambdas composed together).

**We adopt:** A `GroupsModel` that tracks `clipId -> groupId`. The timeline's
`requestClipMove` checks for a group and forwards the move to all members.

### 2.10 Two monitors: Clip Monitor + Project Monitor

Kdenlive has TWO monitors: the Clip Monitor shows the source clip from the
bin (scrubbing the original file), and the Project Monitor shows the
rendered timeline at the playhead. This is critical for professional
workflow — you scrub the source in the Clip Monitor to find an in/out, then
push to the timeline.

**We adopt:** `MonitorManager` owning two `Monitor` widgets. Clicking a bin
clip loads it in the Clip Monitor; clicking on the timeline focuses the
Project Monitor.

## 3. What we deliberately do NOT adopt from Kdenlive

- **MLT framework dependency** — too heavy for our CI (would need to
  build MLT from source on Windows, ~1 hour per build). We keep our thin
  FFmpeg wrapper. The MediaBackend abstraction means we can swap in MLT
  later without touching the model layer.
- **QML timeline UI** — Kdenlive renders the timeline with QML for
  performance with thousands of clips. We stay with QWidget + custom paint
  for simplicity; it scales fine to a few hundred clips per timeline which
  covers the "basics" target.
- **KDE Frameworks dependencies** (KConfig, KDDockWidgets, KMessageWidget,
  KRecentDirs, KAutoSaveFile) — would require packaging all of KDE on
  Windows. We use only Qt6.
- **Subtitle model, capture/recorder, scopes (vectorscope, histogram)**,
  **jog shuttle, speech-to-text** — out of scope for the basics, but the
  architecture leaves clean extension points.

## 4. The redesigned `bet` architecture

```
src/
|- main.cpp
|- definitions.h              # ObjectId, ClipType, OperationType, PlaylistState
|- core.h/cpp                 # Core singleton (like Kdenlive's Core)
|- undohelper.h/cpp           # Fun = std::function<bool()>, FunctionalUndoCommand, PUSH_LAMBDA
|
|- model/
|   |- MoveableItem.h         # base for Clip/Composition (getId/getPosition/getInOut)
|   |- Profile.h/cpp          # width/height/fps/sar/dar/colorspace
|   |- BinClip.h/cpp          # source media descriptor (probe + thumbnails)
|   |- BinFolder.h/cpp        # folder in the bin tree
|   |- BinModel.h/cpp         # tree of BinFolder/BinClip, ID-based registry
|   |- TimelineModel.h/cpp    # the timeline; owns tracks+clips, ID registry
|   |- TrackModel.h/cpp       # one track (video/audio/image), owns clip IDs
|   |- ClipModel.h/cpp        # timeline clip instance, references a BinClip by id
|   |- CompositionModel.h/cpp # transition between two clips (placeholder for now)
|   |- GroupsModel.h/cpp      # clip grouping tree
|   |- SnapModel.h/cpp        # sorted snap-point set
|   `- MarkerListModel.h/cpp  # named positions on the timeline
|
|- assets/
|   |- EffectsRepository.h/cpp     # registry: effectId -> EffectDescription
|   |- EffectDescription.h/cpp     # name, parameters, type (video/audio)
|   |- EffectStackModel.h/cpp      # tree of effects on a clip/track
|   |- EffectItemModel.h/cpp       # one effect instance with parameter values
|   `- KeyframeModel.h/cpp         # per-parameter keyframes (linear/smooth/discrete)
|
|- media/
|   |- MediaBackend.h         # interface: probe, open, grabFrame, audioPeaks
|   |- FFmpegBackend.h/cpp    # MediaBackend impl using FFmpeg libs
|   `- ThumbnailCache.h/cpp   # disk cache of thumbnails keyed by file hash
|
|- project/
|   |- Project.h/cpp          # owns BinModel + TimelineModel + Profile + path
|   |- ProjectSerializer.h/cpp # load/save .veproj (XML, MLT-like structure)
|   `- ProfileRepository.h/cpp # built-in profiles (1080p25, 720p30, etc.)
|
|- ui/
|   |- MainWindow.h/cpp
|   |- toolbar/Toolbar.h/cpp
|   |- bin/BinWidget.h/cpp        # QTreeView of BinModel with drag-to-timeline
|   |- bin/ClipMonitorWidget.h/cpp # shows source clip when bin item clicked
|   |- monitor/ProjectMonitor.h/cpp # shows timeline frame at playhead
|   |- monitor/MonitorManager.h/cpp # switches between clip/project monitor
|   |- timeline2/TimelineWidget.h/cpp
|   |- timeline2/TimelineRuler.h/cpp
|   |- timeline2/TrackHeadWidget.h/cpp
|   |- timeline2/ClipItem.h/cpp
|   |- effects/EffectListWidget.h/cpp    # browses EffectsRepository
|   |- effects/EffectStackView.h/cpp     # edits selected clip's stack
|   `- properties/PropertiesPanel.h/cpp  # clip metadata + duration
|
`- utils/
    |- GenTime.h/cpp          # frame-aware time (Kdenlive-style)
    |- Timecode.h/cpp         # HH:MM:SS:FF string formatting
    `- Xml.h/cpp              # QDom helpers (Kdenlive's Xml namespace)
```

This is a faithful adaptation of Kdenlive's `src/` layout (model/ +
assets/ + bin/ + timeline2/ + monitor/ + effects/ + project/ + utils/),
translated to our lighter dependency set.

## 5. Migration plan

1. Introduce `definitions.h`, `undohelper.{h,cpp}`, `GenTime`, `Profile`.
2. Split `Clip` into `BinClip` (source) + `ClipModel` (instance).
3. Replace ad-hoc Timeline pointer-juggling with `TimelineModel` ID registry
   + `requestXMove/Y/Z` API.
4. Move snapping into a dedicated `SnapModel`.
5. Replace JSON serializer with XML serializer (MLT-like structure).
6. Add `EffectsRepository` + `EffectStackModel`; convert posX/posY/scale/
   opacity into a built-in `transform` effect on the stack.
7. Add `BinWidget` (QTreeView of bin) and `ClipMonitorWidget`; refactor
   `MainWindow` to host Bin (left), Clip Monitor + Project Monitor (top),
   Timeline (bottom), Effect Stack + Properties (right).
8. Push, monitor CI, auto-fix until green.
