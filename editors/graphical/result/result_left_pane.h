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

#ifndef EDITORS_GRAPHICAL_RESULT_RESULT_LEFT_PANE_H_
#define EDITORS_GRAPHICAL_RESULT_RESULT_LEFT_PANE_H_

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <vector>

#include "../../../core_types.h"

class ResultLeftPane : public QWidget {
    Q_OBJECT

 public:
    explicit ResultLeftPane(const QStringList& timeFolders,
                            QWidget* parent = nullptr);

 signals:
     void timeChange(const QString& timeFolder);

 private:
    QDoubleSpinBox* m_angleSpin;
    QHash<QString, FlowCompute::FieldData> m_fieldEditorMap;
    QPushButton *m_checkButton, *m_renumberButton,
        *m_patchButton, *m_applyButton;
    QString m_currentField;
    QStringList m_timeFolders;
    QTableWidget *m_patchTable;

    QHash<QString, FlowCompute::FieldDef> m_fieldData;
    std::vector<FlowCompute::BoundaryConditionDef> m_boundaryConditions;
    std::vector<FlowCompute::MeshPatch> m_boundaryPatches;
};

#endif  // EDITORS_GRAPHICAL_RESULT_RESULT_LEFT_PANE_H_
