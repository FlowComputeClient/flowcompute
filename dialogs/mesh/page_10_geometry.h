#ifndef PAGE_10_GEOMETRY_H
#define PAGE_10_GEOMETRY_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
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
#include <QVBoxLayout>
#include <QWizardPage>

class MainWindow;
class MeshWizard;

class GeometryPage : public QWizardPage {
    Q_OBJECT

public:
    explicit GeometryPage(const QString& caseName, const QStringList& cases, QWidget *parent);
    QString getCaseName() { return m_caseName; };
    QStringList getGeometryFiles() { return m_geometryFiles; };
    int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    MainWindow* mainWin;
    MeshWizard* meshWizard;

    QCheckBox *m_blockMeshCheck, *m_extractCheck, *m_castellatedCheck, *m_snapCheck, *m_layersCheck;
    QString m_caseName;
    QStringList m_cases, m_geometryFiles;
    QComboBox* m_caseCombo;
    QListWidget *m_geometryList;

private slots:
    void caseChanged(const QString& caseName);
};

#endif  // PAGE_10_GEOMETRY_H
