#ifndef SIMPLE_WIDGET_H
#define SIMPLE_WIDGET_H

#include <QCheckBox>
#include <QFormLayout>
#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>

#include "solver_structs.h"

class SimpleWidget : public QWidget {
    Q_OBJECT

public:
    explicit SimpleWidget(QWidget *parent = nullptr);

    // The clean API for the Wizard Page to use
    SimpleConfig getConfig() const;
    void setConfig(const SimpleConfig& config);

signals:
    void configChanged();

private:
    QSpinBox* m_nNonOrthogonalCorrectors;
    QCheckBox* m_consistentCheck;
    QSpinBox* m_pRefCell;
    QDoubleSpinBox* m_pRefValue;
};

#endif // SIMPLE_WIDGET_H