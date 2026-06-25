#ifndef PAGE_40_SURFACE_EXTRACTION_H
#define PAGE_40_SURFACE_EXTRACTION_H

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

class MainWindow;
class MeshWizard;

class SurfaceExtractionPage : public QWizardPage {
    Q_OBJECT

public:
    explicit SurfaceExtractionPage(QWidget *parent);
    int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    MeshWizard* meshWizard;
    QTableWidget* featureTable;
};

#endif  // PAGE_40_SURFACE_EXTRACTION_H
