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

#ifndef WIZARDS_POST_PROCESSING_YPLUS_DIALOG_H_
#define WIZARDS_POST_PROCESSING_YPLUS_DIALOG_H_

#include <QDialog>

#include "parser/function_object.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QListWidget;

class YPlusDialog : public QDialog {
    Q_OBJECT
 public:
    YPlusDialog(const QStringList& fields,
        CaseIO::YPlusConfig& yPlusConfig, QWidget* parent = nullptr);
    CaseIO::YPlusConfig getFunctionObject() { return m_yPlusConfig; };

 private:
    QStringList m_fields;
    CaseIO::YPlusConfig& m_yPlusConfig;
    QCheckBox *m_logCheck, *m_locationCheck;
    QComboBox *m_executeCombo, *m_writeCombo, *m_modeCombo;
    QDoubleSpinBox *m_executeSpin, *m_writeSpin;
    QLineEdit *m_nameEdit;
    QListWidget *m_fieldListWidget;

 private slots:
    void onOkClicked();
};

#endif  // WIZARDS_POST_PROCESSING_YPLUS_DIALOG_H_
