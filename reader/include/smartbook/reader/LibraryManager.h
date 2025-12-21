#ifndef SMARTBOOK_READER_LIBRARYMANAGER_H
#define SMARTBOOK_READER_LIBRARYMANAGER_H

#include <QMainWindow>
#include <QWidget>
#include <QList>
#include <QString>
#include <QByteArray>
#include <memory>

namespace smartbook {
namespace reader {

class LibraryView;
class ReaderViewWindow;

/**
 * @brief Cartridge information for library display
 */
struct CartridgeInfo {
    QString cartridgeGuid;
    QString title;
    QString author;
    QString publicationYear;
    QString publisher;
    QString version;
    QString localPath;
    QByteArray coverImageData;
    
    bool isValid() const { return !cartridgeGuid.isEmpty() && !title.isEmpty(); }
};

/**
 * @brief Main Library Manager window
 * 
 * The Hub - handles application launch, library browsing, import/delete,
 * and Trust Revocation. Relies exclusively on the Manifest for fast loading.
 */
class LibraryManager : public QMainWindow {
    Q_OBJECT

public:
    explicit LibraryManager(QWidget* parent = nullptr);
    ~LibraryManager();

    /**
     * @brief Open a cartridge in a new Reader View Window
     * @param cartridgeGuid GUID of the cartridge to open
     */
    void openCartridge(const QString& cartridgeGuid);

    /**
     * @brief Load library from manifest
     * @return List of cartridge metadata for display
     */
    QList<CartridgeInfo> loadLibraryData();

private slots:
    void onImportCartridge();
    void onDeleteCartridge(const QString& cartridgeGuid);
    void onCartridgeDoubleClicked(const QString& cartridgeGuid);

private:
    void setupUI();
    void setupMenuBar();
    void loadLibrary();

    LibraryView* m_libraryView;
    QList<ReaderViewWindow*> m_readerWindows;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_LIBRARYMANAGER_H
