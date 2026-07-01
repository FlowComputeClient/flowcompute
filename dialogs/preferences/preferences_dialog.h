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

#ifndef PREFERENCES_DIALOG_H_
#define PREFERENCES_DIALOG_H_

#include <QComboBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

class PreferencesDialog : public QDialog {
    Q_OBJECT

 public:
    // Pass the title, the prompt, and the options in the constructor
    PreferencesDialog(QWidget* parent = nullptr);

    QString getTheme() const;

 private:
    QComboBox* m_themeCombo;
};

#endif // PREFERENCES_DIALOG_H_

