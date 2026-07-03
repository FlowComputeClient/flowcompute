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

#ifndef DIALOGS_UTILITY_OUTPUT_UTILITY_OUTPUT_DIALOG_H_
#define DIALOGS_UTILITY_OUTPUT_UTILITY_OUTPUT_DIALOG_H_

#include <QDialog>
#include <QString>
#include <vector>

class QLabel;
class QTextEdit;
class QPushButton;
class QVBoxLayout;

// A clean struct to pass data into the dialog
struct SummaryItem {
    QString text;
    bool isSuccess;
};

class UtilityOutputDialog : public QDialog {
    Q_OBJECT

 public:
    explicit UtilityOutputDialog(const QString& title,
        const QString& description,
        const std::vector<QString>& summaryItems,
        const QString& rawLog, QWidget *parent = nullptr);

 private slots:
    void toggleLogViewer(bool checked);

 private:
    QTextEdit* m_rawLogTextEdit;
    QPushButton* m_toggleButton;
};

#endif  // DIALOGS_UTILITY_OUTPUT_UTILITY_OUTPUT_DIALOG_H_
