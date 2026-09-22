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

#ifndef WIZARDS_SOLVER_PAGE_30_THERMO_H_
#define WIZARDS_SOLVER_PAGE_30_THERMO_H_

#include <QWizardPage>

#include "parser/thermo_physical_properties.h"

class SolverWizard;
class QComboBox;
class QGridLayout;
class QLineEdit;
class QPushButton;

class ThermoPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit ThermoPage(QWidget *parent);

 protected:
    // void initializePage() override;
    bool validatePage() override;

 private:
    SolverWizard* m_solverWizard;
    CaseIO::ThermoConfig* m_cfg;
    CaseIO::MixtureVariant m_mixtureData;

    QComboBox *m_typeCombo, *m_mixtureCombo, *m_transportCombo, *m_thermoCombo,
        *m_eosCombo, *m_specieCombo, *m_energyCombo, *m_chemistryReaderCombo;
    QLineEdit *m_inertSpecieEdit, *m_chemFileEdit, *m_thermoFileEdit;
    QPushButton* m_mixtureButton;
    QWidget *m_inertSpecieContainer, *m_chemistryContainer,
        *m_chemistryFilesContainer;

    void setupField(QGridLayout* layout, int row, const QString& labelText,
        QComboBox*& combo, const QStringList& options);

 private slots:
    void updateExtraFieldsVisibility();
};

#endif  // WIZARDS_SOLVER_PAGE_30_THERMO_H_
