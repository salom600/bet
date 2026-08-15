/*
 * VideoEditor - MainWindow.h
 * CapCut-style layout: top tab bar + left tool strip + media bin (left) +
 * large center preview + adjust panel (right) + timeline (bottom).
 */
#pragma once

#include <QMainWindow>
#include <QUndoStack>

namespace ve {

class Project;
class Toolbar;
class BinWidget;
class ProjectMonitor;
class TimelineWidget;
class EffectStackView;
class PropertiesPanel;
class TopTabBar;
class ToolStrip;
class TransportBar;
class AdjustPanel;
class ClipMonitorWidget;
class MonitorManager;

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
    void onTabChanged(int tabId);
    void onToolChanged(int toolId);

    Project* project_ = nullptr;

    // New CapCut-style components
    TopTabBar*      topTabBar_    = nullptr;
    ToolStrip*      toolStrip_    = nullptr;
    BinWidget*      binWidget_    = nullptr;
    ProjectMonitor* projectMonitor_ = nullptr;
    TransportBar*   transportBar_ = nullptr;
    AdjustPanel*    adjustPanel_  = nullptr;
    TimelineWidget* timeline_     = nullptr;

    // Legacy (kept for compatibility, will be removed)
    Toolbar*            toolbar_       = nullptr;
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
