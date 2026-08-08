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

#include "page_20_time_region.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMetaEnum>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QVBoxLayout>

// Configure post-processing tasks
TimeRegionPage::TimeRegionPage(QWidget *parent): QWizardPage(parent) {
    // Set title
    setTitle(tr("Time Control Configuration"));

    // Set layout (automatically applied because 'this' is passed)
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    // Timing/output group
    QGroupBox* timeGroup = new QGroupBox(tr("Time Control"), this);
    QVBoxLayout* timeLayout = new QVBoxLayout(timeGroup);
    timeLayout->setSpacing(15);
    mainLayout->addWidget(timeGroup);

    // Create group of radio buttons
    m_timeButtonGroup = new QButtonGroup(this);

    // Create radio button for all times
    QRadioButton* allTimesButton = new QRadioButton(tr("All Times"), this);
    allTimesButton->setChecked(true);
    m_timeButtonGroup->addButton(allTimesButton, 0);
    timeLayout->addWidget(allTimesButton);

    // Create radio button for latest time
    QRadioButton* latestTimeButton = new QRadioButton(tr("Latest Time"), this);
    m_timeButtonGroup->addButton(latestTimeButton, 1);
    timeLayout->addWidget(latestTimeButton);

    // Create radio button for specific time
    QHBoxLayout* specificTimeLayout = new QHBoxLayout();
    QRadioButton* specificTimeButton =
        new QRadioButton(tr("Specific Time Ranges (10:50, 100):"), this);
    m_timeButtonGroup->addButton(specificTimeButton, 2);
    specificTimeLayout->addWidget(specificTimeButton);

    // Create edit box for time ranges (Fixed variable shadowing)
    m_specificTimeEdit = new QLineEdit(this);
    m_specificTimeEdit->setEnabled(false);
    specificTimeLayout->addWidget(m_specificTimeEdit);
    timeLayout->addLayout(specificTimeLayout);

    // Enable/disable the edit box
    connect(specificTimeButton, &QRadioButton::toggled, m_specificTimeEdit,
            &QLineEdit::setEnabled);

    // noZero check box
    m_noZeroCheck = new QCheckBox(tr("Ignore '0' directory (-noZero)"), this);
    m_noZeroCheck->setChecked(true);
    mainLayout->addWidget(m_noZeroCheck);

    // Constant check box
    m_constantCheck =
        new QCheckBox(tr("Include 'constant' directory (-constant)"), this);
    m_constantCheck->setChecked(false);
    mainLayout->addWidget(m_constantCheck);

    // Register widgets
    registerField("time_allTimes", allTimesButton);
    registerField("time_latestTime", latestTimeButton);
    registerField("time_specificTime", specificTimeButton);
    registerField("time_ranges", m_specificTimeEdit);
    registerField("time_noZero", m_noZeroCheck);
    registerField("time_constant", m_constantCheck);
}