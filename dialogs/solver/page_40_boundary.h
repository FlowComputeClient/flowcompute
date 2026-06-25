#ifndef PAGE_40_BOUNDARY_H
#define PAGE_40_BOUNDARY_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWizardPage>

#include "../../core_types.h"

class SolverWizard;

class BoundaryPage : public QWizardPage {
    Q_OBJECT

public:
    explicit BoundaryPage(const QHash<QString, FlowCompute::FieldDef>&,
                          const std::vector<FlowCompute::BoundaryConditionDef>&,
                          QWidget *parent);

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    SolverWizard* solverWizard;
    QHash<QString, FlowCompute::FieldData>* m_cfg;
    QString m_currentField, m_currentPatch;

    // Data from wizard
    QHash<QString, FlowCompute::FieldDef> m_fieldData;
    std::vector<FlowCompute::BoundaryConditionDef> m_boundaryConditions;
    std::vector<FlowCompute::BoundaryConditionDef> m_finalList;
    std::vector<FlowCompute::MeshPatch> m_boundaryPatches;
    QStringList m_solverFields, m_turbFields, m_fieldList;

    // Widgets
    QComboBox *m_fieldCombo, *m_bcTypeCombo, *m_patchTypeCombo;
    QFormLayout *m_bcLayout;
    QLineEdit *m_dimensionEdit, *m_internalFieldEdit;
    QListWidget *m_fieldListWidget, *m_patchListWidget;
    QStackedWidget *m_detailStack;
    QTableWidget *m_boundaryTable;

private slots:
    void onFieldSelected(const QString& text);
    void onPatchSelected(const QString& text);
    void onBcTypeChanged(const QString& text);
    void onPatchTypeChanged(const QString& text);
};

#endif  // PAGE_40_BOUNDARY_H
