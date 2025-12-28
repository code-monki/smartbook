#ifndef SMARTBOOK_READER_UI_FORMEMBEDDEDWIDGET_H
#define SMARTBOOK_READER_UI_FORMEMBEDDEDWIDGET_H

#include <QWidget>
#include <QString>

class QPushButton;
class QLabel;
class QTimer;

namespace smartbook {
namespace common {
namespace forms {
    class FormWidgetGenerator;
    class FormDataSerializer;
    class FormValidator;
}
namespace database {
    class CartridgeDBConnector;
}
}

namespace reader {

/**
 * @brief Widget for embedding forms in content pages
 * 
 * FormEmbeddedWidget hosts a Qt Widgets form within a cartridge content page.
 * It loads form schemas from the cartridge database (Form_Definitions table),
 * generates form widgets dynamically, and handles data persistence and validation.
 * 
 * @section Architecture
 * 
 * **Form Generation:**
 * - Form schema is loaded from `Form_Definitions.form_schema_json` column
 * - Schema uses JSON Schema format (Draft 7 compatible)
 * - FormWidgetGenerator creates Qt Widgets from schema
 * - Supports: text, textarea, select, checkbox, radio, number, date fields
 * 
 * **Data Persistence:**
 * - Form data is saved to `User_Data` table in cartridge database
 * - Data is serialized as JSON using FormDataSerializer
 * - Auto-save occurs 2 seconds after last change
 * - Manual save via "Save" button
 * 
 * **Validation:**
 * - FormValidator validates data against schema rules
 * - Checks: required fields, types, ranges, patterns
 * - Validation errors displayed in status label
 * - Data is only saved if validation passes
 * 
 * **Theme Support:**
 * - Forms adapt to selected theme (light, dark, sepia)
 * - Theme colors applied via QPalette
 * - All form widgets (inputs, buttons, labels) are theme-aware
 * 
 * @section Usage
 * 
 * @code
 * FormEmbeddedWidget* widget = new FormEmbeddedWidget("contact_form", parent);
 * bool loaded = widget->loadForm(cartridgePath);
 * if (loaded) {
 *     // Form is ready to use
 *     // Data is automatically loaded from database if available
 * }
 * @endcode
 * 
 * @section Form Schema Format
 * 
 * Forms use JSON Schema format:
 * @code
 * {
 *   "type": "object",
 *   "title": "Contact Form",
 *   "properties": {
 *     "name": {
 *       "type": "string",
 *       "title": "Name",
 *       "defaultValue": "",
 *       "placeholder": "Enter your name"
 *     },
 *     "email": {
 *       "type": "string",
 *       "format": "email",
 *       "title": "Email"
 *     }
 *   },
 *   "required": ["name", "email"]
 * }
 * @endcode
 * 
 * @section User Experience
 * 
 * **Save Button:**
 * - Enabled when form data changes
 * - Disabled after successful save
 * - Triggers validation before saving
 * 
 * **Status Label:**
 * - Shows validation errors (red, auto-hides after 5 seconds)
 * - Shows save success with timestamp (green, persistent)
 * - Shows save failures (red, auto-hides after 5 seconds)
 * 
 * **Auto-Save:**
 * - Saves automatically 2 seconds after last change
 * - Reduces risk of data loss
 * - Validation still occurs before auto-save
 * 
 * @note This widget is created automatically by ReaderView when it detects
 * form markers in HTML content. Manual instantiation is rarely needed.
 * 
 * @see ReaderView
 * @see FormSchemaParser
 * @see FormWidgetGenerator
 * @see FormValidator
 * @see FormDataSerializer
 * @see qt-widgets-forms-guide.adoc
 */
class FormEmbeddedWidget : public QWidget {
    Q_OBJECT

public:
    explicit FormEmbeddedWidget(const QString& formId, QWidget* parent = nullptr);
    ~FormEmbeddedWidget();
    
    /**
     * @brief Load form from cartridge database
     * @param cartridgePath Path to cartridge file
     * @return true if loaded successfully, false otherwise
     */
    bool loadForm(const QString& cartridgePath);
    
    /**
     * @brief Get form ID
     */
    QString formId() const { return m_formId; }
    
    /**
     * @brief Check if form is loaded
     */
    bool isLoaded() const { return m_isLoaded; }
    
    /**
     * @brief Get error message if load failed
     */
    QString errorMessage() const { return m_errorMessage; }
    
    /**
     * @brief Save form data to cartridge
     * @return true if saved successfully, false otherwise
     */
    bool saveFormData();
    
    /**
     * @brief Load form data from cartridge
     * @return true if loaded successfully, false otherwise
     */
    bool loadFormData();
    
    /**
     * @brief Apply theme to form widgets
     * @param bgColor Background color
     * @param textColor Text color
     */
    void applyTheme(const QColor& bgColor, const QColor& textColor);

signals:
    void formLoaded();
    void formLoadError(const QString& error);
    void formDataSaved(bool success);
    void formDataLoaded(bool success);

private slots:
    void onSaveButtonClicked();
    void onFormDataChanged();

private:
    void setupUI();
    void connectFormFields();
    void showValidationErrors(const QStringList& errors);
    void showSaveStatus(bool success, const QString& message = QString());
    
    QString m_formId;
    QString m_cartridgePath;
    bool m_isLoaded;
    QString m_errorMessage;
    
    common::forms::FormWidgetGenerator* m_formGenerator;
    common::forms::FormDataSerializer* m_dataSerializer;
    common::forms::FormValidator* m_validator;
    QWidget* m_formWidget;
    
    QPushButton* m_saveButton;
    QLabel* m_statusLabel;
    QTimer* m_autoSaveTimer;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_UI_FORMEMBEDDEDWIDGET_H

