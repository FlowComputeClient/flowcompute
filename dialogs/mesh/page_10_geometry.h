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
    explicit GeometryPage(QWidget *parent);
    QString getCaseName() { return caseName; };
    QStringList getGeometryFiles() { return geometryFiles; };
    int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    MainWindow* mainWin;
    MeshWizard* meshWizard;

    QString caseName;
    QStringList geometryFiles;
    QComboBox* caseBox;
    QListWidget *listWidget;
    int targetSystemId;

private slots:
    void caseChanged(int);
};

#endif  // PAGE_10_GEOMETRY_H
