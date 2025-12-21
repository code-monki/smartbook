#include "smartbook/creator/PageManager.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

namespace smartbook {
namespace creator {

PageManager::PageManager(QObject* parent)
    : QObject(parent)
    , m_currentPageId(-1)
    , m_dbConnector(nullptr)
{
}

bool PageManager::openCartridge(const QString& cartridgePath)
{
    if (m_dbConnector) {
        closeCartridge();
    }
    
    m_cartridgePath = cartridgePath;
    m_dbConnector = new common::database::CartridgeDBConnector(this);
    
    if (!m_dbConnector->openCartridge(cartridgePath)) {
        qWarning() << "Failed to open cartridge for page management:" << cartridgePath;
        delete m_dbConnector;
        m_dbConnector = nullptr;
        return false;
    }
    
    refreshPageList();
    return true;
}

void PageManager::closeCartridge()
{
    if (m_dbConnector) {
        m_dbConnector->closeCartridge();
        delete m_dbConnector;
        m_dbConnector = nullptr;
    }
    m_pages.clear();
    m_currentPageId = -1;
    m_cartridgePath.clear();
}

QList<PageInfo> PageManager::getPages() const
{
    return m_pages;
}

PageInfo PageManager::getPage(int pageId) const
{
    for (const PageInfo& page : m_pages) {
        if (page.pageId == pageId) {
            return page;
        }
    }
    return PageInfo(); // Invalid page
}

void PageManager::setCurrentPage(int pageId)
{
    if (m_currentPageId != pageId) {
        m_currentPageId = pageId;
        emit currentPageChanged(pageId);
    }
}

int PageManager::createPage(const QString& chapterTitle)
{
    if (!m_dbConnector) {
        qWarning() << "No cartridge open for page creation";
        return -1;
    }
    
    int nextOrder = getNextPageOrder();
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("INSERT INTO Content_Pages (page_order, chapter_title, html_content, associated_css) "
                  "VALUES (?, ?, '<p></p>', '')");
    query.addBindValue(nextOrder);
    query.addBindValue(chapterTitle.isEmpty() ? QVariant() : chapterTitle);
    
    if (!query.exec()) {
        qWarning() << "Failed to create page:" << query.lastError().text();
        return -1;
    }
    
    // Get the new page ID
    int newPageId = query.lastInsertId().toInt();
    
    refreshPageList();
    emit pageListChanged();
    
    return newPageId;
}

bool PageManager::updatePageContent(int pageId, const QString& htmlContent, const QString& css)
{
    if (!m_dbConnector) {
        qWarning() << "No cartridge open for page update";
        return false;
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("UPDATE Content_Pages SET html_content = ?, associated_css = ? WHERE page_id = ?");
    query.addBindValue(htmlContent);
    query.addBindValue(css.isEmpty() ? QVariant() : css);
    query.addBindValue(pageId);
    
    if (!query.exec()) {
        qWarning() << "Failed to update page content:" << query.lastError().text();
        return false;
    }
    
    // Update local cache
    for (PageInfo& page : m_pages) {
        if (page.pageId == pageId) {
            page.htmlContent = htmlContent;
            page.associatedCss = css;
            break;
        }
    }
    
    emit pageContentChanged(pageId);
    
    return true;
}

bool PageManager::updatePageMetadata(int pageId, const QString& chapterTitle)
{
    if (!m_dbConnector) {
        qWarning() << "No cartridge open for page metadata update";
        return false;
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("UPDATE Content_Pages SET chapter_title = ? WHERE page_id = ?");
    query.addBindValue(chapterTitle.isEmpty() ? QVariant() : chapterTitle);
    query.addBindValue(pageId);
    
    if (!query.exec()) {
        qWarning() << "Failed to update page metadata:" << query.lastError().text();
        return false;
    }
    
    // Update local cache
    for (PageInfo& page : m_pages) {
        if (page.pageId == pageId) {
            page.chapterTitle = chapterTitle;
            break;
        }
    }
    
    emit pageListChanged(); // Chapter title affects list display
    emit pageContentChanged(pageId);
    
    return true;
}

bool PageManager::deletePage(int pageId)
{
    if (!m_dbConnector) {
        qWarning() << "No cartridge open for page deletion";
        return false;
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("DELETE FROM Content_Pages WHERE page_id = ?");
    query.addBindValue(pageId);
    
    if (!query.exec()) {
        qWarning() << "Failed to delete page:" << query.lastError().text();
        return false;
    }
    
    if (m_currentPageId == pageId) {
        m_currentPageId = -1;
        emit currentPageChanged(-1);
    }
    
    refreshPageList();
    updatePageOrders(); // Reorder remaining pages
    emit pageListChanged();
    
    return true;
}

bool PageManager::reorderPages(const QList<int>& pageIds)
{
    if (!m_dbConnector) {
        qWarning() << "No cartridge open for page reordering";
        return false;
    }
    
    // Use a transaction to ensure atomicity
    if (!m_dbConnector->beginTransaction()) {
        qWarning() << "Failed to begin transaction for page reordering";
        return false;
    }
    
    // First, set all page_order values to temporary negative values to avoid UNIQUE constraint conflicts
    int maxOrder = m_pages.size() + 1000; // Large offset to avoid conflicts
    for (int i = 0; i < pageIds.size(); ++i) {
        QSqlQuery query(m_dbConnector->getDatabase());
        query.prepare("UPDATE Content_Pages SET page_order = ? WHERE page_id = ?");
        query.addBindValue(-(maxOrder + i)); // Temporary negative value
        query.addBindValue(pageIds[i]);
        
        if (!query.exec()) {
            qWarning() << "Failed to set temporary page_order:" << query.lastError().text();
            m_dbConnector->rollbackTransaction();
            return false;
        }
    }
    
    // Now set the final page_order values
    for (int i = 0; i < pageIds.size(); ++i) {
        QSqlQuery query(m_dbConnector->getDatabase());
        query.prepare("UPDATE Content_Pages SET page_order = ? WHERE page_id = ?");
        query.addBindValue(i + 1); // page_order starts at 1
        query.addBindValue(pageIds[i]);
        
        if (!query.exec()) {
            qWarning() << "Failed to set final page_order:" << query.lastError().text();
            m_dbConnector->rollbackTransaction();
            return false;
        }
    }
    
    if (!m_dbConnector->commitTransaction()) {
        qWarning() << "Failed to commit transaction for page reordering";
        m_dbConnector->rollbackTransaction();
        return false;
    }
    
    refreshPageList();
    emit pageListChanged();
    
    return true;
}

bool PageManager::movePage(int pageId, int newOrder)
{
    if (!m_dbConnector) {
        qWarning() << "No cartridge open for page move";
        return false;
    }
    
    // Get current page order
    PageInfo page = getPage(pageId);
    if (!page.isValid()) {
        return false;
    }
    
    // Collect all page IDs
    QList<int> pageIds;
    for (const PageInfo& p : m_pages) {
        if (p.pageId != pageId) {
            pageIds.append(p.pageId);
        }
    }
    
    // Insert at new position
    if (newOrder <= 0) {
        pageIds.prepend(pageId);
    } else if (newOrder >= pageIds.size()) {
        pageIds.append(pageId);
    } else {
        pageIds.insert(newOrder - 1, pageId);
    }
    
    return reorderPages(pageIds);
}

void PageManager::refreshPageList()
{
    m_pages.clear();
    
    if (!m_dbConnector) {
        return;
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("SELECT page_id, page_order, chapter_title, html_content, associated_css "
                  "FROM Content_Pages "
                  "ORDER BY page_order");
    
    if (!query.exec()) {
        qWarning() << "Failed to refresh page list:" << query.lastError().text();
        return;
    }
    
    while (query.next()) {
        PageInfo page;
        page.pageId = query.value(0).toInt();
        page.pageOrder = query.value(1).toInt();
        page.chapterTitle = query.value(2).toString();
        page.htmlContent = query.value(3).toString();
        page.associatedCss = query.value(4).toString();
        
        if (page.isValid()) {
            m_pages.append(page);
        }
    }
}

int PageManager::getNextPageOrder() const
{
    if (m_pages.isEmpty()) {
        return 1;
    }
    
    int maxOrder = 0;
    for (const PageInfo& page : m_pages) {
        if (page.pageOrder > maxOrder) {
            maxOrder = page.pageOrder;
        }
    }
    
    return maxOrder + 1;
}

void PageManager::updatePageOrders()
{
    // Reorder all pages sequentially starting from 1
    for (int i = 0; i < m_pages.size(); ++i) {
        int pageId = m_pages[i].pageId;
        QSqlQuery query(m_dbConnector->getDatabase());
        query.prepare("UPDATE Content_Pages SET page_order = ? WHERE page_id = ?");
        query.addBindValue(i + 1);
        query.addBindValue(pageId);
        query.exec();
    }
    
    refreshPageList();
}

} // namespace creator
} // namespace smartbook
