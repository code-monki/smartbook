#ifndef SMARTBOOK_CREATOR_UI_FORMBUILDERDIALOG_H
#define SMARTBOOK_CREATOR_UI_FORMBUILDERDIALOG_H

#include <QDialog>

namespace smartbook {
namespace creator {
namespace ui {

/**
 * @brief Form builder dialog
 */
class FormBuilderDialog : public QDialog {
    Q_OBJECT

public:
    explicit FormBuilderDialog(QWidget* parent = nullptr);
    ~FormBuilderDialog();

private:
    void setupUI();
};

} // namespace ui
} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_UI_FORMBUILDERDIALOG_H
