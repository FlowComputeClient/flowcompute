#include "selection_dialog.h"

SelectionDialog::SelectionDialog(const QString& title, const QString& prompt,
                                 const QStringList& items, QWidget* parent): QDialog(parent) {

    setWindowTitle(title);
    setMinimumWidth(300);

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Add the dynamic prompt
    QLabel* label = new QLabel(prompt, this);
    layout->addWidget(label);

    // Add the scalable list
    m_listWidget = new QListWidget(this);
    m_listWidget->addItems(items);
    if (!items.isEmpty()) {
        m_listWidget->setCurrentRow(0);
    }
    layout->addWidget(m_listWidget);

    // Standard OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    // Connections
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // The UX Magic: Double-clicking an item acts like clicking OK
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
}

QString SelectionDialog::getSelectedItem() const {
    if (QListWidgetItem* item = m_listWidget->currentItem()) {
        return item->text();
    }
    return QString();
}