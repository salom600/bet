#include "ui/bin/BinWidget.h"
#include "ui/bin/BinTreeView.h"
#include "ui/bin/BinItemDelegate.h"
#include "model/BinModel.h"
#include "model/BinClip.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QToolButton>
#include <QStackedWidget>
#include <QListView>
#include <QTreeView>
#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QApplication>

namespace ve {

// Custom QListView that supports drag of bin clips
class BinIconView : public QListView {
    Q_OBJECT
public:
    explicit BinIconView(QWidget* parent = nullptr) : QListView(parent) {
        setViewMode(QListView::IconMode);
        setGridSize(QSize(140, 120));
        setIconSize(QSize(120, 80));
        setMovement(QListView::Static);
        setResizeMode(QListView::Adjust);
        setUniformItemSizes(true);
        setWordWrap(true);
        setDragEnabled(true);
        setAcceptDrops(false);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);
    }

signals:
    void clipActivated(const QString& binClipId);

protected:
    void mouseDoubleClickEvent(QMouseEvent* e) override {
        QModelIndex ix = indexAt(e->pos());
        if (ix.isValid() && ix.data(BinModel::TypeRole).toString() == "clip") {
            emit clipActivated(ix.data(BinModel::IdRole).toString());
        }
        QListView::mouseDoubleClickEvent(e);
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
        // Set a drag pixmap from the clip thumbnail if available
        QVariant thumbVar = ix.data(BinModel::ThumbnailRole);
        if (thumbVar.isValid() && thumbVar.canConvert<QImage>()) {
            QImage img = thumbVar.value<QImage>();
            if (!img.isNull()) {
                QPixmap pm = QPixmap::fromImage(img).scaled(120, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                drag->setPixmap(pm);
                drag->setHotSpot(QPoint(pm.width()/2, pm.height()/2));
            }
        }
        drag->exec(Qt::CopyAction);
        Q_UNUSED(supportedActions);
    }
};

BinWidget::BinWidget(std::shared_ptr<BinModel> bin, QWidget* parent)
    : QWidget(parent)
    , bin_(bin)
{
    qDebug() << "BinWidget: ctor entered, bin_.get() =" << bin_.get();
    if (!bin_) {
        qCritical() << "BinWidget: bin_ is null!";
        return;
    }

    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    // Panel header
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(8, 6, 8, 6);
    headerLayout->setSpacing(4);
    auto* titleLabel = new QLabel("Project Bin", this);
    titleLabel->setObjectName("panelHeader");
    titleLabel->setStyleSheet("font-weight: bold; color: #5ac8fa; font-size: 11pt; padding: 0;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);

    // View mode toggle buttons
    btnIconView_ = new QToolButton(this);
    btnIconView_->setIcon(QIcon(":/icons/image.svg"));
    btnIconView_->setIconSize(QSize(16, 16));
    btnIconView_->setToolTip("Icon view");
    btnIconView_->setCheckable(true);
    btnIconView_->setChecked(true);
    btnIconView_->setAutoRaise(true);
    btnIconView_->setFixedSize(28, 28);

    btnListView_ = new QToolButton(this);
    btnListView_->setIcon(QIcon(":/icons/film.svg"));
    btnListView_->setIconSize(QSize(16, 16));
    btnListView_->setToolTip("List view");
    btnListView_->setCheckable(true);
    btnListView_->setAutoRaise(true);
    btnListView_->setFixedSize(28, 28);

    btnAddFolder_ = new QToolButton(this);
    btnAddFolder_->setIcon(QIcon(":/icons/new.svg"));
    btnAddFolder_->setIconSize(QSize(16, 16));
    btnAddFolder_->setToolTip("Add folder");
    btnAddFolder_->setAutoRaise(true);
    btnAddFolder_->setFixedSize(28, 28);

    btnRemove_ = new QToolButton(this);
    btnRemove_->setIcon(QIcon(":/icons/delete.svg"));
    btnRemove_->setIconSize(QSize(16, 16));
    btnRemove_->setToolTip("Remove selected item");
    btnRemove_->setAutoRaise(true);
    btnRemove_->setFixedSize(28, 28);

    headerLayout->addWidget(btnAddFolder_);
    headerLayout->addWidget(btnRemove_);
    headerLayout->addWidget(btnIconView_);
    headerLayout->addWidget(btnListView_);

    auto* headerWidget = new QWidget(this);
    headerWidget->setObjectName("panelHeader");
    headerWidget->setStyleSheet("background-color: #1c1f25; border-bottom: 1px solid #2a2d33;");
    headerWidget->setLayout(headerLayout);
    v->addWidget(headerWidget);

    // Stacked widget for icon view / list view
    stack_ = new QStackedWidget(this);

    iconView_ = new BinIconView(stack_);
    iconDelegate_ = new BinItemDelegate(iconView_);
    iconView_->setItemDelegate(iconDelegate_);
    stack_->addWidget(iconView_);

    treeView_ = new BinTreeView(stack_);
    stack_->addWidget(treeView_);

    v->addWidget(stack_, 1);

    // Set models
    iconView_->setModel(bin_.get());
    treeView_->setModel(bin_.get());
    if (treeView_->header()) {
        treeView_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }

    // Default to icon view
    stack_->setCurrentWidget(iconView_);

    qDebug() << "BinWidget: setting up connections...";

    // Connections
    connect(btnIconView_, &QToolButton::clicked, this, [this]() { setViewMode(ViewMode::IconView); });
    connect(btnListView_, &QToolButton::clicked, this, [this]() { setViewMode(ViewMode::ListView); });
    connect(btnAddFolder_, &QToolButton::clicked, this, &BinWidget::onAddFolder);
    connect(btnRemove_, &QToolButton::clicked, this, &BinWidget::onRemoveItem);

    // Clip activation (double-click)
    connect(iconView_, &BinIconView::clipActivated, this, &BinWidget::clipActivated);
    auto* tv = qobject_cast<BinTreeView*>(treeView_);
    if (tv) connect(tv, &BinTreeView::clipActivated, this, &BinWidget::clipActivated);

    // Context menu on tree view
    treeView_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView_, &QWidget::customContextMenuRequested, this, [this](const QPoint& p) {
        QModelIndex ix = treeView_->indexAt(p);
        QMenu menu(treeView_);
        auto* aAddFolder = menu.addAction(QIcon(":/icons/new.svg"), "Add Folder");
        auto* aRemove = menu.addAction(QIcon(":/icons/delete.svg"), "Remove");
        aRemove->setEnabled(ix.isValid());
        QAction* sel = menu.exec(treeView_->viewport()->mapToGlobal(p));
        if (sel == aAddFolder) onAddFolder();
        else if (sel == aRemove && ix.isValid()) onRemoveItem();
    });

    qDebug() << "BinWidget: ctor complete.";
}

void BinWidget::setBin(std::shared_ptr<BinModel> bin) {
    bin_ = bin;
    iconView_->setModel(bin_.get());
    treeView_->setModel(bin_.get());
}

void BinWidget::setViewMode(ViewMode mode) {
    if (mode == ViewMode::IconView) {
        stack_->setCurrentWidget(iconView_);
        btnIconView_->setChecked(true);
        btnListView_->setChecked(false);
    } else {
        stack_->setCurrentWidget(treeView_);
        btnIconView_->setChecked(false);
        btnListView_->setChecked(true);
    }
}

void BinWidget::onAddFolder() {
    QString name = QInputDialog::getText(this, "Add Folder", "Folder name:");
    if (!name.isEmpty()) bin_->addFolder(name);
}

void BinWidget::onRemoveItem() {
    QWidget* current = stack_->currentWidget();
    QModelIndex ix;
    if (current == iconView_) ix = iconView_->currentIndex();
    else ix = treeView_->currentIndex();
    if (ix.isValid()) bin_->removeItem(ix.data(BinModel::IdRole).toString());
}

} // namespace ve

#include "BinWidget.moc"
