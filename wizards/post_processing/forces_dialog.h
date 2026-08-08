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

#ifndef WIZARDS_POST_PROCESSING_FORCES_DIALOG_H_
#define WIZARDS_POST_PROCESSING_FORCES_DIALOG_H_

#include <QDialog>

#include "parser/function_object.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QListWidget;

class ForcesDialog : public QDialog {
    Q_OBJECT
 public:
    ForcesDialog(const QStringList& patches,
        CaseIO::ForcesConfig& forcesConfig, QWidget* parent = nullptr);
    CaseIO::ForcesConfig getFunctionObject() { return m_forcesConfig; };

 private:
    QStringList m_patches;
    CaseIO::ForcesConfig& m_forcesConfig;
    QCheckBox *m_porosityCheck, *m_writeFieldsCheck, *m_logCheck;
    QComboBox *m_flowTypeCombo, *m_executeCombo, *m_writeCombo;
    QDoubleSpinBox *m_rhoInfSpin, *m_pRefSpin, *m_executeSpin, *m_writeSpin;
    std::array<QDoubleSpinBox*, 3> m_cofrSpin;
    QLineEdit *m_nameEdit, *m_pNameEdit, *m_rhoEdit, *m_UNameEdit;
    QListWidget *m_patchesListWidget;

 private slots:
    void onOkClicked();
};

#endif  // WIZARDS_POST_PROCESSING_FORCES_DIALOG_H_
