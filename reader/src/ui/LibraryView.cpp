#include "smartbook/reader/ui/LibraryView.h"
#include "smartbook/common/database/LocalDBManager.h"
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPixmap>
#include <QIcon>
#include <QDebug>

namespace smartbook {
namespace reader {

LibraryView::LibraryView(QWidget* parent)
    : QWidget(parent)
    , m_stackedWidget(nullptr)
    , m_tableView(nullptr)
    , m_gridView(nullptr)
    , m_listModel(nullptr)
    , m_gridModel(nullptr)
    , m_isListView(true)
{
    setupUI();
}

LibraryView::~LibraryView() {
}

void LibraryView::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Stacked widget to switch between views
    m_stackedWidget = new QStackedWidget(this);
    layout->addWidget(m_stackedWidget);
    
    setupListView();
    setupBookshelfView();
    
    // Set initial view
    updateView();
    
    loadCartridges();
}

void LibraryView::setupListView() {
    m_tableView = new QTableView(this);
    m_listModel = new QStandardItemModel(this);
    
    // Set up columns: Title, Author, Edition/Version, Year of Publication
    // DDD 11.1: List-View mandatory columns
    m_listModel->setColumnCount(4);
    m_listModel->setHeaderData(0, Qt::Horizontal, "Title");
    m_listModel->setHeaderData(1, Qt::Horizontal, "Author");
    m_listModel->setHeaderData(2, Qt::Horizontal, "Edition/Version");
    m_listModel->setHeaderData(3, Qt::Horizontal, "Year of Publication");
    
    m_tableView->setModel(m_listModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSortingEnabled(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    
    connect(m_tableView, &QTableView::doubleClicked,
            this, &LibraryView::onTableDoubleClicked);
    
    m_stackedWidget->addWidget(m_tableView);
}

void LibraryView::setupBookshelfView() {
    m_gridView = new QListView(this);
    m_gridModel = new QStandardItemModel(this);
    
    // Bookshelf View: Grid layout with cover images
    // DDD 11.1: Bookshelf View - grid with cover images
    m_gridView->setViewMode(QListView::IconMode);
    m_gridView->setResizeMode(QListView::Adjust);
    m_gridView->setGridSize(QSize(150, 200));
    m_gridView->setSpacing(10);
    
    m_gridView->setModel(m_gridModel);
    
    connect(m_gridView, &QListView::doubleClicked,
            this, &LibraryView::onItemDoubleClicked);
    
    m_stackedWidget->addWidget(m_gridView);
}

void LibraryView::updateView() {
    if (m_isListView) {
        m_stackedWidget->setCurrentWidget(m_tableView);
    } else {
        m_stackedWidget->setCurrentWidget(m_gridView);
    }
}

void LibraryView::refreshLibrary() {
    loadCartridges();
}

void LibraryView::toggleView() {
    m_isListView = !m_isListView;
    updateView();
    // Views are already loaded, just switch display
    // AC: Both views load instantly
}

void LibraryView::loadCartridges() {
    smartbook::common::database::LocalDBManager& dbManager = 
        smartbook::common::database::LocalDBManager::getInstance();

    if (!dbManager.isOpen()) {
        return;
    }

    // Clear existing data
    m_listModel->clear();
    m_gridModel->clear();
    
    // Set up list model headers
    m_listModel->setColumnCount(4);
    m_listModel->setHeaderData(0, Qt::Horizontal, "Title");
    m_listModel->setHeaderData(1, Qt::Horizontal, "Author");
    m_listModel->setHeaderData(2, Qt::Horizontal, "Edition/Version");
    m_listModel->setHeaderData(3, Qt::Horizontal, "Year of Publication");
    
    // Query manifest for all required fields
    // DDD 11.1: List-View columns sourced from manifest
    QSqlQuery query = dbManager.executeQuery(
        "SELECT cartridge_guid, title, author, version, publication_year, cover_image_data "
        "FROM Local_Library_Manifest ORDER BY title"
    );

    while (query.next()) {
        QString guid = query.value(0).toString();
        QString title = query.value(1).toString();
        QString author = query.value(2).toString();
        QString version = query.value(3).toString();
        QString year = query.value(4).toString();
        QByteArray coverImage = query.value(5).toByteArray();
        
        // List View: Add row with all columns
        QList<QStandardItem*> rowItems;
        QStandardItem* titleItem = new QStandardItem(title);
        titleItem->setData(guid, Qt::UserRole);
        rowItems.append(titleItem);
        rowItems.append(new QStandardItem(author));
        rowItems.append(new QStandardItem(version));
        rowItems.append(new QStandardItem(year));
        m_listModel->appendRow(rowItems);
        
        // Bookshelf View: Add item with title and cover
        QStandardItem* gridItem = new QStandardItem(title);
        gridItem->setData(guid, Qt::UserRole);
        if (!coverImage.isEmpty()) {
            QPixmap pixmap;
            if (pixmap.loadFromData(coverImage)) {
                gridItem->setIcon(QIcon(pixmap.scaled(120, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            }
        }
        m_gridModel->appendRow(gridItem);
    }
}

void LibraryView::onItemDoubleClicked(const QModelIndex& index) {
    QString guid = index.data(Qt::UserRole).toString();
    if (!guid.isEmpty()) {
        emit cartridgeDoubleClicked(guid);
    }
}

void LibraryView::onTableDoubleClicked(const QModelIndex& index) {
    QString guid = m_listModel->item(index.row(), 0)->data(Qt::UserRole).toString();
    if (!guid.isEmpty()) {
        emit cartridgeDoubleClicked(guid);
    }
}

} // namespace reader
} // namespace smartbook
