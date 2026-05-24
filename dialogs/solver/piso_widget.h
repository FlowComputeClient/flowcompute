#ifndef PISO_WIDGET_H
#define PISO_WIDGET_H

#include <QFormLayout>
#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>

#include "solver_structs.h"

class PisoWidget : public QWidget {
    Q_OBJECT

public:
    explicit PisoWidget(QWidget *parent = nullptr);

    // The clean API for the Wizard Page to use
    PisoConfig getConfig() const;
    void setConfig(const PisoConfig& config);

signals:
    void configChanged();

private:
    QSpinBox* m_nCorrectors;
    QSpinBox* m_nNonOrthogonalCorrectors;
    QSpinBox* m_pRefCell;
    QDoubleSpinBox* m_pRefValue;
};

#endif // PISO_WIDGET_H