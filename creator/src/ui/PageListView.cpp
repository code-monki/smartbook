#include "smartbook/creator/ui/PageListView.h"
#include "smartbook/creator/PageManager.h"  // Includes PageInfo definition
#include <QListView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QModelIndex>
#include <QAbstractItemView>
#include <QDebug>

namespace smartbook {
namespace creator {

PageListView::PageListView(QWidget* parent)
    : QWidget(parent)
    , m_listView(nullptr)
    , m_model(nullptr)
    , m_pageManager(nullptr)
{
    setupUI();
}

void PageListView::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_model = new QStandardItemModel(this);
    m_listView = new QListView(this);
    m_listView->setModel(m_model);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    
    layout->addWidget(m_listView);
    
    connect(m_listView, &QListView::doubleClicked,
            this, &PageListView::onItemDoubleClicked);
    
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_listView, &QListView::customContextMenuRequested,
            this, &PageListView::onContextMenuRequested);
}

void PageListView::setPageManager(PageManager* pageManager) {
    m_pageManager = pageManager;
    if (m_pageManager) {
        connect(m_pageManager, &PageManager::pageListChanged,
                this, &PageListView::refreshPageList);
        refreshPageList();
    }
}

void PageListView::refreshPageList() {
    if (!m_pageManager) {
        return;
    }
    
    m_model->clear();
    
    // Get pages - PageInfo is in creator namespace
    QList<PageInfo> pages = m_pageManager->getPages();
    for (const PageInfo& page : pages) {
        QString displayText = formatPageDisplay(page.pageId, page.pageOrder, page.chapterTitle);
        QStandardItem* item = new QStandardItem(displayText);
        item->setData(page.pageId, Qt::UserRole); // Store page ID
        m_model->appendRow(item);
    }
}

QString PageListView::formatPageDisplay(int /*pageId*/, int pageOrder, const QString& chapterTitle) const {
    if (!chapterTitle.isEmpty()) {
        return QString("Page %1: %2").arg(pageOrder).arg(chapterTitle);
    }
    return QString("Page %1").arg(pageOrder);
}

int PageListView::getSelectedPageId() const {
    QModelIndex current = m_listView->currentIndex();
    if (!current.isValid()) {
        return -1;
    }
    
    return current.data(Qt::UserRole).toInt();
}

void PageListView::onItemDoubleClicked(const QModelIndex& index) {
    int pageId = index.data(Qt::UserRole).toInt();
    if (pageId > 0 && m_pageManager) {
        m_pageManager->setCurrentPage(pageId);
        emit pageSelected(pageId);
    }
}

void PageListView::onContextMenuRequested(const QPoint& pos) {
    QModelIndex index = m_listView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }
    
    int pageId = index.data(Qt::UserRole).toInt();
    if (pageId <= 0) {
        return;
    }
    
    QMenu menu(this);
    
    QAction* deleteAction = menu.addAction("Delete Page");
    connect(deleteAction, &QAction::triggered, this, [this, pageId]() {
        emit pageDeleteRequested(pageId);
    });
    
    menu.exec(m_listView->mapToGlobal(pos));
}

} // namespace creator
} // namespace smartbook
