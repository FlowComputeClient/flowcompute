#ifndef SURFACE_DIALOG_H
#define SURFACE_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>

class AddSurfaceDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddSurfaceDialog(QWidget *parent = nullptr);

    QString getSurfaceName() const { return m_nameEdit->text().trimmed(); }
    int getSurfaceType() const { return m_typeCombo->currentData().toInt(); }

private:
    QLineEdit* m_nameEdit;
    QComboBox* m_typeCombo;

private slots:
    void validateAndAccept();
};

#endif // SURFACE_DIALOG_H
