#include "ui/tools/ToolStrip.h"

#include <QVBoxLayout>
#include <QToolButton>
#include <QButtonGroup>

namespace ve {

QString ToolStrip::toolName(Tool t) {
    switch (t) {
        case Tool::Select:  return "Select";
        case Tool::Move:    return "Move";
        case Tool::Crop:    return "Crop";
        case Tool::Razor:   return "Razor";
        case Tool::Text:    return "Text";
        case Tool::Sticker: return "Sticker";
        case Tool::Audio:   return "Audio";
        case Tool::Filter:  return "Filter";
    }
    return {};
}

QString ToolStrip::toolIcon(Tool t) {
    switch (t) {
        case Tool::Select:  return ":/icons/open.svg";           // magnifier-ish
        case Tool::Move:    return ":/icons/import.svg";
        case Tool::Crop:    return ":/icons/zoom-in.svg";
        case Tool::Razor:   return ":/icons/delete.svg";         // cut-like
        case Tool::Text:    return ":/icons/image.svg";
        case Tool::Sticker: return ":/icons/image.svg";
        case Tool::Audio:   return ":/icons/add-audio-track.svg";
        case Tool::Filter:  return ":/icons/zoom-out.svg";
    }
    return {};
}

ToolStrip::ToolStrip(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("toolStrip");
    setFixedWidth(48);

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(4, 8, 4, 8);
    layout_->setSpacing(4);

    auto* group = new QButtonGroup(this);
    group->setExclusive(true);

    for (int i = 0; i < 8; ++i) {
        Tool t = static_cast<Tool>(i);
        auto* btn = new QToolButton(this);
        btn->setIcon(QIcon(toolIcon(t)));
        btn->setIconSize(QSize(20, 20));
        btn->setToolTip(toolName(t));
        btn->setCheckable(true);
        btn->setAutoRaise(true);
        btn->setFixedSize(40, 36);
        btn->setStyleSheet(
            "QToolButton { "
            "  background: transparent; "
            "  border: none; "
            "  border-radius: 4px; "
            "} "
            "QToolButton:hover { background: #2a2d33; } "
            "QToolButton:checked { background: #5ac8fa; }");
        group->addButton(btn, i);
        buttons_.append(btn);
        layout_->addWidget(btn);

        connect(btn, &QToolButton::clicked, this, [this, t]() {
            setCurrentTool(t);
        });
    }

    layout_->addStretch(1);

    // Default tool = Select
    setCurrentTool(Tool::Select);
}

void ToolStrip::setCurrentTool(Tool t) {
    if (current_ == t) return;
    current_ = t;
    int idx = static_cast<int>(t);
    if (idx >= 0 && idx < buttons_.size()) {
        buttons_[idx]->setChecked(true);
    }
    emit toolChanged(t);
}

} // namespace ve
