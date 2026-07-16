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

#ifndef DIALOGS_RUN_MESH_RUN_MESH_DIALOG_H_
#define DIALOGS_RUN_MESH_RUN_MESH_DIALOG_H_

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QWidget>
#include <QDialogButtonBox>

class SystemManager;

class RunMeshDialog : public QDialog {
    Q_OBJECT
 public:
    RunMeshDialog(const QString& caseName, const SystemManager& systemMgr,
        const QStringList& files, bool isFoundation, QWidget* parent = nullptr);

 signals:
    void requestRunMesh(const QString& caseName, bool runBlockMesh,
        bool runSurfaceFeature, bool runSnappyHexMesh,
        const QString& snappyCmd, int numCores);

 private:
    const SystemManager& m_systemMgr;
    QStringList m_files;
    bool m_isFoundation;
    QCheckBox *m_runBlockMeshCheck, *m_runSurfaceFeatureCheck,
        *m_runSnappyHexMeshCheck;
    QCheckBox *m_meshOverwriteCheck, *m_meshReconstructCheck;
    QComboBox *m_meshModeCombo, *m_caseCombo, *m_numCoresCombo;

 private slots:
    void onOkClicked();
    void snappyCheckToggled(bool state);
    void snappyHexMeshModeChanged(int index);
    void onCaseChanged(QString caseName);
};

#endif  // DIALOGS_RUN_MESH_RUN_MESH_DIALOG_H_
