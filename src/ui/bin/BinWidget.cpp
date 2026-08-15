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
    qDebug() << "BinWidget: ctor entered, bin_.get() =" << bin_.get();
    if (!bin_) {
        qCritical() << "BinWidget: bin_ is null!";
        return;
    }

    qDebug() << "BinWidget: creating QVBoxLayout...";
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    qDebug() << "BinWidget: creating QLabel title...";
    auto* title = new QLabel("  Bin", this);
    QFont f = title->font(); f.setBold(true); f.setPointSize(11);
    title->setFont(f);
    title->setStyleSheet("background-color: #262931; color: #5ac8fa; padding: 6px;");
    v->addWidget(title);

    qDebug() << "BinWidget: creating BinTreeView...";
    auto* tree = new BinTreeView(this);
    qDebug() << "BinWidget: BinTreeView created. Setting model...";
    tree->setModel(bin_.get());
    qDebug() << "BinWidget: model set. Configuring header section resize...";
    // Now that the model is set, we can configure section resize mode safely.
    if (tree->header()) {
        tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    qDebug() << "BinWidget: header configured. Adding tree to layout.";
    v->addWidget(tree, 1);
    tree_ = tree;

    qDebug() << "BinWidget: setting context menu policy...";
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

    qDebug() << "BinWidget: connecting clipActivated...";
    connect(tree, &BinTreeView::clipActivated, this, &BinWidget::clipActivated);
    qDebug() << "BinWidget: ctor complete.";
}

void BinWidget::setBin(std::shared_ptr<BinModel> bin) {
    bin_ = bin;
    tree_->setModel(bin_.get());
}

} // namespace ve
