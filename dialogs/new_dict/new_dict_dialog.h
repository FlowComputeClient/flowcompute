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

#ifndef DIALOGS_NEW_DICT_NEW_DICT_DIALOG_H_
#define DIALOGS_NEW_DICT_NEW_DICT_DIALOG_H_

#include <QDialog>

class SystemManager;
class QLineEdit;
class QComboBox;

class NewDictDialog : public QDialog {
 public:
    NewDictDialog(const QString& caseName, const QString& openFoamPath,
        QWidget* parent = nullptr);
    QString getFileName() const { return m_fileName; }
    QString getDictContent() const { return m_dictContent; }

 private:
    QString m_openFoamPath, m_fileName, m_dictContent;
    QLineEdit* m_nameEdit;
    QComboBox* m_classCombo;

 private slots:
    void onOkClicked();
};

#endif // DIALOGS_NEW_DICT_NEW_DICT_DIALOG_H_
