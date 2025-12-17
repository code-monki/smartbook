#ifndef SMARTBOOK_READER_UI_CONSENTDIALOG_H
#define SMARTBOOK_READER_UI_CONSENTDIALOG_H

#include <QDialog>
#include <QString>
#include "smartbook/common/security/SignatureVerifier.h"

namespace smartbook {
namespace reader {
namespace ui {

/**
 * @brief Consent dialog for Level 2 and Level 3 cartridges
 * 
 * Displays security warning and allows user to grant or deny consent
 * for embedded application execution.
 */
class ConsentDialog : public QDialog {
    Q_OBJECT

public:
    enum ConsentResult {
        LoadAndAlwaysTrust,
        LoadForSessionOnly,
        Cancel
    };

    explicit ConsentDialog(smartbook::common::security::SecurityLevel level, const QString& cartridgeTitle, QWidget* parent = nullptr);
    ~ConsentDialog();

    /**
     * @brief Get the user's consent decision
     * @return ConsentResult indicating user's choice
     */
    ConsentResult getResult() const { return m_result; }

private slots:
    void onLoadAndAlwaysTrust();
    void onLoadForSessionOnly();
    void onCancel();

private:
    void setupUI(smartbook::common::security::SecurityLevel level, const QString& cartridgeTitle);

    ConsentResult m_result = Cancel;
};

} // namespace ui
} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_UI_CONSENTDIALOG_H
