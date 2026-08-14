#include "ui/bin/BinWidget.h"
#include "model/BinModel.h"
#include "model/BinClip.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QDebug>

namespace ve {

// Subclass QTreeView to emit a custom signal on double-click and to
// start drags with the right mime type.
class BinTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit BinTreeView(QWidget* parent = nullptr) : QTreeView(parent) {
        setDragEnabled(true);
        setAcceptDrops(true);
        setDefaultDropAction(Qt::CopyAction);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setRootIsDecorated(true);
        setUniformRowHeights(true);
        setHeaderHidden(false);
        header()->setStretchLastSection(true);
        header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }

signals:
    void clipActivated(const QString& binClipId);
    void clipDroppedOnTimeline(const QString& binClipId, int positionFrames);

protected:
    void mouseDoubleClickEvent(QMouseEvent* e) override {
        QModelIndex ix = indexAt(e->pos());
        if (ix.isValid() && ix.data(BinModel::TypeRole).toString() == "clip") {
            emit clipActivated(ix.data(BinModel::IdRole).toString());
        }
        QTreeView::mouseDoubleClickEvent(e);
    }

    void startDrag(Qt::DropActions supportedActions) override {
        QModelIndex ix = currentIndex();
        if (!ix.isValid()) return;
        if (ix.data(BinModel::TypeRole).toString() != "clip") return;
        QString clipId = ix.data(BinModel::IdRole).toString();

        auto* mime = new QMimeData;
        mime->setData("application/x-ve-binclip", clipId.toUtf8());
        auto* drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->exec(Qt::CopyAction);
        Q_UNUSED(supportedActions);
    }
};

BinWidget::BinWidget(std::shared_ptr<BinModel> bin, QWidget* parent)
    : QWidget(parent)
    , bin_(bin)
{
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    auto* title = new QLabel("  Bin", this);
    QFont f = title->font(); f.setBold(true); f.setPointSize(11);
    title->setFont(f);
    title->setStyleSheet("background-color: #262931; color: #5ac8fa; padding: 6px;");
    v->addWidget(title);

    tree_ = new BinTreeView(this);
    tree_->setModel(bin_.get());
    v->addWidget(tree_, 1);

    // Context menu for adding folders / removing items
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tree_, &QWidget::customContextMenuRequested, this, [this](const QPoint& p) {
        QModelIndex ix = tree_->indexAt(p);
        QMenu menu(this);
        auto* aAddFolder = menu.addAction("Add Folder");
        auto* aRemove = menu.addAction("Remove");
        aRemove->setEnabled(ix.isValid());
        QAction* sel = menu.exec(tree_->viewport()->mapToGlobal(p));
        if (sel == aAddFolder) {
            QString name = QInputDialog::getText(this, "Add Folder", "Folder name:");
            if (!name.isEmpty()) bin_->addFolder(name);
        } else if (sel == aRemove && ix.isValid()) {
            bin_->removeItem(ix.data(BinModel::IdRole).toString());
        }
    });

    auto* tv = qobject_cast<BinTreeView*>(tree_);
    connect(tv, &BinTreeView::clipActivated, this, &BinWidget::clipActivated);
}

void BinWidget::setBin(std::shared_ptr<BinModel> bin) {
    bin_ = bin;
    tree_->setModel(bin_.get());
}

} // namespace ve
