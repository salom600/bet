# VideoEditor

A cross-platform C++ video editor for **Linux** and **Windows**, built with **Qt6 Widgets** and **FFmpeg**.

![status](https://img.shields.io/badge/status-alpha-orange)
![lang](https://img.shields.io/badge/lang-C%2B%2B17-blue)
![ui](https://img.shields.io/badge/UI-Qt6-green)

## Features

- **Media import** via FFmpeg: MP4, MOV, MKV, AVI, WebM video; PNG/JPEG/BMP/WebP images; MP3/WAV/AAC/FLAC/OGG audio.
- **Drag-and-drop import** straight onto the main window.
- **Timeline**: 6 pre-seeded tracks (2 video + 2 image + 2 audio), add/remove tracks, multi-track clips.
- **Clip operations**: drag to move, drag edges to trim, snap to clip edges and playhead, per-clip thumbnail/waveform.
- **Preview window**: frame-accurate scrubbing, play/pause (Space), aspect-ratio preserving fit.
- **Track controls**: per-track mute / visibility / lock with visual state.
- **Properties inspector**: position X/Y, scale, opacity (video/image); volume, pan (audio); start time + duration for all clips. Edits push onto the undo stack and reflect immediately.
- **Undo / Redo** for all edits (Ctrl+Z / Ctrl+Y).
- **Project save/load** to `.veproj` (JSON) files (Ctrl+S / Ctrl+O).
- **Export / Render** to MP4 (H.264) via FFmpeg CLI filter pipeline (Ctrl+E).
- **Dark theme** with modern, accent-cool palette.

## Keyboard shortcuts

| Shortcut | Action |
|----------|--------|
| Space | Play / Pause |
| Delete | Delete selected clip |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Ctrl+S | Save project |
| Ctrl+O | Open project |
| Ctrl+I | Import media |
| Ctrl+E | Export / Render |

## Build

### Linux

```bash
sudo apt-get install -y \
    build-essential cmake ninja-build pkg-config \
    qt6-base-dev qt6-base-dev-tools libqt6svg6-dev \
    libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libswresample-dev

cmake -S . -B build -G Ninja
cmake --build build -j
./build/bin/VideoEditor
```

### Windows

1. Install **Visual Studio 2022** with the "Desktop development with C++" workload.
2. Install **Qt 6.7+** via the Qt online installer (MSVC 2022 64-bit).
3. Download a prebuilt **FFmpeg shared** build (e.g. from <https://www.gyan.dev/ffmpeg/builds/> or <https://github.com/BtbN/FFmpeg-Builds/releases>) and unzip it to `third_party/ffmpeg` so that `third_party/ffmpeg/include`, `third_party/ffmpeg/lib`, and `third_party/ffmpeg/bin` all exist.
4. Build:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2022_64"
cmake --build build --config Release
.\build\bin\Release\VideoEditor.exe
```

The CMake script copies FFmpeg DLLs next to the executable automatically.

## Project layout

```
.
├── CMakeLists.txt
├── .github/workflows/
│   └── build.yml           # matrix CI (Ubuntu + Windows) + artifacts
├── resources/
│   ├── resources.qrc
│   └── styles/dark.qss
├── src/
│   ├── main.cpp
│   ├── core/               # Project / Timeline / Track / Clip / Command
│   ├── media/              # FFmpeg-backed MediaDecoder
│   ├── ui/                 # MainWindow, Toolbar, Preview, Timeline, Properties
│   └── utils/              # JsonSerializer (.veproj)
└── third_party/ffmpeg/     # Windows only: extracted FFmpeg shared build
```

## Project file format

`.veproj` files are JSON:

```json
{
  "version": 1,
  "filePath": "/path/to/project.veproj",
  "exportWidth": 1920,
  "exportHeight": 1080,
  "exportFps": 30,
  "exportBitrateKbps": 8000,
  "exportFormat": "mp4",
  "timeline": {
    "pps": 50.0,
    "tracks": [
      { "name": "Video", "kind": 0, "muted": false, "locked": false, "visible": true,
        "clips": [
          { "id": "…", "sourcePath": "…", "type": 0,
            "sourceIn": 0.0, "sourceOut": 12.0, "timelineStart": 0.0,
            "posX": 0.0, "posY": 0.0, "scale": 1.0, "opacity": 1.0,
            "volume": 1.0, "pan": 0.0 }
        ]
      }
    ]
  }
}
```

## License

MIT.
