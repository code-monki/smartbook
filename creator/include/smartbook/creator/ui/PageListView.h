#ifndef SMARTBOOK_CREATOR_UI_PAGELISTVIEW_H
#define SMARTBOOK_CREATOR_UI_PAGELISTVIEW_H

#include <QWidget>
#include <QListView>
#include <QStandardItemModel>
#include <QString>

namespace smartbook {
namespace creator {

class PageManager;

/**
 * @brief Page list view widget for page selection
 * 
 * Displays list of pages with chapter titles and allows selection.
 * Implements FR-CT-3.6 (Page Selection)
 */
class PageListView : public QWidget {
    Q_OBJECT

public:
    explicit PageListView(QWidget* parent = nullptr);
    
    /**
     * @brief Set page manager
     * @param pageManager PageManager instance
     */
    void setPageManager(PageManager* pageManager);
    
    /**
     * @brief Refresh page list from PageManager
     */
    void refreshPageList();
    
    /**
     * @brief Get selected page ID
     * @return Selected page ID, or -1 if none selected
     */
    int getSelectedPageId() const;

signals:
    void pageSelected(int pageId);
    void pageCreateRequested();
    void pageDeleteRequested(int pageId);
    void pageReorderRequested(int pageId, int newOrder);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onContextMenuRequested(const QPoint& pos);

private:
    void setupUI();
    QString formatPageDisplay(int pageId, int pageOrder, const QString& chapterTitle) const;
    
    QListView* m_listView;
    QStandardItemModel* m_model;
    PageManager* m_pageManager;
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_UI_PAGELISTVIEW_H
