#include "ui/bin/BinWidget.h"
#include "ui/bin/BinTreeView.h"
#include "model/BinModel.h"
#include "model/BinClip.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QDebug>

namespace ve {

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

    auto* tree = new BinTreeView(this);
    tree->setModel(bin_.get());
    v->addWidget(tree, 1);
    tree_ = tree;

    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tree, &QWidget::customContextMenuRequested, this, [this, tree](const QPoint& p) {
        QModelIndex ix = tree->indexAt(p);
        QMenu menu(tree);
        auto* aAddFolder = menu.addAction("Add Folder");
        auto* aRemove = menu.addAction("Remove");
        aRemove->setEnabled(ix.isValid());
        QAction* sel = menu.exec(tree->viewport()->mapToGlobal(p));
        if (sel == aAddFolder) {
            QString name = QInputDialog::getText(tree, "Add Folder", "Folder name:");
            if (!name.isEmpty()) bin_->addFolder(name);
        } else if (sel == aRemove && ix.isValid()) {
            bin_->removeItem(ix.data(BinModel::IdRole).toString());
        }
    });

    connect(tree, &BinTreeView::clipActivated, this, &BinWidget::clipActivated);
}

void BinWidget::setBin(std::shared_ptr<BinModel> bin) {
    bin_ = bin;
    tree_->setModel(bin_.get());
}

} // namespace ve
