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

#include <QWizard>

#include "systems/system_manager.h"

enum class NewCasePage {
    Page_Intro = 0,
    Page_Remote = 1,
    Page_Tutorial = 2,
    Page_Interactive = 3,
    Page_Project = 4
};

enum class CaseCreationType {
    INTERACTIVE = 0,
    TUTORIAL
};

enum class FlowConfig {
    Incompressible = 0,
    Compressible
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

enum class PhaseConfig {
    SinglePhase,
    MultiPhase
};

enum class HeatConfig {
    NoHeat,
    FluidHeat,
    ConjugateHeat
};

enum class MeshConfig {
    Static,
    DynamicMRF,
    DynamicAMI,
    DynamicOverset,
    Deformable
};

struct CaseConfig {
    FlowConfig flowConfig;
    TurbulenceConfig turbulenceConfig;
    TimeConfig timeConfig;
    PhaseConfig phaseConfig;
    HeatConfig heatConfig;
    MeshConfig meshConfig;
    int priorityConfig;
    bool isOpenCFD;
    bool radiationConfig;
    bool combustionConfig;
    bool buoyancyConfig;
    bool particlesConfig;
};

class NewCaseWizard : public QWizard {
    Q_OBJECT

 public:
    NewCaseWizard(SystemManager& systemMgr, QWidget *parent);
    QStringList processPaths(const QString& path);
    QStringList getTutorials();
    QStringList findOpenFoam();
    CaseConfig& getCaseConfig() { return m_cfg; };

 signals:
    void requestCaseCreation(QString caseName, QString casePath,
        QStringList caseFiles, int systemId, QString openFoamPath,
        CaseFlags flag, CaseType type, QString userName, QString hostName,
        int port);

 protected:
    void accept() override;
    bool validateCurrentPage() override;

 private:
    SystemManager& m_systemMgr;
    TargetType m_targetId;
    std::shared_ptr<TargetSystem> m_system;
    CaseConfig m_cfg;
    QString m_geometryFile, m_openFoamPath, m_openFoamVersion, m_caseName;
    bool m_isOpenCFD;

    bool checkOpenFoam();

    // Create template case
    bool createCase(const QString& newCasePath);
    void createCaseFiles(const QString& newCasePath,
        const QString& versionText, const QString& websiteText);

};

#endif  // WIZARDS_NEW_CASE_WIZARD_NEW_CASE_H_
