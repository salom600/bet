#include "ui/Toolbar.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <functional>

namespace ve {

Toolbar::Toolbar(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(6);

    auto makeBtn = [this](const QString& text, const QString& tooltip, std::function<void()> cb) {
        auto* b = new QPushButton(text, this);
        b->setToolTip(tooltip);
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(32);
        b->setMinimumWidth(36);
        connect(b, &QPushButton::clicked, this, [cb]() { cb(); });
        return b;
    };

    auto* bImport  = makeBtn("Import",  "Import media files (Ctrl+I)", [this]() { emit importClicked(); });
    auto* bExport  = makeBtn("Export",  "Export / Render (Ctrl+E)",    [this]() { emit exportClicked(); });
    auto* bSkipS   = makeBtn(QStringLiteral("\u23EE"), "Skip to start", [this]() { emit skipStartClicked(); });
    auto* bPlay    = makeBtn(QStringLiteral("\u25B6"), "Play / Pause (Space)", [this]() { emit playClicked(); });
    auto* bSkipE   = makeBtn(QStringLiteral("\u23ED"), "Skip to end",   [this]() { emit skipEndClicked(); });
    auto* bStop    = makeBtn(QStringLiteral("\u23F9"), "Stop",          [this]() { emit stopClicked(); });
    auto* bUndo    = makeBtn(QStringLiteral("\u21B6"), "Undo (Ctrl+Z)", [this]() { emit undoClicked(); });
    auto* bRedo    = makeBtn(QStringLiteral("\u21B7"), "Redo (Ctrl+Y)", [this]() { emit redoClicked(); });
    auto* bAddV    = makeBtn("+V",      "Add video track", [this]() { emit addVideoTrack(); });
    auto* bAddA    = makeBtn("+A",      "Add audio track", [this]() { emit addAudioTrack(); });
    auto* bZoomIn  = makeBtn("Zoom +",  "Zoom timeline in",  [this]() { emit zoomIn(); });
    auto* bZoomOut = makeBtn("Zoom \xE2\x88\x92", "Zoom timeline out", [this]() { emit zoomOut(); });

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

} // namespace ve
