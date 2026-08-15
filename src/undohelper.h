/*
 * VideoEditor - undohelper.h
 * Lambda-based undo system, adapted from Kdenlive's src/undohelper.hpp.
 *
 * The key idea: every mutating operation on the model is expressed as a pair
 * of `Fun` objects (undo and redo). A complex operation composes simple
 * operations by chaining their lambdas with PUSH_LAMBDA. If any step fails
 * (returns false), the partial undo lambda is executed to roll back.
 */
#pragma once

#include <functional>
#include <QUndoCommand>
#include <QString>

namespace ve {

/// A unit of reversible work. Returns true on success, false on failure.
using Fun = std::function<bool()>;

/// Empty lambda that always succeeds. Used as the seed for composition.
inline Fun noop() { return []() { return true; }; }

/// Compose: after `lambda` runs, also run `operation`.
/// The composed lambda succeeds only if both succeed.
#define PUSH_LAMBDA(operation, lambda)                                          \
    lambda = [lambda, operation]() {                                            \
        bool v = lambda();                                                      \
        return v && operation();                                                \
    };

/// Compose: run `operation` first, then `lambda`.
#define PUSH_FRONT_LAMBDA(operation, lambda)                                    \
    lambda = [lambda, operation]() {                                            \
        bool v = operation();                                                   \
        return v && lambda();                                                   \
    };

/**
 * @brief A QUndoCommand backed by two Fun objects.
 *
 * QUndoStack calls redo() once on push(), which is bad for us because we
 * already executed the operation while building the lambda. So we use a
 * small `m_firstRedo` guard: the first redo() is a no-op, subsequent
 * redo()s actually run the redo lambda. undo() always runs the undo lambda.
 *
 * This mirrors Kdenlive's FunctionalUndoCommand exactly.
 */
class FunctionalUndoCommand : public QUndoCommand {
public:
    FunctionalUndoCommand(Fun undo, Fun redo, const QString& text,
                          QUndoCommand* parent = nullptr)
        : QUndoCommand(parent)
        , m_undo(std::move(undo))
        , m_redo(std::move(redo))
        , m_firstRedo(true)
    {
        setText(text);
    }

    void undo() override {
        if (m_undo) m_undo();
    }

    void redo() override {
        if (m_firstRedo) {
            m_firstRedo = false;
            return;
        }
        if (m_redo) m_redo();
    }

private:
    Fun  m_undo;
    Fun  m_redo;
    bool m_firstRedo;
};

} // namespace ve
