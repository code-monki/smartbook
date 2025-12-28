/**
 * @file FormEmbeddedWidget.cpp
 * @brief Implementation of FormEmbeddedWidget for embedding forms in content pages
 * 
 * This file implements the FormEmbeddedWidget class, which hosts Qt Widgets forms
 * within cartridge content pages. It loads form schemas from the cartridge database,
 * generates form widgets dynamically, and handles data persistence and validation.
 * 
 * @section Form Generation Process
 * 
 * 1. Load form schema from Form_Definitions.form_schema_json column
 * 2. Parse JSON schema using FormSchemaParser
 * 3. Generate Qt Widgets form layout using FormWidgetGenerator
 * 4. Connect form field change signals for auto-save
 * 5. Load existing form data from User_Data table if available
 * 
 * @section Data Persistence
 * 
 * **Auto-Save:**
 * - Saves automatically 2 seconds after last change
 * - Reduces risk of data loss
 * - Validation occurs before auto-save
 * 
 * **Manual Save:**
 * - User can click "Save" button
 * - Button is enabled when form data changes
 * - Button is disabled after successful save
 * 
 * **Data Storage:**
 * - Form data is serialized as JSON using FormDataSerializer
 * - Stored in User_Data table with form_key = form_id
 * - Timestamp is recorded for each save
 * 
 * @section Validation
 * 
 * Form data is validated before saving:
 * - Required fields must be present and non-empty
 * - Field types must match schema types
 * - Numeric values must be within min/max range
 * - String values must match regex patterns (if specified)
 * 
 * Validation errors are displayed in status label with theme-aware colors.
 * 
 * @section Theme Support
 * 
 * Forms adapt to selected theme:
 * - Light: White background, black text
 * - Dark: Dark background, light text
 * - Sepia: Sepia background, dark brown text
 * 
 * Theme is applied to all form widgets (inputs, buttons, labels) via QPalette.
 * 
 * @see FormEmbeddedWidget.h
 * @see FormSchemaParser
 * @see FormWidgetGenerator
 * @see FormValidator
 * @see FormDataSerializer
 * @see qt-widgets-forms-guide.adoc
 */

#include "smartbook/reader/ui/FormEmbeddedWidget.h"
#include "smartbook/common/forms/FormWidgetGenerator.h"
#include "smartbook/common/forms/FormDataSerializer.h"
#include "smartbook/common/forms/FormValidator.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDebug>
#include <QPalette>
#include <QColor>
#include <QFormLayout>
#include "smartbook/common/utils/ThemeManager.h"

namespace smartbook {
namespace reader {

// ============================================================================
// Constructor and Destructor
// ============================================================================

FormEmbeddedWidget::FormEmbeddedWidget(const QString& formId, QWidget* parent)
    : QWidget(parent)
    , m_formId(formId)
    , m_isLoaded(false)
    , m_formGenerator(new common::forms::FormWidgetGenerator())
    , m_dataSerializer(new common::forms::FormDataSerializer())
    , m_validator(new common::forms::FormValidator())
    , m_formWidget(nullptr)
    , m_saveButton(nullptr)
    , m_statusLabel(nullptr)
    , m_autoSaveTimer(nullptr)
{
    setupUI();
}

FormEmbeddedWidget::~FormEmbeddedWidget()
{
    delete m_formWidget;
    delete m_formGenerator;
    delete m_dataSerializer;
    delete m_validator;
}

// ============================================================================
// Public Methods
// ============================================================================

bool FormEmbeddedWidget::loadForm(const QString& cartridgePath)
{
    /**
     * @brief Load form from cartridge database
     * 
     * Loads form schema from Form_Definitions table and generates form widget.
     * Also loads existing form data if available.
     * 
     * Process:
     * 1. Query Form_Definitions table for form schema
     * 2. Generate form widget from schema using FormWidgetGenerator
     * 3. Add form widget to layout
     * 4. Connect form field change signals for auto-save
     * 5. Load existing form data from User_Data table
     * 
     * @param cartridgePath Path to cartridge file
     * @return true if loaded successfully, false otherwise
     * 
     * @note Emits formLoadError() signal if load fails
     */
    m_cartridgePath = cartridgePath;
    m_isLoaded = false;
    m_errorMessage.clear();
    
    // Open cartridge database
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(cartridgePath)) {
        m_errorMessage = "Failed to open cartridge database";
        emit formLoadError(m_errorMessage);
        return false;
    }
    
    // Query Form_Definitions table for form schema
    QSqlQuery query(connector.getDatabase());
    query.prepare("SELECT form_schema_json FROM Form_Definitions WHERE form_id = ?");
    query.addBindValue(m_formId);
    
    if (!query.exec()) {
        m_errorMessage = "Failed to query form schema: " + query.lastError().text();
        connector.closeCartridge();
        emit formLoadError(m_errorMessage);
        return false;
    }
    
    if (!query.next()) {
        m_errorMessage = "Form not found: " + m_formId;
        connector.closeCartridge();
        emit formLoadError(m_errorMessage);
        return false;
    }
    
    QString schemaJson = query.value(0).toString();
    connector.closeCartridge();
    
    if (schemaJson.isEmpty()) {
        m_errorMessage = "Form schema is empty for: " + m_formId;
        emit formLoadError(m_errorMessage);
        return false;
    }
    
    // Generate form widget from schema
    m_formWidget = m_formGenerator->generateForm(schemaJson);
    if (!m_formWidget) {
        m_errorMessage = "Failed to generate form widget: " + m_formGenerator->lastError();
        emit formLoadError(m_errorMessage);
        return false;
    }
    
    // Add form widget to layout (before buttons)
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        mainLayout->insertWidget(0, m_formWidget);
    } else {
        layout()->addWidget(m_formWidget);
    }
    
    // Connect form field changes for auto-save
    connectFormFields();
    
    // Enable save button
    if (m_saveButton) {
        m_saveButton->setEnabled(true);
    }
    
    m_isLoaded = true;
    emit formLoaded();
    
    // Load existing form data if available
    loadFormData();
    
    return true;
}

bool FormEmbeddedWidget::saveFormData()
{
    /**
     * @brief Save form data to cartridge database
     * 
     * Serializes form data to JSON, validates it against schema, and saves
     * to User_Data table. Shows validation errors if validation fails.
     * 
     * Process:
     * 1. Serialize form data to JSON using FormDataSerializer
     * 2. Load form schema from database for validation
     * 3. Validate data using FormValidator
     * 4. If valid, save to User_Data table
     * 5. Show success/error status in status label
     * 
     * @return true if saved successfully, false otherwise
     * 
     * @note Emits formDataSaved() signal on completion
     */
    qDebug() << "FormEmbeddedWidget::saveFormData: Saving form" << m_formId << "to database";
    
    if (!m_isLoaded || !m_formWidget) {
        qWarning() << "FormEmbeddedWidget::saveFormData: Form not loaded";
        return false;
    }
    
    if (m_cartridgePath.isEmpty()) {
        qWarning() << "FormEmbeddedWidget::saveFormData: No cartridge path";
        return false;
    }
    
    // Serialize form data
    QString dataJson = m_dataSerializer->serializeFormData(m_formWidget);
    if (dataJson.isEmpty()) {
        qWarning() << "FormEmbeddedWidget::saveFormData: Serialized data is empty";
        emit formDataSaved(false);
        return false;
    }
    
    qDebug() << "FormEmbeddedWidget::saveFormData: Serialized data length:" << dataJson.length();
    qDebug() << "FormEmbeddedWidget::saveFormData: Data preview:" << dataJson.left(200);
    
    // Save to User_Data table (validation is done in onSaveButtonClicked before calling this)
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(m_cartridgePath)) {
        qWarning() << "FormEmbeddedWidget::saveFormData: Failed to open cartridge:" << connector.getDatabase().lastError().text();
        emit formDataSaved(false);
        return false;
    }
    
    QSqlQuery saveQuery(connector.getDatabase());
    saveQuery.prepare(R"(
        INSERT OR REPLACE INTO User_Data (form_key, serialized_data, timestamp)
        VALUES (?, ?, datetime('now'))
    )");
    saveQuery.addBindValue(m_formId);
    saveQuery.addBindValue(dataJson);
    
    bool success = saveQuery.exec();
    if (success) {
        qDebug() << "FormEmbeddedWidget::saveFormData: Data saved to User_Data table successfully";
        // Verify it was saved and get timestamp
        QSqlQuery verifyQuery(connector.getDatabase());
        verifyQuery.prepare("SELECT serialized_data, timestamp FROM User_Data WHERE form_key = ?");
        verifyQuery.addBindValue(m_formId);
        if (verifyQuery.exec() && verifyQuery.next()) {
            QString savedData = verifyQuery.value(0).toString();
            QString timestamp = verifyQuery.value(1).toString();
            qDebug() << "FormEmbeddedWidget::saveFormData: Verified in database - timestamp:" << timestamp;
            qDebug() << "FormEmbeddedWidget::saveFormData: Saved data length:" << savedData.length();
        } else {
            qWarning() << "FormEmbeddedWidget::saveFormData: Failed to verify save:" << verifyQuery.lastError().text();
        }
    } else {
        qWarning() << "FormEmbeddedWidget::saveFormData: Failed to save:" << saveQuery.lastError().text();
    }
    
    connector.closeCartridge();
    
    emit formDataSaved(success);
    return success;
}

bool FormEmbeddedWidget::loadFormData()
{
    if (!m_isLoaded || !m_formWidget) {
        return false;
    }
    
    if (m_cartridgePath.isEmpty()) {
        return false;
    }
    
    // Load from User_Data table
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(m_cartridgePath)) {
        emit formDataLoaded(false);
        return false;
    }
    
    QSqlQuery query(connector.getDatabase());
    query.prepare(R"(
        SELECT serialized_data FROM User_Data
        WHERE form_key = ?
        ORDER BY timestamp DESC
        LIMIT 1
    )");
    query.addBindValue(m_formId);
    
    if (!query.exec()) {
        connector.closeCartridge();
        emit formDataLoaded(false);
        return false;
    }
    
    if (query.next()) {
        QString dataJson = query.value(0).toString();
        connector.closeCartridge();
        
        // Load data into form widget
        bool success = m_dataSerializer->loadFormData(m_formWidget, dataJson);
        emit formDataLoaded(success);
        return success;
    }
    
    connector.closeCartridge();
    emit formDataLoaded(true); // No data is not an error
    return true;
}

// ============================================================================
// Private Methods - UI Setup
// ============================================================================

void FormEmbeddedWidget::setupUI()
{
    /**
     * @brief Setup form widget UI
     * 
     * Creates status label and save button, and sets up auto-save timer.
     * Form widget itself is created later in loadForm().
     */
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);
    
    // Status label (initially hidden)
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->hide();
    layout->addWidget(m_statusLabel);
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_saveButton = new QPushButton("Save", this);
    m_saveButton->setEnabled(false); // Disabled until form is loaded
    connect(m_saveButton, &QPushButton::clicked, this, &FormEmbeddedWidget::onSaveButtonClicked);
    buttonLayout->addWidget(m_saveButton);
    
    layout->addLayout(buttonLayout);
    
    // Auto-save timer (saves 2 seconds after last change)
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);
    m_autoSaveTimer->setInterval(2000); // 2 seconds
    connect(m_autoSaveTimer, &QTimer::timeout, this, &FormEmbeddedWidget::onSaveButtonClicked);
}

void FormEmbeddedWidget::connectFormFields()
{
    /**
     * @brief Connect form field change signals
     * 
     * Connects change signals from all form input widgets to onFormDataChanged()
     * slot. This enables auto-save functionality and save button enabling.
     * 
     * Supported widgets:
     * - QLineEdit (textChanged)
     * - QTextEdit (textChanged)
     * - QComboBox (currentIndexChanged)
     * - QCheckBox (toggled)
     * - QSpinBox (valueChanged)
     * - QDoubleSpinBox (valueChanged)
     */
    if (!m_formWidget) {
        return;
    }
    
    // Find all input widgets and connect their change signals
    QList<QLineEdit*> lineEdits = m_formWidget->findChildren<QLineEdit*>();
    for (QLineEdit* edit : lineEdits) {
        connect(edit, &QLineEdit::textChanged, this, &FormEmbeddedWidget::onFormDataChanged);
    }
    
    QList<QTextEdit*> textEdits = m_formWidget->findChildren<QTextEdit*>();
    for (QTextEdit* edit : textEdits) {
        connect(edit, &QTextEdit::textChanged, this, &FormEmbeddedWidget::onFormDataChanged);
    }
    
    QList<QComboBox*> comboBoxes = m_formWidget->findChildren<QComboBox*>();
    for (QComboBox* combo : comboBoxes) {
        connect(combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), 
                this, &FormEmbeddedWidget::onFormDataChanged);
    }
    
    QList<QCheckBox*> checkBoxes = m_formWidget->findChildren<QCheckBox*>();
    for (QCheckBox* check : checkBoxes) {
        connect(check, &QCheckBox::toggled, this, &FormEmbeddedWidget::onFormDataChanged);
    }
    
    QList<QSpinBox*> spinBoxes = m_formWidget->findChildren<QSpinBox*>();
    for (QSpinBox* spin : spinBoxes) {
        connect(spin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), 
                this, &FormEmbeddedWidget::onFormDataChanged);
    }
    
    QList<QDoubleSpinBox*> doubleSpinBoxes = m_formWidget->findChildren<QDoubleSpinBox*>();
    for (QDoubleSpinBox* spin : doubleSpinBoxes) {
        connect(spin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), 
                this, &FormEmbeddedWidget::onFormDataChanged);
    }
}

void FormEmbeddedWidget::onFormDataChanged()
{
    // Enable save button
    if (m_saveButton) {
        m_saveButton->setEnabled(true);
    }
    
    // Reset auto-save timer
    if (m_autoSaveTimer) {
        m_autoSaveTimer->stop();
        m_autoSaveTimer->start();
    }
}

void FormEmbeddedWidget::onSaveButtonClicked()
{
    qDebug() << "FormEmbeddedWidget::onSaveButtonClicked: Saving form" << m_formId;
    
    if (!m_isLoaded || !m_formWidget) {
        qWarning() << "FormEmbeddedWidget::onSaveButtonClicked: Form not loaded";
        return;
    }
    
    // Serialize form data
    QString dataJson = m_dataSerializer->serializeFormData(m_formWidget);
    qDebug() << "FormEmbeddedWidget::onSaveButtonClicked: Serialized data:" << dataJson;
    
    if (dataJson.isEmpty()) {
        qWarning() << "FormEmbeddedWidget::onSaveButtonClicked: No data to save";
        showSaveStatus(false, "No data to save");
        return;
    }
    
    // Get schema for validation
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(m_cartridgePath)) {
        qWarning() << "FormEmbeddedWidget::onSaveButtonClicked: Failed to open cartridge";
        showSaveStatus(false, "Failed to open cartridge");
        return;
    }
    
    QSqlQuery query(connector.getDatabase());
    query.prepare("SELECT form_schema_json FROM Form_Definitions WHERE form_id = ?");
    query.addBindValue(m_formId);
    
    if (!query.exec() || !query.next()) {
        qWarning() << "FormEmbeddedWidget::onSaveButtonClicked: Failed to load schema:" << query.lastError().text();
        connector.closeCartridge();
        showSaveStatus(false, "Failed to load form schema");
        return;
    }
    
    QString schemaJson = query.value(0).toString();
    connector.closeCartridge();
    
    // Validate
    qDebug() << "FormEmbeddedWidget::onSaveButtonClicked: Validating data...";
    if (!m_validator->validate(dataJson, schemaJson)) {
        QStringList errors = m_validator->errors();
        qWarning() << "FormEmbeddedWidget::onSaveButtonClicked: Validation failed:" << errors;
        showValidationErrors(errors);
        return;
    }
    
    qDebug() << "FormEmbeddedWidget::onSaveButtonClicked: Validation passed, saving to database...";
    
    // Save to database
    bool success = saveFormData();
    if (success) {
        qDebug() << "FormEmbeddedWidget::onSaveButtonClicked: Data saved successfully to database";
        showSaveStatus(true, "Form data saved successfully");
        if (m_saveButton) {
            m_saveButton->setEnabled(false); // Disable until next change
        }
    } else {
        qWarning() << "FormEmbeddedWidget::onSaveButtonClicked: Failed to save data";
        showSaveStatus(false, "Failed to save form data");
    }
}

// ============================================================================
// Private Methods - Status Display
// ============================================================================

void FormEmbeddedWidget::showValidationErrors(const QStringList& errors)
{
    /**
     * @brief Display validation errors in status label
     * 
     * Shows validation errors with theme-aware colors. Errors auto-hide
     * after 5 seconds.
     * 
     * @param errors List of error messages
     */
    if (!m_statusLabel) {
        return;
    }
    
    QString errorText = "Validation errors:\n" + errors.join("\n");
    m_statusLabel->setText(errorText);
    
    // Get current theme to set appropriate colors
    QString theme = common::utils::ThemeManager::getInstance().getTheme();
    QString resolvedTheme = common::utils::ThemeManager::getInstance().resolveTheme(theme);
    
    QString errorTextColor, errorBgColor, errorBorderColor;
    
    if (resolvedTheme == "dark") {
        errorTextColor = "#ff6b6b";  // Light red for dark theme
        errorBgColor = "#4a1f1f";     // Dark red background
        errorBorderColor = "#ff6b6b"; // Light red border
    } else if (resolvedTheme == "sepia") {
        errorTextColor = "#8b0000";   // Dark red for sepia theme
        errorBgColor = "#f5e6d3";     // Light sepia background
        errorBorderColor = "#8b0000"; // Dark red border
    } else { // light (default)
        errorTextColor = "#d32f2f";   // Red for light theme
        errorBgColor = "#ffebee";     // Light pink background
        errorBorderColor = "#d32f2f";  // Red border
    }
    
    QString stylesheet = QString("color: %1; background-color: %2; padding: 5px; border: 1px solid %3;")
                         .arg(errorTextColor, errorBgColor, errorBorderColor);
    m_statusLabel->setStyleSheet(stylesheet);
    m_statusLabel->show();
    
    // Hide after 5 seconds
    QTimer::singleShot(5000, this, [this]() {
        if (m_statusLabel) {
            m_statusLabel->hide();
        }
    });
}

void FormEmbeddedWidget::showSaveStatus(bool success, const QString& message)
{
    if (!m_statusLabel) {
        return;
    }
    
    // Get current theme to set appropriate colors
    QString theme = common::utils::ThemeManager::getInstance().getTheme();
    QString resolvedTheme = common::utils::ThemeManager::getInstance().resolveTheme(theme);
    
    QString textColor, bgColor, borderColor;
    
    if (success) {
        // Get timestamp from database to show when it was saved
        common::database::CartridgeDBConnector connector(this);
        QString timestampText = "just now";
        if (connector.openCartridge(m_cartridgePath)) {
            QSqlQuery query(connector.getDatabase());
            query.prepare("SELECT timestamp FROM User_Data WHERE form_key = ? ORDER BY timestamp DESC LIMIT 1");
            query.addBindValue(m_formId);
            if (query.exec() && query.next()) {
                timestampText = query.value(0).toString();
            }
            connector.closeCartridge();
        }
        
        QString statusText = message.isEmpty() ? "Form data saved successfully" : message;
        statusText += QString(" (Saved: %1)").arg(timestampText);
        m_statusLabel->setText(statusText);
        
        // Success colors based on theme
        if (resolvedTheme == "dark") {
            textColor = "#81c784";  // Light green for dark theme
            bgColor = "#1f3a1f";     // Dark green background
            borderColor = "#81c784"; // Light green border
        } else if (resolvedTheme == "sepia") {
            textColor = "#2e7d32";  // Dark green for sepia theme
            bgColor = "#e8f5e9";     // Light green background
            borderColor = "#2e7d32";  // Dark green border
        } else { // light (default)
            textColor = "#2e7d32";  // Green for light theme
            bgColor = "#e8f5e9";     // Light green background
            borderColor = "#4caf50";  // Green border
        }
    } else {
        m_statusLabel->setText(message.isEmpty() ? "Failed to save form data" : message);
        
        // Error colors based on theme
        if (resolvedTheme == "dark") {
            textColor = "#ff6b6b";  // Light red for dark theme
            bgColor = "#4a1f1f";     // Dark red background
            borderColor = "#ff6b6b"; // Light red border
        } else if (resolvedTheme == "sepia") {
            textColor = "#8b0000";  // Dark red for sepia theme
            bgColor = "#f5e6d3";     // Light sepia background
            borderColor = "#8b0000"; // Dark red border
        } else { // light (default)
            textColor = "#d32f2f";  // Red for light theme
            bgColor = "#ffebee";     // Light pink background
            borderColor = "#d32f2f";  // Red border
        }
    }
    
    QString stylesheet = QString("color: %1; background-color: %2; padding: 5px; border: 1px solid %3;")
                         .arg(textColor, bgColor, borderColor);
    m_statusLabel->setStyleSheet(stylesheet);
    m_statusLabel->show();
    
    // For success, keep showing the timestamp; for errors, hide after 5 seconds
    if (!success) {
        QTimer::singleShot(5000, this, [this]() {
            if (m_statusLabel) {
                m_statusLabel->hide();
            }
        });
    }
}

void FormEmbeddedWidget::applyTheme(const QColor& bgColor, const QColor& textColor)
{
    /**
     * @brief Apply theme to form widgets
     * 
     * Applies theme colors to all form widgets (form widget, inputs, buttons, labels)
     * using QPalette. This ensures forms match the selected theme.
     * 
     * @param bgColor Background color for form
     * @param textColor Text color for form
     */
    qDebug() << "FormEmbeddedWidget::applyTheme: Applying theme, bgColor=" << bgColor.name() << ", textColor=" << textColor.name();
    
    // Apply theme to this widget
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, bgColor);
    palette.setColor(QPalette::WindowText, textColor);
    palette.setColor(QPalette::Base, bgColor);
    palette.setColor(QPalette::Text, textColor);
    this->setPalette(palette);
    
    // Apply theme to save button
    if (m_saveButton) {
        QPalette buttonPalette = m_saveButton->palette();
        buttonPalette.setColor(QPalette::Button, bgColor);
        buttonPalette.setColor(QPalette::ButtonText, textColor);
        m_saveButton->setPalette(buttonPalette);
    }
    
    // Apply theme to status label
    if (m_statusLabel) {
        QPalette labelPalette = m_statusLabel->palette();
        labelPalette.setColor(QPalette::Window, bgColor);
        labelPalette.setColor(QPalette::WindowText, textColor);
        m_statusLabel->setPalette(labelPalette);
    }
    
    // Apply theme to form widget and all its children
    if (m_formWidget) {
        // Apply to form widget itself
        QPalette formPalette = m_formWidget->palette();
        formPalette.setColor(QPalette::Window, bgColor);
        formPalette.setColor(QPalette::WindowText, textColor);
        formPalette.setColor(QPalette::Base, bgColor);
        formPalette.setColor(QPalette::Text, textColor);
        m_formWidget->setPalette(formPalette);
        
        // Apply to all input widgets
        QList<QLineEdit*> lineEdits = m_formWidget->findChildren<QLineEdit*>();
        for (QLineEdit* edit : lineEdits) {
            QPalette editPalette = edit->palette();
            editPalette.setColor(QPalette::Base, bgColor);
            editPalette.setColor(QPalette::Text, textColor);
            edit->setPalette(editPalette);
        }
        
        QList<QTextEdit*> textEdits = m_formWidget->findChildren<QTextEdit*>();
        for (QTextEdit* edit : textEdits) {
            QPalette editPalette = edit->palette();
            editPalette.setColor(QPalette::Base, bgColor);
            editPalette.setColor(QPalette::Text, textColor);
            edit->setPalette(editPalette);
        }
        
        QList<QComboBox*> comboBoxes = m_formWidget->findChildren<QComboBox*>();
        for (QComboBox* combo : comboBoxes) {
            QPalette comboPalette = combo->palette();
            comboPalette.setColor(QPalette::Base, bgColor);
            comboPalette.setColor(QPalette::Text, textColor);
            combo->setPalette(comboPalette);
        }
        
        QList<QSpinBox*> spinBoxes = m_formWidget->findChildren<QSpinBox*>();
        for (QSpinBox* spin : spinBoxes) {
            QPalette spinPalette = spin->palette();
            spinPalette.setColor(QPalette::Base, bgColor);
            spinPalette.setColor(QPalette::Text, textColor);
            spin->setPalette(spinPalette);
        }
        
        QList<QDoubleSpinBox*> doubleSpinBoxes = m_formWidget->findChildren<QDoubleSpinBox*>();
        for (QDoubleSpinBox* spin : doubleSpinBoxes) {
            QPalette spinPalette = spin->palette();
            spinPalette.setColor(QPalette::Base, bgColor);
            spinPalette.setColor(QPalette::Text, textColor);
            spin->setPalette(spinPalette);
        }
        
        // Apply to labels (from QFormLayout)
        QList<QLabel*> labels = m_formWidget->findChildren<QLabel*>();
        for (QLabel* label : labels) {
            QPalette labelPalette = label->palette();
            labelPalette.setColor(QPalette::WindowText, textColor);
            label->setPalette(labelPalette);
        }
    }
    
    qDebug() << "FormEmbeddedWidget::applyTheme: Theme applied successfully";
}

} // namespace reader
} // namespace smartbook

