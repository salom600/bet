#pragma once

#include <QWidget>
#include <QList>

class QToolButton;
class QVBoxLayout;

namespace ve {

/// Vertical tool strip on the far left of the window, CapCut-style.
/// Tools: Select, Move, Crop, Razor (split), Text, Sticker, Audio, Filter
///
/// The selected tool determines the cursor and behavior when clicking on
/// the timeline (e.g. Razor splits clips, Select moves them).
class ToolStrip : public QWidget {
    Q_OBJECT
public:
    enum class Tool {
        Select = 0,    // Default arrow - select & move clips
        Move,          // Hand - pan timeline
        Crop,          // Trim/resize
        Razor,         // Split clip at playhead
        Text,          // Add text overlay
        Sticker,       // Add sticker
        Audio,         // Add audio
        Filter,        // Apply filter
    };

    explicit ToolStrip(QWidget* parent = nullptr);

    Tool currentTool() const { return current_; }
    void setCurrentTool(Tool t);

    static QString toolName(Tool t);
    static QString toolIcon(Tool t);

signals:
    void toolChanged(Tool newTool);

private:
    QVBoxLayout* layout_ = nullptr;
    QList<QToolButton*> buttons_;
    Tool current_ = Tool::Select;
};

} // namespace ve
