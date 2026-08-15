#include "ui/bin/BinTreeView.h"
#include "model/BinModel.h"
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QModelIndex>
#include <QHeaderView>
#include <QDebug>

namespace ve {

BinTreeView::BinTreeView(QWidget* parent)
    : QTreeView(parent)
{
    qDebug() << "BinTreeView: ctor entered";
    setDragEnabled(true);
    qDebug() << "BinTreeView: setDragEnabled OK";
    setAcceptDrops(true);
    setDefaultDropAction(Qt::CopyAction);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setRootIsDecorated(true);
    setUniformRowHeights(true);
    setHeaderHidden(false);
    qDebug() << "BinTreeView: basic settings OK";

    // Configure header AFTER setting a model. QHeaderView::setSectionResizeMode
    // requires a model to be set on the header; calling it before setModel()
    // can trigger null-pointer derefs in Qt's internal layout recalculation.
    // We defer the section resize mode configuration until after setModel()
    // is called by BinWidget.

    QHeaderView* hdr = header();
    qDebug() << "BinTreeView: header() =" << hdr;
    if (hdr) {
        hdr->setStretchLastSection(true);
        qDebug() << "BinTreeView: stretchLastSection set";
    }
    qDebug() << "BinTreeView: ctor complete";
}

void BinTreeView::mouseDoubleClickEvent(QMouseEvent* e) {
    QModelIndex ix = indexAt(e->pos());
    if (ix.isValid() && ix.data(BinModel::TypeRole).toString() == "clip") {
        emit clipActivated(ix.data(BinModel::IdRole).toString());
    }
    QTreeView::mouseDoubleClickEvent(e);
}

void BinTreeView::startDrag(Qt::DropActions supportedActions) {
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

} // namespace ve
