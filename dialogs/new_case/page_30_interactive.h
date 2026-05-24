#ifndef PAGE_30_INTERACTIVE_H
#define PAGE_30_INTERACTIVE_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRadioButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWizardPage>

class NewCaseWizard;

class InteractivePage : public QWizardPage {
    Q_OBJECT

public:
    explicit InteractivePage(QWidget *parent);

protected:
    bool validatePage() override;

private:
    QButtonGroup *m_flowButtonGroup, *m_timeButtonGroup, *m_turbulenceButtonGroup;
    QCheckBox *m_heatCheck, *m_radiationCheck, *m_combustionCheck;
    QSlider *m_prioritySlider;
};

#endif  // PAGE_30_INTERACTIVE_H
