#include "smartbook/reader/ui/ReaderView.h"
#include <QWebEngineView>
#include <QVBoxLayout>
#include <QUrl>
#include <QDebug>

namespace smartbook {
namespace reader {

ReaderView::ReaderView(QWidget* parent)
    : QWidget(parent)
    , m_webView(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_webView = new QWebEngineView(this);
    layout->addWidget(m_webView);

    // TODO: Configure WebEngine profile (network blocking, CSP, etc.)
    // TODO: Setup WebChannel bridge
}

ReaderView::~ReaderView() {
}

void ReaderView::loadCartridge(const QString& cartridgePath) {
    qDebug() << "Loading cartridge:" << cartridgePath;
    // TODO: Load cartridge content into web view
    // 1. Extract HTML content from cartridge
    // 2. Load into QWebEngineView
    // 3. Setup WebChannel bridge
    emit contentLoaded();
}

} // namespace reader
} // namespace smartbook
