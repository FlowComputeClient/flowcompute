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

#ifndef SURFACE_DIALOG_H
#define SURFACE_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>

class AddSurfaceDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddSurfaceDialog(QWidget *parent = nullptr);

    QString getSurfaceName() const { return m_nameEdit->text().trimmed(); }
    int getSurfaceType() const { return m_typeCombo->currentData().toInt(); }

private:
    QLineEdit* m_nameEdit;
    QComboBox* m_typeCombo;

private slots:
    void validateAndAccept();
};

#endif // SURFACE_DIALOG_H
