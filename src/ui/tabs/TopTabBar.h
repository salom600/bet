#pragma once

#include <QWidget>
#include <QList>
#include <QString>
#include <QIcon>

class QHBoxLayout;
class QToolButton;
class QButtonGroup;
class QLabel;

namespace ve {

/// Horizontal tab bar at the top of the window, CapCut-style.
/// Tabs: Project | Media | Text | Stickers | Transitions | Videos | Audio |
///       Filters | Effects | Adjust
///
/// Clicking a tab changes the content of the LEFT panel (bin / text library /
/// transitions library / etc.) and/or the RIGHT panel (Adjust tab opens
/// color grading sliders).
class TopTabBar : public QWidget {
    Q_OBJECT
public:
    enum class Tab {
        Project = 0,
        Media,
        Text,
        Stickers,
        Transitions,
        Videos,
        Audio,
        Filters,
        Effects,
        Adjust,
    };

    explicit TopTabBar(QWidget* parent = nullptr);

    Tab currentTab() const { return current_; }
    void setCurrentTab(Tab t);

    static QString tabName(Tab t);
    static QIcon tabIcon(Tab t);

signals:
    void tabChanged(Tab newTab);

private:
    void onButtonClicked(int id);

    QHBoxLayout*  layout_     = nullptr;
    QButtonGroup* group_      = nullptr;
    QList<QToolButton*> buttons_;
    Tab current_ = Tab::Media;
};

} // namespace ve
