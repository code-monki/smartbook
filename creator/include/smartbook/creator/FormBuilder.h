#ifndef SMARTBOOK_CREATOR_FORMBUILDER_H
#define SMARTBOOK_CREATOR_FORMBUILDER_H

#include <QWidget>
#include <QString>

namespace smartbook {
namespace creator {

/**
 * @brief Form builder widget
 * 
 * Provides interface for creating and editing form definitions (JSON schema).
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
     */
    void loadFormDefinition(const QString& formId, const QString& schemaJson);

    /**
     * @brief Get current form definition as JSON
     * @return JSON schema string
     */
    QString getFormDefinitionJson() const;

private:
    void setupUI();
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_FORMBUILDER_H
