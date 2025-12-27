#ifndef SMARTBOOK_READER_UI_FORMEMBEDDEDWIDGET_H
#define SMARTBOOK_READER_UI_FORMEMBEDDEDWIDGET_H

#include <QWidget>
#include <QString>

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
 * @brief Widget for embedded form in content
 * 
 * Displays a form widget generated from a JSON schema stored in the cartridge.
 * Handles form data persistence and validation.
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

signals:
    void formLoaded();
    void formLoadError(const QString& error);
    void formDataSaved(bool success);
    void formDataLoaded(bool success);

private:
    QString m_formId;
    QString m_cartridgePath;
    bool m_isLoaded;
    QString m_errorMessage;
    
    common::forms::FormWidgetGenerator* m_formGenerator;
    common::forms::FormDataSerializer* m_dataSerializer;
    common::forms::FormValidator* m_validator;
    QWidget* m_formWidget;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_UI_FORMEMBEDDEDWIDGET_H

