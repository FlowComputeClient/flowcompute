#include "surface_dialog.h"

#include "../../dialogs/solver/solver_structs.h"

#include <QMetaEnum>

AddSurfaceDialog::AddSurfaceDialog(QWidget *parent)
    : QDialog(parent) {

    setWindowTitle(tr("Add New Surface"));
    setModal(true);
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(15);

    // Name Input
    m_nameEdit = new QLineEdit(this);
    layout->addRow(tr("Surface Name:"), m_nameEdit);

    // Type Selection
    m_typeCombo = new QComboBox(this);
    QMetaEnum metaEnum = QMetaEnum::fromType<Solver::SurfaceType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_typeCombo->addItem(metaEnum.key(i), metaEnum.value(i));
    }
    layout->addRow(tr("Surface Type:"), m_typeCombo);

    // Standard OK/Cancel Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addRow(buttonBox);

    // Connections
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddSurfaceDialog::validateAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    this->adjustSize();
}

void AddSurfaceDialog::validateAndAccept() {
    if (m_nameEdit->text().trimmed().isEmpty()) {
        // Optional: Show a small warning tooltip or message box
        return; // Prevent closing if the name is empty
    }
    accept(); // Closes dialog and returns QDialog::Accepted
}