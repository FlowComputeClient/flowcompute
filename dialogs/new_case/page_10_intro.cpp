#include "page_10_intro.h"

#include "../../main_window.h"
#include "wizard_new_case.h"

// Introduction page asks for the case name and platform
IntroPage::IntroPage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Case Configuration"));

    // Create a grid layout with two columns
    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(20);

    // Ask for case folder name
    layout->addWidget(new QLabel(tr("Name of new case:")), 0, 0);
    caseNameEdit = new QLineEdit(this);
    layout->addWidget(caseNameEdit, 0, 1);
    caseNameEdit->setText("test");
    registerField("caseName", caseNameEdit);

    // Ask for the target system
    QGroupBox* targetGroup = new QGroupBox(tr("Target System"), this);
    layout->addWidget(targetGroup, 1, 0, 1, 2);
    QFont f = targetGroup->font();
    f.setBold(true);
    targetGroup->setFont(f);

    // Create grid layout for group
    QGridLayout* targetGroupLayout = new QGridLayout(targetGroup);
    targetGroup->setLayout(targetGroupLayout);

    // Create radio button group
    targetButtonGroup = new QButtonGroup(this);

    // Create target options
    int row = 0;
    QRadioButton *remoteButton = new QRadioButton(tr("Remote:"), targetGroup);
    remoteIPAddrEdit = new QLineEdit(targetGroup);
    if (MainWindow::isWindows()) {
        if (MainWindow::isWslAvailable()) {
            QRadioButton *localWinButton = new QRadioButton(tr("Local (WSL)"), targetGroup);
            targetGroupLayout->addWidget(localWinButton, row++, 0);
            targetButtonGroup->addButton(localWinButton, static_cast<int>(TargetType::LOCAL_WINDOWS));
        }
        targetGroupLayout->addWidget(remoteButton, row, 0);
        targetButtonGroup->addButton(remoteButton, static_cast<int>(TargetType::REMOTE_LINUX));
        targetGroupLayout->addWidget(remoteIPAddrEdit, row++, 1);

    } else {
        QRadioButton *localLinuxButton = new QRadioButton(tr("Local"), targetGroup);
        targetButtonGroup->addButton(localLinuxButton, static_cast<int>(TargetType::LOCAL_LINUX));
        targetGroupLayout->addWidget(localLinuxButton, row++, 0);
        targetGroupLayout->addWidget(remoteButton, row, 0);
        targetButtonGroup->addButton(remoteButton, static_cast<int>(TargetType::REMOTE_LINUX));
        targetGroupLayout->addWidget(remoteIPAddrEdit, row++, 1);
    }

    // Register target system ID
    connect(targetButtonGroup, &QButtonGroup::idClicked, this, &IntroPage::targetSystemChanged);
    registerField("targetSystemId", this, "targetSystemId");

    // Set the first button to checked
    QList<QAbstractButton*> buttons = targetButtonGroup->buttons();
    buttons.first()->setChecked(true);

    // Disable IP box for local target
    remoteIPAddrEdit->setEnabled(remoteButton->isChecked());
    connect(remoteButton, &QRadioButton::toggled, remoteIPAddrEdit, &QLineEdit::setEnabled);

    // Ask for the case creation method
    QGroupBox* caseCreationGroup = new QGroupBox(tr("Case Creation"), this);
    layout->addWidget(caseCreationGroup, 2, 0, 1, 2);
    f = caseCreationGroup->font();
    f.setBold(true);
    caseCreationGroup->setFont(f);

    // Create radio button group
    caseCreationButtonGroup = new QButtonGroup(this);

    // Create layout for group
    QVBoxLayout* caseCreationGroupLayout = new QVBoxLayout(caseCreationGroup);
    caseCreationGroup->setLayout(caseCreationGroupLayout);

    // Create interactive radio button
    m_interactiveRadio = new QRadioButton(tr("Interactive case builder"), caseCreationGroup);
    caseCreationButtonGroup->addButton(m_interactiveRadio, 0);
    caseCreationGroupLayout->addWidget(m_interactiveRadio);
    m_interactiveRadio->setChecked(true);

    // Create tutorial radio button
    m_tutorialRadio = new QRadioButton(tr("Copy a tutorial case"), caseCreationGroup);
    caseCreationButtonGroup->addButton(m_tutorialRadio, 1);
    caseCreationGroupLayout->addWidget(m_tutorialRadio);

    // Register base case button ID
    connect(caseCreationButtonGroup, &QButtonGroup::idClicked,
            this, &IntroPage::setCaseCreationType);
    registerField("caseCreationType", this, "caseCreationType");

    // Set the page layout
    setLayout(layout);
}

int IntroPage::nextId() const {

    // Set next page according to radio button selection
    if (m_tutorialRadio->isChecked()) {
        return static_cast<int>(WizardPage::Page_Tutorial);
    }
    else if (m_interactiveRadio->isChecked()) {
        return static_cast<int>(WizardPage::Page_Interactive);
    }

    return -1;
}

// Set target system ID
void IntroPage::setTargetSystemId(int id) {
    QAbstractButton *button = targetButtonGroup->button(id);
    if (button) {
        button->setChecked(true);
    }
}

// Set base case ID
void IntroPage::setCaseCreationType(int id) {
    QAbstractButton *button = caseCreationButtonGroup->button(id);
    if (button) {
        button->setChecked(true);
    }
}