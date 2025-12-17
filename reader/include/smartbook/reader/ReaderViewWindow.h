#ifndef SMARTBOOK_READER_READERVIEWWINDOW_H
#define SMARTBOOK_READER_READERVIEWWINDOW_H

#include <QMainWindow>
#include <QString>
#include <memory>

namespace smartbook {
namespace reader {

class ReaderView;
class WebChannelBridge;

/**
 * @brief Reader View Window - container for a single opened cartridge
 * 
 * Owns the web view and data connector. Enforces security policy and
 * data isolation. Each cartridge gets its own Reader View Window.
 */
class ReaderViewWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ReaderViewWindow(const QString& cartridgeGuid, QWidget* parent = nullptr);
    ~ReaderViewWindow();

    /**
     * @brief Get the cartridge GUID
     * @return Cartridge GUID
     */
    QString getCartridgeGuid() const { return m_cartridgeGuid; }

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onContentLoaded();
    void onError(const QString& errorMessage);

private:
    void setupUI();
    void loadCartridge();
    void saveWindowState();

    QString m_cartridgeGuid;
    ReaderView* m_readerView;
    WebChannelBridge* m_webChannelBridge;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_READERVIEWWINDOW_H
