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

#ifndef PAGE_20_TUTORIAL_H_
#define PAGE_20_TUTORIAL_H_

#include <QButtonGroup>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QStringList>
#include <QTreeWidget>
#include <QVariant>
#include <QVBoxLayout>
#include <QWizardPage>

class NewCaseWizard;

class TutorialPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit TutorialPage(QWidget*);
    int nextId() const override;

 protected:
    void initializePage() override;

 private:
    QLineEdit *m_hiddenPathEdit;
    QTreeWidget* m_tutorialTree;

    void onTreeSelectionChanged();
    void populateTutorialTree(QTreeWidget* treeWidget,
                              const QStringList& paths);
};

#endif  // PAGE_20_TUTORIAL_H_
