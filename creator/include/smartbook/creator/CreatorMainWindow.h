#ifndef SMARTBOOK_CREATOR_CREATORMAINWINDOW_H
#define SMARTBOOK_CREATOR_CREATORMAINWINDOW_H

#include <QMainWindow>
#include <QWidget>

namespace smartbook {
namespace creator {

class ContentEditor;
class FormBuilder;

/**
 * @brief Creator Tool main window
 * 
 * Provides interface for creating and editing Smartbook cartridges.
 */
class CreatorMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit CreatorMainWindow(QWidget* parent = nullptr);
    ~CreatorMainWindow();

private slots:
    void onNewCartridge();
    void onOpenCartridge();
    void onSaveCartridge();
    void onExportCartridge();

private:
    void setupUI();
    void setupMenuBar();

    ContentEditor* m_contentEditor;
    FormBuilder* m_formBuilder;
    QString m_currentCartridgePath;
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_CREATORMAINWINDOW_H
