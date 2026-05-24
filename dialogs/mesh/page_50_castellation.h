#ifndef PAGE_50_CASTELLATION_H
#define PAGE_50_CASTELLATION_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWizardPage>

#include "mesh_structs.h"

class MainWindow;
class MeshWizard;

class CastellationPage : public QWizardPage {
    Q_OBJECT

public:
    explicit CastellationPage(QWidget *parent);
    int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    CastellatedMeshConfig* m_cfg;
    MeshWizard* meshWizard;
    QComboBox* meshRegionBox;
    QSpinBox *maxGlobalCellBox, *maxLocalCellBox, *cellLevelBox;
    QDoubleSpinBox *featureAngleBox;
    std::array<QDoubleSpinBox*, 3> loc;
    QTableWidget* meshRefinementTable;
    QCheckBox* freeStandingZoneBox;

    QVector3D computeExternalPoint();

    void updateGlobalSettings();
    void updateMeshTransition();
    void updateSurfaceRefinement();

private slots:
    void onMeshRegionChanged();
    void onMeshLocationChanged();
};

#endif  // PAGE_50_CASTELLATION_H
