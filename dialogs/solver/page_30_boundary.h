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
#include "solver_structs.h"

class SolverWizard;

class BoundaryPage : public QWizardPage {
    Q_OBJECT

public:
    explicit BoundaryPage(const QHash<QString, FlowCompute::FieldData>&,
                          const QList<FlowCompute::BoundaryCondition>&,
                          QWidget *parent);
    // int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    SolverWizard* solverWizard;
    QListWidget* m_fieldsListWidget;
    QString m_currentField;

    // Data from wizard
    QHash<QString, FlowCompute::FieldData> m_fieldData;
    QList<FlowCompute::BoundaryCondition> m_boundaryConditions;
    QList<FlowCompute::BoundaryCondition> m_finalList;
    QList<FlowCompute::BoundaryPatch> m_boundaryPatches;
    QStringList m_solverFields;
    QStringList m_turbFields;

    // Widgets
    QComboBox* m_fieldBox;
    QLineEdit* m_dimensionEdit;
    QLineEdit* m_internalFieldEdit;
    QTableWidget* m_boundaryTable;

    // Store entered results
    struct PatchState {
        QString bcName;
        QHash<QString, QString> parameters; // Maps parameter name (e.g., "value") to typed text
    };

    struct FieldEditor {
        QString dimension;
        QString internalField;
        QHash<QString, PatchState> patchStates; // Stores the user's data
    };
    QHash<QString, FieldEditor> m_fieldEditorMap;

private slots:
    void fieldChanged();
    void bcChanged(int row, const QString& text);
    bool setParams(const QString& bcName, QHash<QString, QString>& params);
};

#endif  // PAGE_30_BOUNDARY_H
