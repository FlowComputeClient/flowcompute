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

#include "../../geometry/mesh_data.h"
#include "../../geometry/stl/stl_reader.h"
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
    QComboBox *scaleFactorBox;
    QLabel* geometryLabel;
    std::array<QDoubleSpinBox*, 6> dimSpinBox;
    std::array<double, 3> minGeometry;
    std::array<double, 3> maxGeometry;
    void setBoundingBox();
    void updateCellCount();
    double m_cellSize = -1.0;
    QDoubleSpinBox *targetCellSizeBox;
    QLineEdit* cellCountX;
    QLineEdit* cellCountY;
    QLineEdit* cellCountZ;
    QLineEdit* cellCountTotal;

private slots:
    void fitBoundsPressed();
};

#endif  // PAGE_20_BLOCKMESH_H
