#ifndef PAGE_10_INTRO_H
#define PAGE_10_INTRO_H

#include <QButtonGroup>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWizardPage>

class NewCaseWizard;

class IntroPage : public QWizardPage {
    Q_OBJECT
    Q_PROPERTY(int targetSystemId READ targetSystemId WRITE setTargetSystemId NOTIFY targetSystemChanged)
    Q_PROPERTY(int caseCreationType READ caseCreationType WRITE setCaseCreationType NOTIFY caseCreationTypeChanged)

public:
    explicit IntroPage(QWidget *parent);
    int nextId() const override;
    QString& getOpenFoamPath() { return openFoamPath; };

    // Getter functions
    int targetSystemId() const { return targetButtonGroup->checkedId(); }
    int caseCreationType() const { return caseCreationButtonGroup->checkedId(); }

    // Setter functions
    void setTargetSystemId(int id);
    void setCaseCreationType(int id);

signals:
    void targetSystemChanged();
    void caseCreationTypeChanged();

protected:
    QString createSelectionDialog(const QStringList& paths);

private:
    QString openFoamPath;
    QLineEdit *caseNameEdit, *remoteIPAddrEdit;
    QButtonGroup *targetButtonGroup, *caseCreationButtonGroup;
    QRadioButton *m_tutorialRadio, *m_interactiveRadio;
};

#endif  // PAGE_10_INTRO_H
