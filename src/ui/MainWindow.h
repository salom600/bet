/*
 * VideoEditor - MainWindow.h
 * Top-level window: hosts Bin (left), monitors (top), timeline (bottom),
 * EffectStack + Properties (right).
 *
 * Adapted from Kdenlive's src/mainwindow.h.
 */
#pragma once

#include <QMainWindow>
#include <QUndoStack>

namespace ve {

class Project;
class Toolbar;
class BinWidget;
class ClipMonitorWidget;
class ProjectMonitor;
class MonitorManager;
class TimelineWidget;
class EffectStackView;
class PropertiesPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    Project* project() { return project_; }

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
    void setupActions();
    void setupUi();
    void setupMenus();
    void updateWindowTitle();

    Project* project_ = nullptr;

    Toolbar*            toolbar_       = nullptr;
    BinWidget*          binWidget_     = nullptr;
    ClipMonitorWidget*  clipMonitor_   = nullptr;
    ProjectMonitor*     projectMonitor_ = nullptr;
    MonitorManager*     monitorManager_ = nullptr;
    TimelineWidget*     timeline_      = nullptr;
    EffectStackView*    effectStack_   = nullptr;
    PropertiesPanel*    properties_    = nullptr;

    // Menu actions
    QAction* actImport_ = nullptr;
    QAction* actExport_ = nullptr;
    QAction* actSave_   = nullptr;
    QAction* actSaveAs_ = nullptr;
    QAction* actOpen_   = nullptr;
    QAction* actNew_    = nullptr;
    QAction* actUndo_   = nullptr;
    QAction* actRedo_   = nullptr;
    QAction* actDelete_ = nullptr;
    QAction* actPlay_   = nullptr;
};

} // namespace ve
