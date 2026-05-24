#ifndef PAGE_20_PHYSICS_H
#define PAGE_20_PHYSICS_H

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
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWizardPage>

#include "../../core_types.h"
#include "solver_structs.h"

class SolverWizard;

class PhysicsPage : public QWizardPage {
    Q_OBJECT

    // Define properties for tree selection
    Q_PROPERTY(QString turbulenceModel READ getTurbulenceModel
                   WRITE setTurbulenceModel NOTIFY selectionChanged)
    Q_PROPERTY(QString turbulenceCategory READ getTurbulenceCategory
                   WRITE setTurbulenceCategory NOTIFY selectionChanged)
    Q_PROPERTY(QString turbulenceSubCategory READ getTurbulenceSubCategory
                   WRITE setTurbulenceSubCategory NOTIFY selectionChanged)

public:
    explicit PhysicsPage(const FlowCompute::TurbulenceDatabase& turbModels,
                         QWidget *parent);
    // int nextId() const override;

    // Accessors for the properties
    QString getTurbulenceModel() const { return m_selectedModel; }
    void setTurbulenceModel(QString arg) { m_selectedModel = arg; }
    QString getTurbulenceCategory() const { return m_selectedCategory; }
    void setTurbulenceCategory(QString arg) { m_selectedCategory = arg; }
    QString getTurbulenceSubCategory() const { return m_selectedSubCategory; }
    void setTurbulenceSubCategory(QString arg) { m_selectedSubCategory = arg; }

signals:
    void selectionChanged();

protected:
    void initializePage() override;
    // bool validatePage() override;

private:
    SolverWizard* solverWizard;
    PhysicsConfig* m_cfg;
    FlowCompute::TurbulenceDatabase m_turbModels;

    QTreeWidget* turbulenceTree;
    QComboBox *transportModelBox;
    QTableWidget* propertiesTable;
    QStringList standardProperties;

    // Backing variables for the properties
    QString m_selectedModel;
    QString m_selectedCategory;
    QString m_selectedSubCategory;

private slots:
    void modelChanged();
};

#endif  // PAGE_20_PHYSICS_H
