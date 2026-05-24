#ifndef PAGE_30_BLOCKMESH_H
#define PAGE_30_BLOCKMESH_H

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

#include "../../geometry/mesh_data.h"
#include "../../geometry/stl/stl_reader.h"
#include "mesh_structs.h"

class MainWindow;
class MeshWizard;

class BlockMeshPage2 : public QWizardPage {
    Q_OBJECT

public:
    explicit BlockMeshPage2(QWidget *parent);
    int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    BlockMeshConfig* m_cfg;
    MeshWizard* meshWizard;
    QTableWidget* patchTable;
    QCheckBox* gradingCheckBox;
    std::array<QDoubleSpinBox*, 3> gradingSpinBox;

    void updateBoundaryGroup();
    void updateGradingGroup();
    int identifyFaceIndex(std::array<int, 4> faceArray);

    // A single, shared source of truth for the 6 hex faces
    static constexpr std::array<std::array<int, 4>, 6> CANONICAL_FACES = {{
        {0, 3, 4, 7}, // 0: X-Min (Left)
        {1, 2, 5, 6}, // 1: X-Max (Right)
        {0, 1, 4, 5}, // 2: Y-Min (Front)
        {2, 3, 6, 7}, // 3: Y-Max (Back)
        {0, 1, 2, 3}, // 4: Z-Min (Bottom)
        {4, 5, 6, 7}  // 5: Z-Max (Top)
    }};
};

#endif  // PAGE_30_BLOCKMESH_H
