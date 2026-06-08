#ifndef PAGE_20_BLOCKMESH_H
#define PAGE_20_BLOCKMESH_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWizardPage>

#include "../../geometry/graphic_data.h"
#include "mesh_structs.h"

class MainWindow;
class MeshWizard;

class BlockMeshPage1 : public QWizardPage {
    Q_OBJECT

public:
    explicit BlockMeshPage1(QWidget *parent);

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    BlockMeshConfig* m_cfg;
    MeshWizard* meshWizard;
    BoundingBox m_rawGeomBox;
    QComboBox *scaleFactorBox;
    QLabel* geometryLabel;
    std::array<QDoubleSpinBox*, 6> dimSpin;
    std::array<double, 3> minGeometry, maxGeometry;
    void setBoundingBox();
    void updateCellCount();
    double m_cellSize = -1.0;
    QDoubleSpinBox *targetCellSizeSpin;
    std::array<QLineEdit*, 3> cellCountEdits, actualSizeEdits;
    QLineEdit *cellCountTotalEdit, *maxAspectRatioEdit;

    double m_previousScaleFactor = 1.0;
    // double getCurrentScaleFactor() const;
    double getCurrentScaleFactor(const QString& text) const;

private slots:
    void fitBoundsPressed();
    void onScaleFactorChanged(const QString& text);
};

#endif  // PAGE_20_BLOCKMESH_H
