#ifndef PIMPLE_WIDGET_H
#define PIMPLE_WIDGET_H

#include <QFormLayout>
#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>

#include "solver_structs.h"

class PimpleWidget : public QWidget {
    Q_OBJECT

public:
    explicit PimpleWidget(QWidget *parent = nullptr);

    // The clean API for the Wizard Page to use
    PimpleConfig getConfig() const;
    void setConfig(const PimpleConfig& config);

signals:
    void configChanged();

private:
    QSpinBox* m_nOuterCorrectors;
    QSpinBox* m_nCorrectors;
    QSpinBox* m_nNonOrthogonalCorrectors;
    QSpinBox* m_pRefCell;
    QDoubleSpinBox* m_pRefValue;
};

#endif // PIMPLE_WIDGET_H