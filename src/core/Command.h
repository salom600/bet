#pragma once

#include <QUndoCommand>
#include <functional>

namespace ve {

/// Generic reversible command backed by two lambdas (redo/undo).
class LambdaCommand : public QUndoCommand {
public:
    using Fn = std::function<void()>;

    LambdaCommand(const QString& text, Fn redo, Fn undo, QUndoCommand* parent = nullptr)
        : QUndoCommand(parent)
        , redo_(std::move(redo))
        , undo_(std::move(undo))
    {
        setText(text);
    }

    void undo() override { if (undo_) undo_(); }
    void redo() override { if (redo_) redo_(); }

private:
    Fn redo_;
    Fn undo_;
};

} // namespace ve
