#ifndef SMARTBOOK_READER_UI_LIBRARYVIEW_H
#define SMARTBOOK_READER_UI_LIBRARYVIEW_H

#include <QWidget>
#include <QListView>
#include <QGridLayout>

namespace smartbook {
namespace reader {

/**
 * @brief Library view widget - displays cartridges in library
 * 
 * Supports both List View and Grid View (Bookshelf) modes.
 * Relies on Local_Library_Manifest for fast loading.
 */
class LibraryView : public QWidget {
    Q_OBJECT

public:
    explicit LibraryView(QWidget* parent = nullptr);
    ~LibraryView();

    /**
     * @brief Refresh the library view
     */
    void refreshLibrary();

    /**
     * @brief Toggle between List View and Grid View
     */
    void toggleView();

signals:
    void cartridgeDoubleClicked(const QString& cartridgeGuid);
    void cartridgeDeleteRequested(const QString& cartridgeGuid);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);

private:
    void setupUI();
    void loadCartridges();

    QListView* m_listView;
    bool m_isListView = true;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_UI_LIBRARYVIEW_H
