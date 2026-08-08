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

#ifndef WIZARDS_POST_PROCESSING_PROBES_DIALOG_H_
#define WIZARDS_POST_PROCESSING_PROBES_DIALOG_H_

#include <QDialog>

#include "parser/function_object.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QListWidget;
class QTableWidget;

class ProbesDialog : public QDialog {
    Q_OBJECT
 public:
    ProbesDialog(const QStringList& fields,
        CaseIO::ProbesConfig& ProbesConfig, QWidget* parent = nullptr);
    CaseIO::ProbesConfig getFunctionObject() { return m_probesConfig; };

 private:
    QStringList m_fields;
    CaseIO::ProbesConfig& m_probesConfig;
    QCheckBox *m_logCheck, *m_fixedLocCheck, *m_outBoundsCheck, *m_verboseCheck;
    QCheckBox *m_sampleCheck;
    QComboBox *m_executeCombo, *m_writeCombo, *m_interpolationCombo;
    QDoubleSpinBox *m_executeSpin, *m_writeSpin;
    QLineEdit *m_nameEdit;
    QListWidget *m_fieldListWidget;
    QTableWidget *m_probesTable;

 private slots:
    void onOkClicked();
};

#endif  // WIZARDS_POST_PROCESSING_PROBES_DIALOG_H_
