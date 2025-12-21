#ifndef SMARTBOOK_CREATOR_PAGEMANAGER_H
#define SMARTBOOK_CREATOR_PAGEMANAGER_H

#include <QObject>
#include <QString>
#include <QList>

namespace smartbook {
namespace common {
namespace database {
    class CartridgeDBConnector;
}
}

namespace creator {

/**
 * @brief Page information structure
 */
struct PageInfo {
    int pageId;
    int pageOrder;
    QString chapterTitle;
    QString htmlContent;
    QString associatedCss;
    
    bool isValid() const { return pageId > 0; }
};

/**
 * @brief Page manager for Content_Pages table operations
 * 
 * Handles page CRUD operations, ordering, and chapter organization.
 * Implements FR-CT-3.6 through FR-CT-3.9
 */
class PageManager : public QObject {
    Q_OBJECT

public:
    explicit PageManager(QObject* parent = nullptr);
    
    /**
     * @brief Open cartridge for page management
     * @param cartridgePath Path to cartridge SQLite file
     * @return true if opened successfully
     */
    bool openCartridge(const QString& cartridgePath);
    
    /**
     * @brief Close cartridge
     */
    void closeCartridge();
    
    /**
     * @brief Get all pages
     * @return List of page information, ordered by page_order
     */
    QList<PageInfo> getPages() const;
    
    /**
     * @brief Get page by ID
     * @param pageId Page ID
     * @return PageInfo if found, invalid PageInfo otherwise
     */
    PageInfo getPage(int pageId) const;
    
    /**
     * @brief Get current page
     * @return Current page ID, or -1 if none selected
     */
    int getCurrentPageId() const { return m_currentPageId; }
    
    /**
     * @brief Set current page
     * @param pageId Page ID to select
     */
    void setCurrentPage(int pageId);
    
    /**
     * @brief Create a new page
     * @param chapterTitle Optional chapter title
     * @return New page ID, or -1 on error
     */
    int createPage(const QString& chapterTitle = QString());
    
    /**
     * @brief Update page content
     * @param pageId Page ID
     * @param htmlContent HTML content
     * @param css Optional CSS stylesheet
     * @return true if updated successfully
     */
    bool updatePageContent(int pageId, const QString& htmlContent, const QString& css = QString());
    
    /**
     * @brief Update page metadata
     * @param pageId Page ID
     * @param chapterTitle Chapter title
     * @return true if updated successfully
     */
    bool updatePageMetadata(int pageId, const QString& chapterTitle);
    
    /**
     * @brief Delete a page
     * @param pageId Page ID to delete
     * @return true if deleted successfully
     */
    bool deletePage(int pageId);
    
    /**
     * @brief Reorder pages
     * @param pageIds List of page IDs in new order
     * @return true if reordered successfully
     */
    bool reorderPages(const QList<int>& pageIds);
    
    /**
     * @brief Move page to new position
     * @param pageId Page ID to move
     * @param newOrder New page_order value
     * @return true if moved successfully
     */
    bool movePage(int pageId, int newOrder);

signals:
    void pageListChanged();
    void currentPageChanged(int pageId);
    void pageContentChanged(int pageId);

private:
    void refreshPageList();
    int getNextPageOrder() const;
    void updatePageOrders();
    
    QString m_cartridgePath;
    int m_currentPageId;
    QList<PageInfo> m_pages;
    common::database::CartridgeDBConnector* m_dbConnector;
    
    // Forward declaration resolved in implementation
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_PAGEMANAGER_H
