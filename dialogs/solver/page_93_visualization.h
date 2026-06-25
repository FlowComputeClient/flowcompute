#ifndef PAGE_93_VISUALIZATION_H
#define PAGE_93_VISUALIZATION_H

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QLineEdit>
#include <QListWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QWizardPage>

#include "solver_structs.h"

class SolverWizard;

class VisualizationPage : public QWizardPage {
    Q_OBJECT

public:
    explicit VisualizationPage(QWidget *parent);

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    SolverWizard* solverWizard;
    bool m_isESI = false;

    QIntValidator* m_intValidator;
    QDoubleValidator* m_doubleValidator;

    QComboBox *m_surfaceTypeCombo, *m_surfaceFormatCombo, *m_writeControlCombo, *m_interpolationCombo, *m_typeCombo;
    QLineEdit *m_writeIntervalEdit;
    QListWidget *m_fieldListWidget, *m_surfaceListWidget;
    QStackedWidget *m_surfaceStack;

    const std::unordered_map<Solver::SurfaceType, QStringList> m_surfaceParameters = {
        {Solver::SurfaceType::patch, {"patches", "interpolate"}},
        {Solver::SurfaceType::cuttingPlane, {"planeType", "basePoint", "normalVector", "interpolate"}},
        {Solver::SurfaceType::isoSurface, {"isoField", "isoValue", "interpolate", "regularise"}},
        {Solver::SurfaceType::isoSurfaceCell, {"isoField", "isoValue", "interpolate", "regularise"}},
        {Solver::SurfaceType::meshedSurface, {"surface", "source", "interpolate"}},
        {Solver::SurfaceType::distanceSurface, {"distance", "signed", "surface", "interpolate"}}
    };

private slots:
    void addSurface();
    void removeSurface();
};

#endif // PAGE_93_VISUALIZATION_H
