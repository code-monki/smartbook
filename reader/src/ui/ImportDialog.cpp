#include "smartbook/reader/ui/ImportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFileInfo>

namespace smartbook {
namespace reader {
namespace ui {

class ImportDialog::Private {
public:
    QProgressBar* overallProgressBar;
    QListWidget* fileListWidget;
    QLabel* statusLabel;
    QPushButton* cancelButton;
    QHash<QString, QListWidgetItem*> fileItems;
};

ImportDialog::ImportDialog(QWidget* parent)
    : QDialog(parent)
    , d(new Private)
{
    setupUI();
}

ImportDialog::~ImportDialog() {
    delete d;
}

void ImportDialog::setupUI() {
    setWindowTitle("Importing Cartridges");
    setMinimumWidth(600);
    resize(600, 400);
    setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Status label
    d->statusLabel = new QLabel("Preparing import...", this);
    layout->addWidget(d->statusLabel);

    // Overall progress bar
    d->overallProgressBar = new QProgressBar(this);
    d->overallProgressBar->setRange(0, 100);
    d->overallProgressBar->setValue(0);
    layout->addWidget(d->overallProgressBar);

    // File list
    QLabel* fileListLabel = new QLabel("Files:", this);
    layout->addWidget(fileListLabel);

    d->fileListWidget = new QListWidget(this);
    d->fileListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(d->fileListWidget);

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    d->cancelButton = buttonBox->addButton("Cancel", QDialogButtonBox::RejectRole);
    connect(d->cancelButton, &QPushButton::clicked, this, &ImportDialog::onCancel);
    layout->addWidget(buttonBox);
}

void ImportDialog::setFiles(const QStringList& filePaths) {
    m_filePaths = filePaths;
    d->fileItems.clear();
    d->fileListWidget->clear();

    for (const QString& filePath : filePaths) {
        QFileInfo fileInfo(filePath);
        QString displayName = fileInfo.fileName();
        
        QListWidgetItem* item = new QListWidgetItem(displayName, d->fileListWidget);
        item->setData(Qt::UserRole, filePath);
        item->setForeground(Qt::gray);
        d->fileItems[filePath] = item;
    }

    updateOverallProgress();
}

void ImportDialog::updateFileProgress(const QString& filePath, const QString& status, int progress) {
    m_fileStatuses[filePath] = status;
    if (progress >= 0) {
        m_fileProgress[filePath] = progress;
    }

    QListWidgetItem* item = d->fileItems.value(filePath);
    if (item) {
        QFileInfo fileInfo(filePath);
        QString displayText = QString("%1 - %2").arg(fileInfo.fileName(), status);
        item->setText(displayText);
        
        if (status.contains("Completed") || status.contains("Skipped")) {
            item->setForeground(Qt::darkGreen);
        } else if (status.contains("Failed") || status.contains("Error")) {
            item->setForeground(Qt::red);
        } else {
            item->setForeground(Qt::black);
        }
    }

    updateOverallProgress();
}

ImportDialog::DuplicateAction ImportDialog::showDuplicateDialog(
    const QString& cartridgeTitle,
    const QString& cartridgeGuid)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Duplicate Cartridge Detected");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(QString("A cartridge with the same GUID already exists in your library.\n\n"
                           "Title: %1\n"
                           "GUID: %2\n\n"
                           "How would you like to proceed?")
                    .arg(cartridgeTitle, cartridgeGuid));
    
    QPushButton* replaceButton = msgBox.addButton("Replace", QMessageBox::DestructiveRole);
    msgBox.addButton("Skip", QMessageBox::RejectRole);
    QPushButton* keepBothButton = msgBox.addButton("Keep Both", QMessageBox::AcceptRole);
    
    msgBox.exec();

    if (msgBox.clickedButton() == replaceButton) {
        return Replace;
    } else if (msgBox.clickedButton() == keepBothButton) {
        return KeepBoth;
    } else {
        return Skip;
    }
}

void ImportDialog::showCompletionSummary(int successCount, int errorCount, int skippedCount) {
    QString summary = QString("Import completed:\n\n"
                             "Successfully imported: %1\n"
                             "Failed: %2\n"
                             "Skipped: %3")
                     .arg(successCount).arg(errorCount).arg(skippedCount);
    
    if (errorCount > 0 || skippedCount > 0) {
        QMessageBox::warning(this, "Import Summary", summary);
    } else {
        QMessageBox::information(this, "Import Complete", summary);
    }
}

void ImportDialog::updateOverallProgress() {
    if (m_filePaths.isEmpty()) {
        d->overallProgressBar->setValue(0);
        return;
    }

    int totalProgress = 0;
    int fileCount = m_filePaths.size();
    
    for (const QString& filePath : m_filePaths) {
        totalProgress += m_fileProgress.value(filePath, 0);
    }

    int overallProgress = fileCount > 0 ? (totalProgress / fileCount) : 0;
    d->overallProgressBar->setValue(overallProgress);

    int completedCount = 0;
    for (const QString& status : m_fileStatuses.values()) {
        if (status.contains("Completed") || status.contains("Skipped") || status.contains("Failed")) {
            completedCount++;
        }
    }

    d->statusLabel->setText(QString("Processing: %1 of %2 files")
                           .arg(completedCount).arg(fileCount));
}

void ImportDialog::onCancel() {
    m_cancelled = true;
    reject();
}

} // namespace ui
} // namespace reader
} // namespace smartbook

