#ifndef PAGE_70_LAYERCONTROL_H
#define PAGE_70_LAYERCONTROL_H

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

class LayerControlPage : public QWizardPage {
    Q_OBJECT

public:
    explicit LayerControlPage(QWidget *parent);

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    LayerControlConfig *m_cfg;
    MeshWizard *meshWizard;

    QCheckBox *relativeSizesCheck;
    QDoubleSpinBox *expansionRatioSpin, *finalLayerSpin, *minThicknessSpin, *featureAngleSpin;
    QSpinBox *layerIterSpin, *surfaceSmoothingSpin, *internalSmoothingSpin, *thicknessSmoothingSpin;
    QTableWidget* surfaceLayerTable;
};

#endif  // PAGE_70_LAYERCONTROL_H
