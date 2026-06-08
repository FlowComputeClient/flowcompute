#ifndef PAGE_30_BOUNDARY_H
#define PAGE_30_BOUNDARY_H

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
    // int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    SolverWizard* solverWizard;
    QListWidget* m_fieldsListWidget;
    QString m_currentField;
    QHash<QString, FlowCompute::FieldData> m_fieldEditorMap;

    // Data from wizard
    QHash<QString, FlowCompute::FieldDef> m_fieldData;
    std::vector<FlowCompute::BoundaryConditionDef> m_boundaryConditions;
    std::vector<FlowCompute::BoundaryConditionDef> m_finalList;
    std::vector<FlowCompute::MeshPatch> m_boundaryPatches;
    QStringList m_solverFields;
    QStringList m_turbFields;

    // Widgets
    QComboBox* m_fieldBox;
    QLineEdit* m_dimensionEdit;
    QLineEdit* m_internalFieldEdit;
    QTableWidget* m_boundaryTable;

private slots:
    void fieldChanged();
    void bcChanged(int row, const QString& text);
    bool setParams(const QString& bcName, std::unordered_map<QString, QString>& params);
};

#endif  // PAGE_30_BOUNDARY_H
