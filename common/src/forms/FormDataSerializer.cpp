#include "smartbook/common/forms/FormDataSerializer.h"
#include <QFormLayout>
#include <QLayoutItem>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QDate>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>

namespace smartbook {
namespace common {
namespace forms {

FormDataSerializer::FormDataSerializer()
{
}

FormDataSerializer::~FormDataSerializer()
{
}

QString FormDataSerializer::serializeFormData(QWidget* formWidget)
{
    m_lastError.clear();
    
    if (!formWidget) {
        m_lastError = "Form widget is null";
        return QString();
    }
    
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    if (!layout) {
        m_lastError = "Form widget does not have a QFormLayout";
        return QString();
    }
    
    QJsonObject data;
    
    // Iterate through form layout rows
    for (int i = 0; i < layout->rowCount(); ++i) {
        QLayoutItem* fieldItem = layout->itemAt(i, QFormLayout::FieldRole);
        if (!fieldItem) {
            continue;
        }
        
        QWidget* fieldWidget = fieldItem->widget();
        if (!fieldWidget) {
            continue;
        }
        
        // Get field name from objectName
        QString fieldName = fieldWidget->objectName();
        if (fieldName.isEmpty()) {
            continue;
        }
        
        // Extract value from widget
        QJsonValue value = extractWidgetValue(fieldWidget);
        if (!value.isNull()) {
            data[fieldName] = value;
        }
    }
    
    QJsonDocument doc(data);
    return doc.toJson(QJsonDocument::Compact);
}

bool FormDataSerializer::loadFormData(QWidget* formWidget, const QString& jsonData)
{
    m_lastError.clear();
    
    if (!formWidget) {
        m_lastError = "Form widget is null";
        return false;
    }
    
    // Parse JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = "JSON parse error: " + parseError.errorString();
        return false;
    }
    
    if (!doc.isObject()) {
        m_lastError = "Data must be a JSON object";
        return false;
    }
    
    QJsonObject data = doc.object();
    
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    if (!layout) {
        m_lastError = "Form widget does not have a QFormLayout";
        return false;
    }
    
    // Iterate through form layout rows
    for (int i = 0; i < layout->rowCount(); ++i) {
        QLayoutItem* fieldItem = layout->itemAt(i, QFormLayout::FieldRole);
        if (!fieldItem) {
            continue;
        }
        
        QWidget* fieldWidget = fieldItem->widget();
        if (!fieldWidget) {
            continue;
        }
        
        // Get field name from objectName
        QString fieldName = fieldWidget->objectName();
        if (fieldName.isEmpty()) {
            continue;
        }
        
        // Set value if present in data
        if (data.contains(fieldName)) {
            QJsonValue value = data[fieldName];
            if (!setWidgetValue(fieldWidget, value)) {
                qWarning() << "Failed to set value for field:" << fieldName;
            }
        }
    }
    
    return true;
}

QJsonValue FormDataSerializer::extractWidgetValue(QWidget* widget)
{
    if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
        return lineEdit->text();
    } else if (QTextEdit* textEdit = qobject_cast<QTextEdit*>(widget)) {
        return textEdit->toPlainText();
    } else if (QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
        return comboBox->currentText();
    } else if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
        return checkBox->isChecked();
    } else if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
        return spinBox->value();
    } else if (QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
        return doubleSpinBox->value();
    } else if (QDateEdit* dateEdit = qobject_cast<QDateEdit*>(widget)) {
        return dateEdit->date().toString(Qt::ISODate);
    } else if (QRadioButton* radioButton = qobject_cast<QRadioButton*>(widget)) {
        // For radio buttons, return value only if checked
        if (radioButton->isChecked()) {
            return radioButton->text();
        }
    }
    
    return QJsonValue();
}

bool FormDataSerializer::setWidgetValue(QWidget* widget, const QJsonValue& value)
{
    if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget)) {
        lineEdit->setText(value.toString());
        return true;
    } else if (QTextEdit* textEdit = qobject_cast<QTextEdit*>(widget)) {
        textEdit->setPlainText(value.toString());
        return true;
    } else if (QComboBox* comboBox = qobject_cast<QComboBox*>(widget)) {
        int index = comboBox->findText(value.toString());
        if (index >= 0) {
            comboBox->setCurrentIndex(index);
            return true;
        }
        return false;
    } else if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
        checkBox->setChecked(value.toBool());
        return true;
    } else if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
        spinBox->setValue(value.toInt());
        return true;
    } else if (QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
        doubleSpinBox->setValue(value.toDouble());
        return true;
    } else if (QDateEdit* dateEdit = qobject_cast<QDateEdit*>(widget)) {
        QDate date = QDate::fromString(value.toString(), Qt::ISODate);
        if (date.isValid()) {
            dateEdit->setDate(date);
            return true;
        }
        return false;
    } else if (QRadioButton* radioButton = qobject_cast<QRadioButton*>(widget)) {
        // For radio buttons, check if text matches
        if (radioButton->text() == value.toString()) {
            radioButton->setChecked(true);
            return true;
        }
    }
    
    return false;
}

} // namespace forms
} // namespace common
} // namespace smartbook

