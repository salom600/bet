# VideoEditor 0.2 — Kdenlive-inspired architecture

A cross-platform C++ video editor for **Linux** and **Windows**, built with
**Qt6 Widgets** and **FFmpeg**, redesigned using architectural patterns
studied from [Kdenlive](https://github.com/KDE/kdenlive).

![status](https://img.shields.io/badge/status-alpha-orange)
![lang](https://img.shields.io/badge/lang-C%2B%2B17-blue)
![ui](https://img.shields.io/badge/UI-Qt6-green)
![arch](https://img.shields.io/badge/architecture-Kdenlive--inspired-purple)

## What's new in 0.2

Version 0.1 was a basic scaffold. Version 0.2 redesigns the architecture by
studying Kdenlive's source code (cloned from `github.com/KDE/kdenlive`,
~900 source files) and adapting its key patterns to a lighter dependency
set (Qt6 + FFmpeg, no MLT, no KDE Frameworks).

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the full analysis
and the list of patterns adopted.

## Architecture (Kdenlive-inspired, MLT-light)

```
src/
├── definitions.h              # ObjectId, ClipType, OperationType, ...
├── undohelper.h               # Fun = std::function<bool()>, FunctionalUndoCommand
├── main.cpp
│
├── model/                     # Pure data model (no UI, no FFmpeg)
│   ├── MoveableItem.h         # base for Clip/Composition (getId/getPosition/...)
│   ├── Profile.h/cpp          # width/height/fps/sar/dar/colorspace
│   ├── BinClip.h/cpp          # source media descriptor
│   ├── BinFolder.h/cpp        # folder in the bin tree
│   ├── BinModel.h/cpp         # QAbstractItemModel tree of BinFolder/BinClip
│   ├── TimelineModel.h/cpp    # ID-registry timeline; request* API
│   ├── TrackModel.h/cpp       # one track, owns clip IDs
│   ├── ClipModel.h/cpp        # timeline clip instance → references BinClip
│   ├── SnapModel.h/cpp        # sorted snap-point set
│   └── ...
│
├── assets/                    # Effect / asset repository layer
│   ├── EffectsRepository.h/cpp
│   ├── EffectDescription.h/cpp
│   ├── EffectStackModel.h/cpp # tree of effects on a clip
│   ├── EffectItemModel.h/cpp  # one effect instance
│   └── KeyframeModel.h/cpp    # (placeholder for now)
│
├── media/                     # Media backend abstraction
│   ├── MediaBackend.h         # interface
│   ├── FFmpegBackend.h/cpp    # impl using libavformat/codec/swscale/...
│   └── ThumbnailCache.h/cpp   # disk cache of thumbnails
│
├── project/                   # Project + persistence
│   ├── Project.h/cpp          # owns Bin + Timeline + Profile + path
│   ├── ProjectSerializer.h/cpp # XML .veproj (MLT-like structure)
│   └── ProfileRepository.h/cpp # built-in render profiles
│
├── ui/                        # Qt6 widgets
│   ├── MainWindow.h/cpp
│   ├── toolbar/Toolbar.h/cpp
│   ├── bin/BinWidget.h/cpp    # QTreeView of BinModel, drag-to-timeline
│   ├── bin/ClipMonitorWidget.h/cpp
│   ├── monitor/ProjectMonitor.h/cpp
│   ├── monitor/MonitorManager.h/cpp
│   ├── timeline2/TimelineWidget.h/cpp
│   ├── timeline2/TimelineRuler.h/cpp
│   ├── timeline2/TrackHeadWidget.h/cpp
│   ├── timeline2/ClipItem.h/cpp
│   ├── effects/EffectStackView.h/cpp
│   └── properties/PropertiesPanel.h/cpp
│
└── utils/
    ├── GenTime.h/cpp          # frame-aware time
    ├── Timecode.h/cpp         # (planned)
    └── Xml.h/cpp              # QDom helpers
```

## Key Kdenlive patterns adopted

1. **Bin ≠ Timeline** — source clips live in `BinModel` (a tree of folders
   and clips). Dragging a bin clip onto the timeline creates a `ClipModel`
   that references the bin clip by string ID. One bin clip can be placed
   multiple times.

2. **ID-based references, not pointers** — every model element has a
   unique integer ID. Undo/redo lambdas capture IDs (trivially copyable)
   instead of dangling pointers.

3. **`request*` API on TimelineModel** — `requestClipInsertion`,
   `requestClipMove`, `requestClipResize`, `requestItemDeletion`,
   `requestAddTrack`, `requestRemoveTrack`. All UI manipulation goes
   through these.

4. **Lambda-based undo** — `using Fun = std::function<bool()>` +
   `PUSH_LAMBDA` macro for composition. `FunctionalUndoCommand` wraps two
   `Fun` objects as a `QUndoCommand`.

5. **MoveableItem base** — `ClipModel` inherits from `MoveableItem` which
   provides `getId/getPosition/getInOut/getCurrentTrackId/setSelected`.

6. **SnapModel** — separate `std::map<int,int>` (position→refcount) for
   efficient snapping. Maintained incrementally as clips move/resize.

7. **EffectStackModel** — each clip owns an effect stack. Built-in
   `transform` / `fade_in` / `fade_out` / `volume` / `pan` / `blur` /
   `brightness` effects loaded from `EffectsRepository`. Auto-appends a
   `transform` effect on video/image clips and `volume`+`pan` on audio
   clips at creation.

8. **Two monitors** — Clip Monitor (scrub source bin clip) + Project
   Monitor (rendered timeline at playhead), managed by `MonitorManager`.

9. **ProfileModel** — `Profile` struct (width/height/fps/sar/dar/colorspace)
   on the project. Built-in profiles loaded by `ProfileRepository`.

10. **XML project format (MLT-like)** — `.veproj` files use a
    `<veproject>` root with `<profile>`, `<bin>` (folders+clips),
    `<timeline>` (tracks+clips) structure mirroring MLT's XML so a future
    MLT integration is straightforward.

## Features (current)

- Import via File → Import or drag-and-drop onto the bin
- Bin with folders, drag-to-timeline
- 6 pre-seeded tracks (2 video + 2 image + 2 audio)
- Clip drag/trim with snap-to-edges and snap-to-playhead
- Clip Monitor + Project Monitor
- Per-track mute / visibility / lock with visual state
- Per-clip Effect Stack (add/remove/reorder effects, edit parameters)
- Undo/Redo (Ctrl+Z / Ctrl+Y)
- Project save/load to XML `.veproj` (Ctrl+S / Ctrl+O)
- Export to MP4 (H.264) via FFmpeg `filter_complex` pipeline (Ctrl+E)
- Dark theme (2026 modern) with accent `#5ac8fa`
- Keyboard shortcuts: Space, Delete, Ctrl+Z/Y/S/O/I/E

## Build

### Linux

```bash
sudo apt-get install -y \
    build-essential cmake ninja-build pkg-config \
    qt6-base-dev qt6-base-dev-tools \
    libavformat-dev libavcodec-dev libavutil-dev \
    libswscale-dev libswresample-dev \
    libgl1-mesa-dev libegl1-mesa-dev libxkbcommon-dev

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/VideoEditor
```

### Windows

1. Install Visual Studio 2022 with C++ workload.
2. (Done automatically in CI) Qt6 installed via `aqtinstall`.
3. (Done automatically in CI) FFmpeg shared build from BtbN/FFmpeg-Builds.
4. Build with CMake + Ninja.

See `.github/workflows/build.yml` for the exact CI recipe.

## CI

`.github/workflows/build.yml` builds on **Ubuntu 24.04 + Windows 2022** in
parallel. Artifacts are uploaded on every push (retention 30 days). Release
assets auto-upload on tag pushes (`v*`).

## License

MIT.
