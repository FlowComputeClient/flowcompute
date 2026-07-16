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

#ifndef PAGE_10_INTRO_H_
#define PAGE_10_INTRO_H_

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
    Q_PROPERTY(int targetSystemId READ targetSystemId
                   WRITE setTargetSystemId NOTIFY targetSystemChanged)
    Q_PROPERTY(int caseCreationType READ caseCreationType
                   WRITE setCaseCreationType NOTIFY caseCreationTypeChanged)

 public:
    IntroPage(bool isWindows, bool isWslAvailable, QWidget *parent);
    int nextId() const override;
    QString& getOpenFoamPath() { return m_openFoamPath; };

    // Getter functions
    int targetSystemId() const {
        return m_targetButtonGroup->checkedId(); }
    int caseCreationType() const {
        return m_caseCreationButtonGroup->checkedId(); }

    // Setter functions
    void setTargetSystemId(int id);
    void setCaseCreationType(int id);

 signals:
    void targetSystemChanged();
    void caseCreationTypeChanged();

 protected:
    QString createSelectionDialog(const QStringList& paths);

 private:
    QString m_openFoamPath;
    QLineEdit *m_caseNameEdit, *m_remoteIPAddrEdit;
    QButtonGroup *m_targetButtonGroup, *m_caseCreationButtonGroup;
    QRadioButton *m_tutorialRadio, *m_interactiveRadio;
};

#endif  // PAGE_10_INTRO_H_
