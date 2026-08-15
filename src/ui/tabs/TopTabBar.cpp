#include "ui/tabs/TopTabBar.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QButtonGroup>
#include <QLabel>
#include <QFrame>

namespace ve {

QString TopTabBar::tabName(Tab t) {
    switch (t) {
        case Tab::Project:     return "Project";
        case Tab::Media:       return "Media";
        case Tab::Text:        return "Text";
        case Tab::Stickers:    return "Stickers";
        case Tab::Transitions: return "Transitions";
        case Tab::Videos:      return "Videos";
        case Tab::Audio:       return "Audio";
        case Tab::Filters:     return "Filters";
        case Tab::Effects:     return "Effects";
        case Tab::Adjust:      return "Adjust";
    }
    return {};
}

QIcon TopTabBar::tabIcon(Tab t) {
    // Use our existing SVG icon set; map tabs to icons
    switch (t) {
        case Tab::Project:     return QIcon(":/icons/new.svg");
        case Tab::Media:       return QIcon(":/icons/film.svg");
        case Tab::Text:        return QIcon(":/icons/image.svg");
        case Tab::Stickers:    return QIcon(":/icons/image.svg");
        case Tab::Transitions: return QIcon(":/icons/import.svg");
        case Tab::Videos:      return QIcon(":/icons/video.svg");
        case Tab::Audio:       return QIcon(":/icons/add-audio-track.svg");
        case Tab::Filters:     return QIcon(":/icons/zoom-in.svg");
        case Tab::Effects:     return QIcon(":/icons/import.svg");
        case Tab::Adjust:      return QIcon(":/icons/open.svg");
    }
    return {};
}

TopTabBar::TopTabBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("topTabBar");
    setFixedHeight(40);

    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(8, 0, 8, 0);
    layout_->setSpacing(2);

    group_ = new QButtonGroup(this);
    group_->setExclusive(true);

    // Brand logo on the left
    auto* brand = new QLabel(this);
    brand->setText("<span style='color:#5ac8fa;font-weight:bold;font-size:13pt'>VE</span>");
    brand->setStyleSheet("padding: 0 12px 0 4px;");
    layout_->addWidget(brand);

    // Create one button per tab
    for (int i = 0; i < 10; ++i) {
        Tab t = static_cast<Tab>(i);
        auto* btn = new QToolButton(this);
        btn->setText(tabName(t));
        btn->setIcon(tabIcon(t));
        btn->setIconSize(QSize(16, 16));
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setCheckable(true);
        btn->setAutoRaise(true);
        btn->setStyleSheet(
            "QToolButton { "
            "  color: #8a8d96; "
            "  background: transparent; "
            "  border: none; "
            "  border-bottom: 2px solid transparent; "
            "  padding: 6px 14px; "
            "  font-size: 10pt; "
            "} "
            "QToolButton:hover { color: #e0e0e6; background: #1c1f25; } "
            "QToolButton:checked { "
            "  color: #5ac8fa; "
            "  border-bottom: 2px solid #5ac8fa; "
            "  background: #1c1f25; "
            "}");
        group_->addButton(btn, i);
        buttons_.append(btn);
        layout_->addWidget(btn);
    }

    layout_->addStretch(1);

    // Project metadata on the right (CapCut-style)
    auto* meta = new QLabel(this);
    meta->setText("<span style='color:#8a8d96'>Project 01 · </span>"
                  "<span style='color:#e0e0e6'>Untitled</span>");
    meta->setStyleSheet("padding: 0 12px;");
    layout_->addWidget(meta);

    connect(group_, &QButtonGroup::idClicked, this, &TopTabBar::onButtonClicked);

    // Default to Media tab
    setCurrentTab(Tab::Media);
}

void TopTabBar::setCurrentTab(Tab t) {
    if (current_ == t) return;
    current_ = t;
    int idx = static_cast<int>(t);
    if (idx >= 0 && idx < buttons_.size()) {
        buttons_[idx]->setChecked(true);
    }
    emit tabChanged(t);
}

void TopTabBar::onButtonClicked(int id) {
    Tab t = static_cast<Tab>(id);
    if (current_ != t) {
        current_ = t;
        emit tabChanged(t);
    }
}

} // namespace ve
