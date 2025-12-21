#ifndef SMARTBOOK_READER_UI_LIBRARYVIEW_H
#define SMARTBOOK_READER_UI_LIBRARYVIEW_H

#include <QWidget>
#include <QListView>
#include <QTableView>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QAbstractItemView>

namespace smartbook {
namespace reader {

/**
 * @brief Library view widget - displays cartridges in library
 * 
 * Supports both List View (table) and Bookshelf View (grid) modes.
 * Relies on Local_Library_Manifest for fast loading.
 * 
 * DDD Section 11.1: UI Dual View
 */
class LibraryView : public QWidget {
    Q_OBJECT

public:
    explicit LibraryView(QWidget* parent = nullptr);
    ~LibraryView();

    /**
     * @brief Refresh the library view from manifest
     */
    void refreshLibrary();

    /**
     * @brief Toggle between List View and Bookshelf View
     * 
     * AC: Both views load instantly (< 100ms)
     */
    void toggleView();

    /**
     * @brief Get current view mode
     * @return true if List View, false if Bookshelf View
     */
    bool isListView() const { return m_isListView; }

signals:
    void cartridgeDoubleClicked(const QString& cartridgeGuid);
    void cartridgeDeleteRequested(const QString& cartridgeGuid);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onTableDoubleClicked(const QModelIndex& index);

private:
    void setupUI();
    void loadCartridges();
    void setupListView();
    void setupBookshelfView();
    void updateView();

    QStackedWidget* m_stackedWidget;
    QTableView* m_tableView;      // List View: Table with columns
    QListView* m_gridView;        // Bookshelf View: Grid layout
    QStandardItemModel* m_listModel;
    QStandardItemModel* m_gridModel;
    bool m_isListView = true;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_UI_LIBRARYVIEW_H
