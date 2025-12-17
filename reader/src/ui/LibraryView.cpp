#include "smartbook/reader/ui/LibraryView.h"
#include "smartbook/common/database/LocalDBManager.h"
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QVBoxLayout>
#include <QDebug>

namespace smartbook {
namespace reader {

LibraryView::LibraryView(QWidget* parent)
    : QWidget(parent)
    , m_listView(nullptr)
{
    setupUI();
}

LibraryView::~LibraryView() {
}

void LibraryView::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    m_listView = new QListView(this);
    layout->addWidget(m_listView);

    connect(m_listView, &QListView::doubleClicked,
            this, &LibraryView::onItemDoubleClicked);

    loadCartridges();
}

void LibraryView::refreshLibrary() {
    loadCartridges();
}

void LibraryView::toggleView() {
    m_isListView = !m_isListView;
    // TODO: Switch between ListView and GridView
    loadCartridges();
}

void LibraryView::loadCartridges() {
    smartbook::common::database::LocalDBManager& dbManager = 
        smartbook::common::database::LocalDBManager::getInstance();

    if (!dbManager.isOpen()) {
        return;
    }

    QStandardItemModel* model = new QStandardItemModel(this);
    model->setColumnCount(1);
    model->setHeaderData(0, Qt::Horizontal, "Cartridges");

    QSqlQuery query = dbManager.executeQuery(
        "SELECT cartridge_guid, title, author FROM Local_Library_Manifest ORDER BY title"
    );

    while (query.next()) {
        QString guid = query.value(0).toString();
        QString title = query.value(1).toString();
        QString author = query.value(2).toString();
        QString displayText = QString("%1 - %2").arg(title, author);

        QStandardItem* item = new QStandardItem(displayText);
        item->setData(guid, Qt::UserRole);
        model->appendRow(item);
    }

    m_listView->setModel(model);
}

void LibraryView::onItemDoubleClicked(const QModelIndex& index) {
    QString guid = index.data(Qt::UserRole).toString();
    if (!guid.isEmpty()) {
        emit cartridgeDoubleClicked(guid);
    }
}

} // namespace reader
} // namespace smartbook
