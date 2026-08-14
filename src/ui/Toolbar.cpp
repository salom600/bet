#include "ui/Toolbar.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

namespace ve {

Toolbar::Toolbar(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(6);

    auto* bImport  = makeBtn("Import",  "Import media files (Ctrl+I)", SIGNAL(importClicked()));
    auto* bExport  = makeBtn("Export",  "Export / Render (Ctrl+E)",    SIGNAL(exportClicked()));
    auto* bSkipS   = makeBtn("⏮",       "Skip to start",               SIGNAL(skipStartClicked()));
    auto* bPlay    = makeBtn("▶",       "Play / Pause (Space)",        SIGNAL(playClicked()));
    auto* bSkipE   = makeBtn("⏭",       "Skip to end",                 SIGNAL(skipEndClicked()));
    auto* bStop    = makeBtn("⏹",       "Stop",                        SIGNAL(stopClicked()));
    auto* bUndo    = makeBtn("↶",       "Undo (Ctrl+Z)",               SIGNAL(undoClicked()));
    auto* bRedo    = makeBtn("↷",       "Redo (Ctrl+Y)",               SIGNAL(redoClicked()));
    auto* bAddV    = makeBtn("+V",      "Add video track",             SIGNAL(addVideoTrack()));
    auto* bAddA    = makeBtn("+A",      "Add audio track",             SIGNAL(addAudioTrack()));
    auto* bZoomIn  = makeBtn("Zoom +",  "Zoom timeline in",            SIGNAL(zoomIn()));
    auto* bZoomOut = makeBtn("Zoom −",  "Zoom timeline out",           SIGNAL(zoomOut()));

    layout->addWidget(bImport);
    layout->addWidget(bExport);
    layout->addSpacing(12);
    layout->addWidget(bSkipS);
    layout->addWidget(bPlay);
    layout->addWidget(bStop);
    layout->addWidget(bSkipE);
    layout->addSpacing(12);
    layout->addWidget(bUndo);
    layout->addWidget(bRedo);
    layout->addSpacing(12);
    layout->addWidget(bAddV);
    layout->addWidget(bAddA);
    layout->addSpacing(12);
    layout->addWidget(bZoomOut);
    layout->addWidget(bZoomIn);
    layout->addStretch(1);

    auto* brand = new QLabel("<b style='color:#5ac8fa'>VideoEditor</b>", this);
    layout->addWidget(brand);
}

QPushButton* Toolbar::makeBtn(const QString& text, const QString& tooltip, const char* signal) {
    auto* b = new QPushButton(text, this);
    b->setToolTip(tooltip);
    b->setCursor(Qt::PointingHandCursor);
    b->setFixedHeight(32);
    b->setMinimumWidth(36);
    connect(b, &QPushButton::clicked, this, signal);
    return b;
}

} // namespace ve
