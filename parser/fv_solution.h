#ifndef PARSER_FV_SOLUTION_H_
#define PARSER_FV_SOLUTION_H_

#include <memory>

#include <QMap>
#include <QObject>

#include "open_foam_dictionary.h"

namespace CaseIO {

struct ResidualControl {
    bool isEnabled;
    QString fieldName;
    QString tolerance;
};

struct SimpleConfig {
    unsigned int nNonOrthogonalCorrectors = 2;
    bool consistent = false;
    unsigned int pRefCell = 0;
    double pRefValue = 0.0;
    std::vector<ResidualControl> resControls;
};

struct PisoConfig {
    bool momentumPredictor = true;
    unsigned int nCorrectors = 2;
    unsigned int nNonOrthogonalCorrectors = 2;
    unsigned int pRefCell = 0;
    double pRefValue = 0.0;
};

struct PimpleConfig {
    bool momentumPredictor = true;
    unsigned int nOuterCorrectors = 1;
    unsigned int nCorrectors = 2;
    unsigned int nNonOrthogonalCorrectors = 2;
    unsigned int pRefCell = 0;
    double pRefValue = 0.0;
    std::vector<ResidualControl> resControls;
};

struct FieldMathConfig {
    Q_GADGET

 public:
    enum class LinearSolver {
        GAMG = 0,
        smoothSolver,
        PBiCGStab,
        PCG,
        PBiCG,
        diagonal
    };
    Q_ENUM(LinearSolver)

    enum class Smoother {
        symGaussSeidel = 0,
        GaussSeidel,
        DICGaussSeidel,
        DILUGaussSeidel,
        Jacobi,
        NONE
    };
    Q_ENUM(Smoother)

    enum class Preconditioner {
        DILU = 0,
        DIC,
        GAMG,
        FDIC,
        ILU,
        NONE
    };
    Q_ENUM(Preconditioner)

    LinearSolver solver = LinearSolver::GAMG;
    Smoother smoother = Smoother::NONE;
    Preconditioner preconditioner = Preconditioner::NONE;

    // Standard tolerances
    double absTolerance = 1e-6;
    double relTolerance = 0.1;

    // Final iteration overrides
    bool hasFinalOverride = false;
    double finalAbsTolerance = 1e-6;
    double finalRelTolerance = 0.0;
    double relaxationFactor = 0.5;
    bool isFieldsRelaxation = false;
};

struct MathConfig {
    QMap<QString, FieldMathConfig> fieldMathConfigs;
    std::variant<std::monostate, SimpleConfig, PimpleConfig, PisoConfig>
        algorithmConfig;
};

// Parse fvSolution
void parseFvSolution(std::shared_ptr<OpenFoamDictionary> dict,
                     MathConfig& config);

// Create new fvSolution
QString createFvSolution(const MathConfig& cfg, const QString& openFoamPath);
};

#endif  // PARSER_FV_SOLUTION_H_
