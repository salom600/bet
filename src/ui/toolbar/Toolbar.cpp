#include "ui/toolbar/Toolbar.h"
#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QFrame>

namespace ve {

Toolbar::Toolbar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("toolbar");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(4);

    // File operations group
    auto* bImport = makeIconButton(":/icons/import.svg", "Import media files (Ctrl+I)", "Import");
    auto* bExport = makeIconButton(":/icons/export.svg", "Export / Render (Ctrl+E)", "Export");

    // Playback group
    auto* bSkipS = makeIconButton(":/icons/skip-start.svg", "Skip to start");
    auto* bPlay  = makeIconButton(":/icons/play.svg", "Play / Pause (Space)");
    auto* bStop  = makeIconButton(":/icons/stop.svg", "Stop");
    auto* bSkipE = makeIconButton(":/icons/skip-end.svg", "Skip to end");

    // Edit group
    auto* bUndo    = makeIconButton(":/icons/undo.svg", "Undo (Ctrl+Z)");
    auto* bRedo    = makeIconButton(":/icons/redo.svg", "Redo (Ctrl+Y)");
    auto* bDelete  = makeIconButton(":/icons/delete.svg", "Delete selected clip (Del)");

    // Track group
    auto* bAddV = makeIconButton(":/icons/add-video-track.svg", "Add video track");
    auto* bAddA = makeIconButton(":/icons/add-audio-track.svg", "Add audio track");

    // Zoom group
    auto* bZoomIn  = makeIconButton(":/icons/zoom-in.svg", "Zoom timeline in");
    auto* bZoomOut = makeIconButton(":/icons/zoom-out.svg", "Zoom timeline out");

    layout->addWidget(bImport);
    layout->addWidget(bExport);
    layout->addWidget(makeDivider());
    layout->addWidget(bSkipS);
    layout->addWidget(bPlay);
    layout->addWidget(bStop);
    layout->addWidget(bSkipE);
    layout->addWidget(makeDivider());
    layout->addWidget(bUndo);
    layout->addWidget(bRedo);
    layout->addWidget(bDelete);
    layout->addWidget(makeDivider());
    layout->addWidget(bAddV);
    layout->addWidget(bAddA);
    layout->addWidget(makeDivider());
    layout->addWidget(bZoomOut);
    layout->addWidget(bZoomIn);
    layout->addStretch(1);

    // Brand label on the right
    auto* brand = new QLabel(this);
    brand->setText("<span style='color:#5ac8fa;font-weight:bold;font-size:13pt'>Video</span>"
                   "<span style='color:#e0e0e6;font-weight:bold;font-size:13pt'>Editor</span>"
                   "<span style='color:#5a5d65;font-size:9pt;margin-left:8px'>v0.3.0</span>");
    brand->setAlignment(Qt::AlignCenter);
    layout->addWidget(brand);

    // Wire signals
    connect(bImport,  &QToolButton::clicked, this, &Toolbar::importClicked);
    connect(bExport,  &QToolButton::clicked, this, &Toolbar::exportClicked);
    connect(bSkipS,   &QToolButton::clicked, this, &Toolbar::skipStartClicked);
    connect(bPlay,    &QToolButton::clicked, this, &Toolbar::playClicked);
    connect(bStop,    &QToolButton::clicked, this, &Toolbar::stopClicked);
    connect(bSkipE,   &QToolButton::clicked, this, &Toolbar::skipEndClicked);
    connect(bUndo,    &QToolButton::clicked, this, &Toolbar::undoClicked);
    connect(bRedo,    &QToolButton::clicked, this, &Toolbar::redoClicked);
    connect(bDelete,  &QToolButton::clicked, this, &Toolbar::deleteClicked);
    connect(bAddV,    &QToolButton::clicked, this, &Toolbar::addVideoTrack);
    connect(bAddA,    &QToolButton::clicked, this, &Toolbar::addAudioTrack);
    connect(bZoomIn,  &QToolButton::clicked, this, &Toolbar::zoomIn);
    connect(bZoomOut, &QToolButton::clicked, this, &Toolbar::zoomOut);
}

QToolButton* Toolbar::makeIconButton(const QString& iconPath, const QString& tooltip,
                                      const QString& text) {
    auto* b = new QToolButton(this);
    b->setIcon(QIcon(iconPath));
    b->setIconSize(QSize(20, 20));
    b->setToolTip(tooltip);
    b->setCursor(Qt::PointingHandCursor);
    b->setFixedSize(QSize(36, 32));
    b->setAutoRaise(true);
    if (!text.isEmpty()) {
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        b->setText(text);
        b->setFixedSize(QSize(96, 32));
    } else {
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    b->setStyleSheet(
        "QToolButton { background: transparent; border: none; border-radius: 4px; padding: 4px; }"
        "QToolButton:hover { background: #2a2d33; }"
        "QToolButton:pressed { background: #5ac8fa; }"
        "QToolButton:checked { background: #5ac8fa; }");
    return b;
}

QFrame* Toolbar::makeDivider() {
    auto* div = new QFrame(this);
    div->setFrameShape(QFrame::VLine);
    div->setFrameShadow(QFrame::Sunken);
    div->setFixedWidth(1);
    div->setStyleSheet("color: #2a2d33;");
    return div;
}

} // namespace ve
