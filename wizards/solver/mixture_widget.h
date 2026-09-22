#ifndef WIZARDS_SOLVER_MIXTURE_WIDGET_H_
#define WIZARDS_SOLVER_MIXTURE_WIDGET_H_

#include "parser/thermo_physical_properties.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QWidget>

class MixtureWidget : public QWidget {
    Q_OBJECT

 public:
    explicit MixtureWidget(QWidget* parent = nullptr) : QWidget(parent) {}
    virtual ~MixtureWidget() = default;

    virtual CaseIO::MixtureVariant getData() const = 0;

 protected:
    // Storage fields
    QMap<QString, QDoubleSpinBox*> m_scalarInputs;
    QMap<QString, QLineEdit*> m_vectorInputs;

    // Receive scalar input
    void addScalarField(QFormLayout* layout, const QString& labelText,
                        const QString& dictKey) {
        QDoubleSpinBox* spin = new QDoubleSpinBox(this);
        spin->setRange(-1e12, 1e12);
        spin->setDecimals(6);
        spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
        layout->addRow(labelText, spin);
        m_scalarInputs[dictKey] = spin;
    }

    // Receive vector of values
    void addVectorField(QFormLayout* layout, const QString& labelText,
                        const QString& dictKey) {
        QLineEdit* edit = new QLineEdit(this);
        edit->setPlaceholderText("Space-separated values...");
        layout->addRow(labelText, edit);
        m_vectorInputs[dictKey] = edit;
    }
};

#endif // WIZARDS_SOLVER_MIXTURE_WIDGET_H_
