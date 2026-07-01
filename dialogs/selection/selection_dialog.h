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

#ifndef SELECTION_DIALOG_H_
#define SELECTION_DIALOG_H_

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

class SelectionDialog : public QDialog {
    Q_OBJECT

 public:
    // Pass the title, the prompt, and the options in the constructor
    SelectionDialog(const QString& title, const QString& prompt,
                    const QStringList& items, QWidget* parent = nullptr);

    QString getSelectedItem() const;

 private:
    QListWidget* m_listWidget;
};

#endif // SELECTION_DIALOG_H_
