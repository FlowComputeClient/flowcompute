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

#ifndef WIZARD_NEW_CASE_H
#define WIZARD_NEW_CASE_H

#include <QFile>
#include <QWizard>

#include "page_10_intro.h"
#include "page_20_tutorial.h"
#include "page_30_interactive.h"
#include "page_40_project.h"

#include "template_strings.h"

class MainWindow;

enum class WizardPage {
    Page_Intro = 0,
    Page_Tutorial = 1,
    Page_Interactive = 2,
    Page_Project = 3
};

enum TargetType {
    LOCAL_WINDOWS = 0,
    LOCAL_LINUX = 1,
    REMOTE_LINUX = 2,
    COUNT
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
    explicit NewCaseWizard(QWidget *parent);
    void setTargetSystemId(int id);
    QStringList getHomeFolders();
    QStringList getTutorials();
    QStringList findOpenFoam();
    CaseConfig& getCaseConfig() { return m_caseConfig; };

protected:
    void accept() override;
    bool validateCurrentPage() override;

private:
    MainWindow* mainWin;
    TargetType m_targetId;
    QString m_openFoamPath, m_caseName;
    CaseConfig m_caseConfig;
    QString m_geometryFile;

    QString createSelectionDialog(const QStringList& paths);

    // Create template case
    bool createCase(QString newCasePath);
    void createCaseFiles(const QString&, const QString&, const QString&);
};

#endif // WIZARD_NEW_CASE_H
