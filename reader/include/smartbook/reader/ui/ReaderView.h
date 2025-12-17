#ifndef SMARTBOOK_READER_UI_READERVIEW_H
#define SMARTBOOK_READER_UI_READERVIEW_H

#include <QWidget>
#include <QWebEngineView>

namespace smartbook {
namespace reader {

/**
 * @brief Reader view widget - displays cartridge content
 * 
 * Uses QWebEngineView to render HTML content with embedded applications.
 */
class ReaderView : public QWidget {
    Q_OBJECT

public:
    explicit ReaderView(QWidget* parent = nullptr);
    ~ReaderView();

    /**
     * @brief Load cartridge content
     * @param cartridgePath Path to cartridge file
     */
    void loadCartridge(const QString& cartridgePath);

signals:
    void contentLoaded();
    void errorOccurred(const QString& errorMessage);

private:
    QWebEngineView* m_webView;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_UI_READERVIEW_H
