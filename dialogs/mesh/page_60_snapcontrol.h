#ifndef PAGE_60_SNAPCONTROL_H
#define PAGE_60_SNAPCONTROL_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
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

class SnapControlPage : public QWizardPage {
    Q_OBJECT

public:
    explicit SnapControlPage(QWidget *parent);
    int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    SnapControlConfig* m_cfg;
    MeshWizard* meshWizard;
    QSpinBox *smoothingBox, *maxSnappingBox, *relaxationBox, *snapIterationBox;
    QCheckBox *explicitSnapBox, *implicitSnapBox;
    QDoubleSpinBox *toleranceBox;
};

#endif  // PAGE_60_SNAPCONTROL_H
