#pragma once

#include <QMainWindow>
#include <QUndoStack>
#include "core/Project.h"

class QAction;
class QLabel;

namespace ve {

class Toolbar;
class PreviewWidget;
class TimelineWidget;
class PropertiesPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    Project* project() { return project_; }
    QUndoStack* undoStack() { return undoStack_; }

    void importFiles(const QStringList& files);
    void exportProject();
    void saveProject();
    void saveProjectAs();
    void loadProject();
    void newProject();

protected:
    void closeEvent(QCloseEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

private:
    void setupMenus();
    void setupActions();
    void setupUi();
    void updateWindowTitle();
    void maybeGenerateThumbnailsForNewClip(Clip* clip);

    Project*          project_;
    QUndoStack*       undoStack_;

    Toolbar*          toolbar_       = nullptr;
    PreviewWidget*    preview_       = nullptr;
    TimelineWidget*   timeline_      = nullptr;
    PropertiesPanel*  properties_    = nullptr;

    // Menu actions
    QAction* actImport_   = nullptr;
    QAction* actExport_   = nullptr;
    QAction* actSave_     = nullptr;
    QAction* actSaveAs_   = nullptr;
    QAction* actOpen_     = nullptr;
    QAction* actNew_      = nullptr;
    QAction* actUndo_     = nullptr;
    QAction* actRedo_     = nullptr;
    QAction* actDelete_   = nullptr;
    QAction* actPlay_     = nullptr;
};

} // namespace ve
