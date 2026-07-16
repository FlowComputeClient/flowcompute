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

#ifndef WIZARDS_NEW_CASE_WIZARD_NEW_CASE_H_
#define WIZARDS_NEW_CASE_WIZARD_NEW_CASE_H_

#include <QFile>
#include <QWizard>

#include "page_10_intro.h"
#include "page_20_tutorial.h"
#include "page_30_interactive.h"
#include "page_40_project.h"

#include "systems/system_manager.h"

enum class WizardPage {
    Page_Intro = 0,
    Page_Tutorial = 1,
    Page_Interactive = 2,
    Page_Project = 3
};

enum class CaseCreationType {
    INTERACTIVE = 0,
    TUTORIAL
};

enum class FlowConfig {
    Incompressible = 0,
    Compressible,
    Multiphase
};

enum class TurbulenceConfig {
    Laminar = 0,
    RAS,
    LES
};

enum class TimeConfig {
    SteadyState,
    Transient
};

struct CaseConfig {
    FlowConfig flowConfig;
    TurbulenceConfig turbulenceConfig;
    TimeConfig timeConfig;
    int priorityConfig;
    bool heatConfig;
    bool radiationConfig;
    bool combustionConfig;
};

class NewCaseWizard : public QWizard {
    Q_OBJECT

 public:
    NewCaseWizard(bool m_isWindows, bool m_isWslAvailable,
                  SystemManager& systemMgr, QWidget *parent);
    QStringList processPaths(QString path);
    QStringList getTutorials();
    QStringList findOpenFoam();
    CaseConfig& getCaseConfig() { return m_caseConfig; };

 signals:
    void requestCaseCreation(QString caseName, QString casePath,
        QStringList caseFiles, int systemId, QString openFoamPath);

 protected:
    void accept() override;
    bool validateCurrentPage() override;

 private:
    const SystemManager& m_systemMgr;
    TargetType m_targetId;
    std::shared_ptr<TargetSystem> m_system;
    QString m_openFoamPath, m_caseName;
    CaseConfig m_caseConfig;
    QString m_geometryFile;

    QString createSelectionDialog(const QStringList& paths);

    // Create template case
    bool createCase(QString newCasePath);
    void createCaseFiles(const QString&, const QString&, const QString&);
};

#endif  // WIZARDS_NEW_CASE_WIZARD_NEW_CASE_H_
