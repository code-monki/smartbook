#include "smartbook/creator/FormBuilder.h"
#include "smartbook/creator/FormManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QToolBar>
#include <QAction>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QMessageBox>
#include <QInputDialog>
#include <QAbstractItemView>
#include <QFont>
#include <QDebug>

namespace smartbook {
namespace creator {

FormBuilder::FormBuilder(QWidget* parent)
    : QWidget(parent)
    , m_fieldList(nullptr)
    , m_formIdEdit(nullptr)
    , m_formTitleEdit(nullptr)
    , m_jsonPreview(nullptr)
    , m_validationStatus(nullptr)
    , m_toolbar(nullptr)
{
    setupUI();
}

FormBuilder::~FormBuilder() {
}

void FormBuilder::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Form metadata section
    QHBoxLayout* metadataLayout = new QHBoxLayout();
    metadataLayout->addWidget(new QLabel("Form ID:", this));
    m_formIdEdit = new QLineEdit(this);
    m_formIdEdit->setPlaceholderText("e.g., contact_form_v1");
    metadataLayout->addWidget(m_formIdEdit);
    
    metadataLayout->addWidget(new QLabel("Form Title:", this));
    m_formTitleEdit = new QLineEdit(this);
    m_formTitleEdit->setPlaceholderText("e.g., Contact Information Form");
    metadataLayout->addWidget(m_formTitleEdit);
    
    mainLayout->addLayout(metadataLayout);
    
    // Toolbar for field actions
    setupToolbar();
    if (m_toolbar) {
        mainLayout->addWidget(m_toolbar);
    }
    
    // Field list and JSON preview splitter
    QHBoxLayout* contentLayout = new QHBoxLayout();
    
    // Field list
    QVBoxLayout* fieldListLayout = new QVBoxLayout();
    fieldListLayout->addWidget(new QLabel("Fields:", this));
    m_fieldList = new QListWidget(this);
    m_fieldList->setSelectionMode(QAbstractItemView::SingleSelection);
    fieldListLayout->addWidget(m_fieldList);
    
    contentLayout->addLayout(fieldListLayout, 1);
    
    // JSON preview
    QVBoxLayout* previewLayout = new QVBoxLayout();
    previewLayout->addWidget(new QLabel("JSON Schema Preview:", this));
    m_jsonPreview = new QTextEdit(this);
    m_jsonPreview->setReadOnly(true);
    m_jsonPreview->setFont(QFont("Courier", 10));
    previewLayout->addWidget(m_jsonPreview);
    
    m_validationStatus = new QLabel("", this);
    m_validationStatus->setStyleSheet("color: green;");
    previewLayout->addWidget(m_validationStatus);
    
    contentLayout->addLayout(previewLayout, 1);
    
    mainLayout->addLayout(contentLayout, 1);
    
    // Connect signals
    connect(m_formIdEdit, &QLineEdit::textChanged, this, &FormBuilder::onFieldChanged);
    connect(m_formTitleEdit, &QLineEdit::textChanged, this, &FormBuilder::onFieldChanged);
    connect(m_fieldList, &QListWidget::itemSelectionChanged, this, &FormBuilder::onFieldChanged);
}

void FormBuilder::setupToolbar() {
    m_toolbar = new QToolBar(this);
    
    QAction* addTextAction = m_toolbar->addAction("Add Text Field");
    connect(addTextAction, &QAction::triggered, this, &FormBuilder::addTextField);
    
    QAction* addNumberAction = m_toolbar->addAction("Add Number Field");
    connect(addNumberAction, &QAction::triggered, this, &FormBuilder::addNumberField);
    
    QAction* addTextareaAction = m_toolbar->addAction("Add Textarea");
    connect(addTextareaAction, &QAction::triggered, this, &FormBuilder::addTextareaField);
    
    QAction* addSelectAction = m_toolbar->addAction("Add Select");
    connect(addSelectAction, &QAction::triggered, this, &FormBuilder::addSelectField);
    
    QAction* addGroupAction = m_toolbar->addAction("Add Group");
    connect(addGroupAction, &QAction::triggered, this, &FormBuilder::addGroupField);
    
    m_toolbar->addSeparator();
    
    QAction* removeAction = m_toolbar->addAction("Remove Field");
    connect(removeAction, &QAction::triggered, this, [this]() {
        int row = m_fieldList->currentRow();
        if (row >= 0) {
            removeField(row);
        }
    });
    
    QAction* moveUpAction = m_toolbar->addAction("Move Up");
    connect(moveUpAction, &QAction::triggered, this, [this]() {
        int row = m_fieldList->currentRow();
        if (row > 0) {
            moveFieldUp(row);
        }
    });
    
    QAction* moveDownAction = m_toolbar->addAction("Move Down");
    connect(moveDownAction, &QAction::triggered, this, [this]() {
        int row = m_fieldList->currentRow();
        if (row >= 0 && row < m_fieldList->count() - 1) {
            moveFieldDown(row);
        }
    });
}

bool FormBuilder::loadFormDefinition(const QString& formId, const QString& schemaJson) {
    m_formId = formId;
    m_formIdEdit->setText(formId);
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(schemaJson.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse form schema JSON:" << error.errorString();
        return false;
    }
    
    QJsonObject root = doc.object();
    
    m_formTitle = root.value("formTitle").toString();
    m_formTitleEdit->setText(m_formTitle);
    
    m_fields = root.value("fields").toArray();
    
    refreshFieldList();
    updateFormJson();
    
    return true;
}

QString FormBuilder::getFormDefinitionJson() const {
    QJsonObject schema = buildFormSchema();
    QJsonDocument doc(schema);
    return doc.toJson(QJsonDocument::Indented);
}

void FormBuilder::setFormId(const QString& formId) {
    m_formId = formId;
    m_formIdEdit->setText(formId);
    onFieldChanged();
}

bool FormBuilder::validateFormSchema() const {
    QStringList errors;
    
    // Validate form ID
    if (m_formId.isEmpty()) {
        errors << "Form ID is required";
    }
    
    // Validate form title
    if (m_formTitle.isEmpty()) {
        errors << "Form Title is required";
    }
    
    // Validate fields
    if (m_fields.isEmpty()) {
        errors << "At least one field is required";
    }
    
    // Validate each field
    for (int i = 0; i < m_fields.size(); ++i) {
        QJsonObject field = m_fields[i].toObject();
        validateField(field, errors);
    }
    
    return errors.isEmpty();
}

QStringList FormBuilder::getValidationErrors() const {
    QStringList errors;
    
    if (m_formId.isEmpty()) {
        errors << "Form ID is required";
    }
    
    if (m_formTitle.isEmpty()) {
        errors << "Form Title is required";
    }
    
    if (m_fields.isEmpty()) {
        errors << "At least one field is required";
    }
    
    for (int i = 0; i < m_fields.size(); ++i) {
        QJsonObject field = m_fields[i].toObject();
        validateField(field, errors);
    }
    
    return errors;
}

void FormBuilder::addTextField() {
    QString fieldId = generateFieldId("text");
    QJsonObject field = createFieldObject(fieldId, "text", "Text Field");
    m_fields.append(field);
    refreshFieldList();
    updateFormJson();
    emit formChanged();
}

void FormBuilder::addNumberField() {
    QString fieldId = generateFieldId("number");
    QJsonObject field = createFieldObject(fieldId, "number", "Number Field");
    m_fields.append(field);
    refreshFieldList();
    updateFormJson();
    emit formChanged();
}

void FormBuilder::addTextareaField() {
    QString fieldId = generateFieldId("textarea");
    QJsonObject field = createFieldObject(fieldId, "textarea", "Textarea Field");
    m_fields.append(field);
    refreshFieldList();
    updateFormJson();
    emit formChanged();
}

void FormBuilder::addSelectField() {
    QString fieldId = generateFieldId("select");
    QJsonObject field = createFieldObject(fieldId, "select", "Select Field");
    
    // Add default options for select
    QJsonArray options;
    options.append(QJsonObject{{"value", "option1"}, {"label", "Option 1"}});
    options.append(QJsonObject{{"value", "option2"}, {"label", "Option 2"}});
    field.insert("options", options);
    
    m_fields.append(field);
    refreshFieldList();
    updateFormJson();
    emit formChanged();
}

void FormBuilder::addGroupField() {
    QString fieldId = generateFieldId("group");
    QJsonObject field = createFieldObject(fieldId, "group", "Group");
    
    // Add empty children array for group
    QJsonArray children;
    field.insert("children", children);
    
    m_fields.append(field);
    refreshFieldList();
    updateFormJson();
    emit formChanged();
}

void FormBuilder::removeField(int index) {
    if (index >= 0 && index < m_fields.size()) {
        m_fields.removeAt(index);
        refreshFieldList();
        updateFormJson();
        emit formChanged();
    }
}

void FormBuilder::moveFieldUp(int index) {
    if (index > 0 && index < m_fields.size()) {
        QJsonValue field = m_fields.takeAt(index);
        m_fields.insert(index - 1, field);
        refreshFieldList();
        m_fieldList->setCurrentRow(index - 1);
        updateFormJson();
        emit formChanged();
    }
}

void FormBuilder::moveFieldDown(int index) {
    if (index >= 0 && index < m_fields.size() - 1) {
        QJsonValue field = m_fields.takeAt(index);
        m_fields.insert(index + 1, field);
        refreshFieldList();
        m_fieldList->setCurrentRow(index + 1);
        updateFormJson();
        emit formChanged();
    }
}

void FormBuilder::onFieldChanged() {
    m_formId = m_formIdEdit->text();
    m_formTitle = m_formTitleEdit->text();
    updateFormJson();
    emit formChanged();
}

void FormBuilder::updateFormJson() {
    QJsonObject schema = buildFormSchema();
    QJsonDocument doc(schema);
    m_jsonPreview->setPlainText(doc.toJson(QJsonDocument::Indented));
    
    // Update validation status
    bool isValid = validateFormSchema();
    if (isValid) {
        m_validationStatus->setText("✓ Valid");
        m_validationStatus->setStyleSheet("color: green;");
    } else {
        QStringList errors = getValidationErrors();
        m_validationStatus->setText("✗ Invalid: " + errors.join("; "));
        m_validationStatus->setStyleSheet("color: red;");
    }
    
    emit validationStatusChanged(isValid);
}

void FormBuilder::refreshFieldList() {
    m_fieldList->clear();
    
    for (int i = 0; i < m_fields.size(); ++i) {
        QJsonObject field = m_fields[i].toObject();
        QString fieldId = field.value("fieldId").toString();
        QString fieldType = field.value("fieldType").toString();
        QString label = field.value("label").toString();
        
        QString displayText = QString("%1 (%2): %3").arg(fieldId, fieldType, label);
        m_fieldList->addItem(displayText);
    }
}

QJsonObject FormBuilder::createFieldObject(const QString& fieldId, const QString& fieldType, 
                                            const QString& label) const {
    QJsonObject field;
    field.insert("fieldId", fieldId);
    field.insert("fieldType", fieldType);
    field.insert("label", label);
    return field;
}

QJsonObject FormBuilder::buildFormSchema() const {
    QJsonObject schema;
    schema.insert("schemaVersion", "1.0");
    schema.insert("formId", m_formId);
    schema.insert("formTitle", m_formTitle);
    schema.insert("fields", m_fields);
    return schema;
}

bool FormBuilder::validateField(const QJsonObject& field, QStringList& errors) const {
    QString fieldId = field.value("fieldId").toString();
    
    if (fieldId.isEmpty()) {
        errors << "Field: fieldId is required";
        return false;
    }
    
    QString fieldType = field.value("fieldType").toString();
    if (fieldType.isEmpty()) {
        errors << QString("Field '%1': fieldType is required").arg(fieldId);
        return false;
    }
    
    QString label = field.value("label").toString();
    if (label.isEmpty()) {
        errors << QString("Field '%1': label is required").arg(fieldId);
        return false;
    }
    
    // Validate field type specific requirements
    if (fieldType == "select" || fieldType == "radio") {
        if (!field.contains("options")) {
            errors << QString("Field '%1': options array is required for select/radio type").arg(fieldId);
        } else {
            QJsonArray options = field.value("options").toArray();
            if (options.isEmpty()) {
                errors << QString("Field '%1': options array cannot be empty").arg(fieldId);
            }
        }
    }
    
    if (fieldType == "group") {
        if (!field.contains("children")) {
            errors << QString("Field '%1': children array is required for group type").arg(fieldId);
        }
    }
    
    return true;
}

QString FormBuilder::generateFieldId(const QString& fieldType) const {
    int count = 1;
    QString baseId = fieldType + "_field";
    QString fieldId = baseId;
    
    // Ensure unique field ID
    while (true) {
        bool exists = false;
        for (int i = 0; i < m_fields.size(); ++i) {
            QJsonObject field = m_fields[i].toObject();
            if (field.value("fieldId").toString() == fieldId) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            break;
        }
        
        fieldId = QString("%1_%2").arg(baseId).arg(count);
        count++;
    }
    
    return fieldId;
}

bool FormBuilder::saveToCartridge(FormManager* formManager) {
    if (!formManager) {
        qWarning() << "FormManager is null for save operation";
        return false;
    }
    
    if (!validateFormSchema()) {
        QStringList errors = getValidationErrors();
        qWarning() << "Cannot save invalid form:" << errors.join("; ");
        return false;
    }
    
    QString schemaJson = getFormDefinitionJson();
    return formManager->saveFormDefinition(m_formId, schemaJson);
}

} // namespace creator
} // namespace smartbook
