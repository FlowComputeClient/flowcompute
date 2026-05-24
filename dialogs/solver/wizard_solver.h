#ifndef WIZARD_SOLVER
#define WIZARD_SOLVER

#include <QFile>
#include <QList>
#include <QRegularExpression>
#include <QWizard>

#include "../../parser/open_foam_dictionary.h"
#include "../../core_types.h"
#include "solver_structs.h"
#include "solver_io.h"

#include "page_10_control.h"
#include "page_20_physics.h"
#include "page_30_boundary.h"
#include "page_40_algorithm.h"
#include "page_50_parallel.h"

class MainWindow;

class SolverWizard : public QWizard {
    Q_OBJECT

public:
    explicit SolverWizard(const QString& caseName,
                          const QList<FlowCompute::SolverFamily>& families,
                          const FlowCompute::TurbulenceDatabase& turbModels,
                          const QHash<QString, FlowCompute::FieldData>& fieldData,
                          const QList<FlowCompute::BoundaryCondition>& boundaryConditions,
                          QWidget *parent);

    bool parseFiles();
    ControlConfig& getControlConfig() { return m_controlConfig; };
    PhysicsConfig& getPhysicsConfig() { return m_physicsConfig; };
    MathConfig& getMathConfig() { return m_mathConfig; };
    ParallelConfig& getParallelConfig() { return m_parallelConfig; };

    QStringList getSolverFields();
    QStringList getTurbulenceFields();
    QList<FlowCompute::BoundaryPatch>& getBoundaries() { return m_boundaries; };

    void setConfiguredFields(const QStringList& fields) { m_configuredFields = fields; }
    QStringList getConfiguredFields() const { return m_configuredFields; }

    FlowCompute::Algorithm getSolverAlgorithm();

protected:
    void accept() override;
    bool validateCurrentPage() override;

private:
    MainWindow* mainWin;
    int m_targetSystemId;
    QList<FlowCompute::BoundaryPatch> m_boundaries;

    // Data from config files
    QList<FlowCompute::SolverFamily> m_families;
    FlowCompute::TurbulenceDatabase m_turbModels;
    QHash<QString, FlowCompute::FieldData> m_fieldData;
    QList<FlowCompute::BoundaryCondition> m_boundaryConditions;

    // Lookup maps
    QHash<QString, FlowCompute::Algorithm> m_solverAlgorithmMap;

    // Solver dictionary structures
    bool showParsingErrorMessage(QString fileName);
    QMap<QString, std::shared_ptr<OpenFoamDictionary>> m_dictMap;
    ControlConfig m_controlConfig;
    PhysicsConfig m_physicsConfig;
    MathConfig m_mathConfig;
    ParallelConfig m_parallelConfig;

    QStringList m_configuredFields;
    QString m_caseName;
    QString createSelectionDialog(const QStringList& paths);
};

#endif // WIZARD_SOLVER
