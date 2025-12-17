#include "smartbook/creator/ui/FormBuilderDialog.h"
#include <QVBoxLayout>
#include <QLabel>

namespace smartbook {
namespace creator {
namespace ui {

FormBuilderDialog::FormBuilderDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
}

FormBuilderDialog::~FormBuilderDialog() {
}

void FormBuilderDialog::setupUI() {
    setWindowTitle("Form Builder");
    resize(800, 600);

    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QLabel* label = new QLabel("Form Builder Dialog", this);
    layout->addWidget(label);

    // TODO: Implement form builder dialog UI
}

} // namespace ui
} // namespace creator
} // namespace smartbook
