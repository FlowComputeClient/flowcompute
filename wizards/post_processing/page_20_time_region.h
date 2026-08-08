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

#ifndef WIZARDS_POST_PROCESSING_PAGE_20_TIME_REGION_H_
#define WIZARDS_POST_PROCESSING_PAGE_20_TIME_REGION_H_

#include <QWizardPage>

class QButtonGroup;
class QCheckBox;
class QLineEdit;

class TimeRegionPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit TimeRegionPage(QWidget *parent);

 protected:
    // void initializePage() override;
    // bool validatePage() override;

 private:
    QCheckBox *m_noZeroCheck, *m_constantCheck;
    QLineEdit* m_specificTimeEdit;
    QButtonGroup* m_timeButtonGroup;

 private slots:
};

#endif  // WIZARDS_POST_PROCESSING_PAGE_20_TIME_REGION_H_
