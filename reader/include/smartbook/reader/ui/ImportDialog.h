#ifndef SMARTBOOK_READER_UI_IMPORTDIALOG_H
#define SMARTBOOK_READER_UI_IMPORTDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

namespace smartbook {
namespace reader {
namespace ui {

/**
 * @brief Import progress dialog for cartridge import operations
 * 
 * Displays progress for importing cartridges, including file-by-file
 * status and overall progress.
 */
class ImportDialog : public QDialog {
    Q_OBJECT

public:
    enum DuplicateAction {
        Skip,
        Replace,
        KeepBoth
    };

    explicit ImportDialog(QWidget* parent = nullptr);
    ~ImportDialog();

    /**
     * @brief Set the list of files to import
     * @param filePaths List of file paths to import
     */
    void setFiles(const QStringList& filePaths);

    /**
     * @brief Update progress for a specific file
     * @param filePath File path
     * @param status Status message
     * @param progress Progress percentage (0-100)
     */
    void updateFileProgress(const QString& filePath, const QString& status, int progress = -1);

    /**
     * @brief Show duplicate detection dialog
     * @param cartridgeTitle Title of the duplicate cartridge
     * @param cartridgeGuid GUID of the duplicate cartridge
     * @return User's choice (Skip, Replace, or KeepBoth)
     */
    DuplicateAction showDuplicateDialog(const QString& cartridgeTitle, const QString& cartridgeGuid);

    /**
     * @brief Show completion summary
     * @param successCount Number of successfully imported files
     * @param errorCount Number of files that failed
     * @param skippedCount Number of files that were skipped
     */
    void showCompletionSummary(int successCount, int errorCount, int skippedCount);

    /**
     * @brief Check if user cancelled the import
     * @return true if cancelled, false otherwise
     */
    bool wasCancelled() const { return m_cancelled; }

public slots:
    void onCancel();

private:
    void setupUI();
    void updateOverallProgress();

    QStringList m_filePaths;
    QHash<QString, QString> m_fileStatuses;
    QHash<QString, int> m_fileProgress;
    bool m_cancelled = false;
    
    // UI elements
    class Private;
    Private* d;
};

} // namespace ui
} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_UI_IMPORTDIALOG_H

