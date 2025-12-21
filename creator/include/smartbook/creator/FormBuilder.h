#ifndef SMARTBOOK_CREATOR_FORMBUILDER_H
#define SMARTBOOK_CREATOR_FORMBUILDER_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QToolBar>

namespace smartbook {
namespace creator {

class FormManager;

/**
 * @brief Form builder widget
 * 
 * Provides visual interface for creating and editing form definitions (JSON schema).
 * Implements FR-CT-3.16 through FR-CT-3.19
 */
class FormBuilder : public QWidget {
    Q_OBJECT

public:
    explicit FormBuilder(QWidget* parent = nullptr);
    ~FormBuilder();

    /**
     * @brief Load form definition for editing
     * @param formId Form identifier
     * @param schemaJson JSON schema string
     * @return true if loaded successfully
     */
    bool loadFormDefinition(const QString& formId, const QString& schemaJson);

    /**
     * @brief Get current form definition as JSON
     * @return JSON schema string
     */
    QString getFormDefinitionJson() const;

    /**
     * @brief Get form ID
     * @return Current form ID, or empty if new form
     */
    QString getFormId() const { return m_formId; }

    /**
     * @brief Set form ID
     * @param formId Form identifier
     */
    void setFormId(const QString& formId);

    /**
     * @brief Validate form schema
     * @return true if valid, false otherwise
     */
    bool validateFormSchema() const;

    /**
     * @brief Get validation errors
     * @return List of validation error messages
     */
    QStringList getValidationErrors() const;
    
    /**
     * @brief Save form to cartridge via FormManager
     * @param formManager FormManager instance
     * @return true if saved successfully
     */
    bool saveToCartridge(FormManager* formManager);

signals:
    void formChanged();
    void validationStatusChanged(bool isValid);

public slots:
    void addTextField();
    void addNumberField();
    void addTextareaField();
    void addSelectField();
    void addGroupField();
    void removeField(int index);
    void moveFieldUp(int index);
    void moveFieldDown(int index);

private slots:
    void onFieldChanged();
    void updateFormJson();

private:
    void setupUI();
    void setupToolbar();
    void refreshFieldList();
    QJsonObject createFieldObject(const QString& fieldId, const QString& fieldType, 
                                   const QString& label) const;
    QJsonObject buildFormSchema() const;
    bool validateField(const QJsonObject& field, QStringList& errors) const;
    QString generateFieldId(const QString& fieldType) const;
    
    QString m_formId;
    QString m_formTitle;
    QJsonArray m_fields;
    
    QListWidget* m_fieldList;
    QLineEdit* m_formIdEdit;
    QLineEdit* m_formTitleEdit;
    QTextEdit* m_jsonPreview;
    QLabel* m_validationStatus;
    QToolBar* m_toolbar;
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_FORMBUILDER_H
