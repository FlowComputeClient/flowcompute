// Copyright 2026 FlowCompute LLC
//
// This file is part of FlowCompute.
//
// FlowCompute is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// FlowCompute is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with FlowCompute. If not, see <https://www.gnu.org/licenses/>.

#include "page_10_intro.h"

#include "wizard_new_case.h"

// Introduction page asks for the case name and platform
IntroPage::IntroPage(bool isWindows, bool isWslAvailable, QWidget *parent):
    QWizardPage(parent) {
    // Set title and style
    setTitle(tr("Case Configuration"));

    // Create a grid layout with two columns
    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(20);

    // Ask for case folder name
    layout->addWidget(new QLabel(tr("Name of new case:")), 0, 0);
    m_caseNameEdit = new QLineEdit(this);
    layout->addWidget(m_caseNameEdit, 0, 1);
    m_caseNameEdit->setText("test");
    registerField("caseName", m_caseNameEdit);

    // Ask for the target system
    QGroupBox* targetGroup = new QGroupBox(tr("Target System"), this);
    layout->addWidget(targetGroup, 1, 0, 1, 2);
    QFont f = targetGroup->font();
    f.setBold(true);
    targetGroup->setFont(f);

    // Create grid layout for group
    QGridLayout* targetGroupLayout = new QGridLayout(targetGroup);
    targetGroup->setLayout(targetGroupLayout);
    targetGroupLayout->setSpacing(20);

    // Create radio button group
    m_targetButtonGroup = new QButtonGroup(this);

    // Create target options
    int row = 0;
    QRadioButton *remoteButton = new QRadioButton(tr("Remote:"), targetGroup);
    m_remoteIPAddrEdit = new QLineEdit(targetGroup);
    if (isWindows) {
        if (isWslAvailable) {
            QRadioButton *localWinButton =
                new QRadioButton(tr("Local (WSL)"), targetGroup);
            targetGroupLayout->addWidget(localWinButton, row++, 0);
            m_targetButtonGroup->addButton(localWinButton,
                static_cast<int>(TargetType::LOCAL_WINDOWS));
        }
        targetGroupLayout->addWidget(remoteButton, row, 0);
        m_targetButtonGroup->addButton(remoteButton,
            static_cast<int>(TargetType::REMOTE_LINUX));
        targetGroupLayout->addWidget(m_remoteIPAddrEdit, row++, 1);

    } else {
        QRadioButton *localLinuxButton =
            new QRadioButton(tr("Local"), targetGroup);
        m_targetButtonGroup->addButton(localLinuxButton,
            static_cast<int>(TargetType::LOCAL_LINUX));
        targetGroupLayout->addWidget(localLinuxButton, row++, 0);
        targetGroupLayout->addWidget(remoteButton, row, 0);
        m_targetButtonGroup->addButton(remoteButton,
            static_cast<int>(TargetType::REMOTE_LINUX));
        targetGroupLayout->addWidget(m_remoteIPAddrEdit, row++, 1);
    }

    // Register target system ID
    connect(m_targetButtonGroup, &QButtonGroup::idClicked, this,
            &IntroPage::targetSystemChanged);
    registerField("targetSystemId", this, "targetSystemId");

    // Set the first button to checked
    QList<QAbstractButton*> buttons = m_targetButtonGroup->buttons();
    buttons.first()->setChecked(true);

    // Disable IP box for local target
    m_remoteIPAddrEdit->setEnabled(remoteButton->isChecked());
    connect(remoteButton, &QRadioButton::toggled,
            m_remoteIPAddrEdit, &QLineEdit::setEnabled);

    // Ask for the case creation method
    QGroupBox* caseCreationGroup =
        new QGroupBox(tr("Case Initialization"), this);
    layout->addWidget(caseCreationGroup, 2, 0, 1, 2);
    f = caseCreationGroup->font();
    f.setBold(true);
    caseCreationGroup->setFont(f);

    // Create radio button group
    m_caseCreationButtonGroup = new QButtonGroup(this);

    // Create layout for group
    QVBoxLayout* caseCreationGroupLayout = new QVBoxLayout(caseCreationGroup);
    caseCreationGroupLayout->setSpacing(20);
    caseCreationGroup->setLayout(caseCreationGroupLayout);

    // Create interactive radio button
    m_interactiveRadio = new QRadioButton(tr("Interactive case builder"),
                                          caseCreationGroup);
    m_caseCreationButtonGroup->addButton(m_interactiveRadio, 0);
    caseCreationGroupLayout->addWidget(m_interactiveRadio);
    m_interactiveRadio->setChecked(true);

    // Create tutorial radio button
    m_tutorialRadio = new QRadioButton(tr("Copy a tutorial case"),
                                       caseCreationGroup);
    m_caseCreationButtonGroup->addButton(m_tutorialRadio, 1);
    caseCreationGroupLayout->addWidget(m_tutorialRadio);

    // Register base case button ID
    connect(m_caseCreationButtonGroup, &QButtonGroup::idClicked,
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
    QAbstractButton *button = m_targetButtonGroup->button(id);
    if (button) {
        button->setChecked(true);
    }
}

// Set base case ID
void IntroPage::setCaseCreationType(int id) {
    QAbstractButton *button = m_caseCreationButtonGroup->button(id);
    if (button) {
        button->setChecked(true);
    }
}
