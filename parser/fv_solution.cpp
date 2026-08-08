#include "fv_solution.h"

#include <QDebug>
#include <QDir>
#include <QMetaEnum>
#include <QRegularExpression>

#include "common.h"

static QStringList extractFieldsFromRegex(QString key) {
    // Clean potential residual quotes
    key = key.remove('"');

    // Translate default
    if (key == ".*") {
        return { "< default >" };
    }

    // If it's a grouped regex like (U|k|omega)
    if (key.startsWith('(') && key.endsWith(')')) {
        key = key.mid(1, key.length() - 2);
        return key.split('|', Qt::SkipEmptyParts);
    }
    return { key };
}

void CaseIO::parseFvSolution(std::shared_ptr<OpenFoamDictionary> dict,
                             MathConfig& cfg) {
    if (!dict) return;

    // ==========================================
    // 1. Parse Linear Solvers & Tolerances
    // ==========================================
    QStringList solverKeys = dict->getDictKeys("solvers");
    for (const QString& rawKey : std::as_const(solverKeys)) {
        QString path = "solvers/" + rawKey;

        QString solverStr = dict->getString(path + "/solver");
        QString precondStr = dict->getString(path + "/preconditioner");
        QString smootherStr = dict->getString(path + "/smoother");
        double tol = dict->getNumber(path + "/tolerance");
        double relTol = dict->getNumber(path + "/relTol");

        // Expand regex groupings into individual fields
        QStringList fields = extractFieldsFromRegex(rawKey);

        for (const QString& fieldName : std::as_const(fields)) {
            bool isFinal = fieldName.endsWith("Final");
            QString baseField =
                isFinal ? fieldName.left(fieldName.length() - 5) : fieldName;

            // Ensure the base field exists in our map
            if (!cfg.fieldMathConfigs.contains(baseField)) {
                cfg.fieldMathConfigs[baseField] = FieldMathConfig();
            }

            auto& fieldConfig = cfg.fieldMathConfigs[baseField];

            // If final, map it to the base field's struct
            if (isFinal) {
                fieldConfig.hasFinalOverride = true;
                fieldConfig.finalAbsTolerance =
                    std::isnan(tol) ? 1e-6 : tol;
                fieldConfig.finalRelTolerance =
                    std::isnan(relTol) ? 0.0 : relTol;
            } else {

                // solver
                if (!solverStr.isEmpty()) {
                    bool ok = false;
                    int enumValue =
                        QMetaEnum::fromType<FieldMathConfig::LinearSolver>().
                            keyToValue(solverStr.toUtf8().constData(), &ok);
                    if (ok) {
                        fieldConfig.solver =
                        static_cast<FieldMathConfig::LinearSolver>(enumValue);
                    }
                }

                // preconditioner
                if (!precondStr.isEmpty()) {
                    bool ok = false;
                    int enumValue =
                        QMetaEnum::fromType<FieldMathConfig::Preconditioner>().
                            keyToValue(precondStr.toUtf8().constData(), &ok);
                    if (ok) {
                        fieldConfig.preconditioner =
                        static_cast<FieldMathConfig::Preconditioner>(enumValue);
                    }
                }

                // smoother
                if (!smootherStr.isEmpty()) {
                    bool ok = false;
                    int enumValue =
                        QMetaEnum::fromType<FieldMathConfig::Smoother>().
                            keyToValue(smootherStr.toUtf8().constData(), &ok);
                    if (ok) {
                        fieldConfig.smoother =
                            static_cast<FieldMathConfig::Smoother>(enumValue);
                    }
                }
                fieldConfig.absTolerance = std::isnan(tol) ? 1e-6 : tol;
                fieldConfig.relTolerance = std::isnan(relTol) ? 0.1 : relTol;
            }
        }
    }

    // ==========================================
    // 2. Parse Relaxation Factors
    // ==========================================
    QStringList relFields = dict->getDictKeys("relaxationFactors/fields");
    for (const QString& rf : std::as_const(relFields)) {
        double val = dict->getNumber("relaxationFactors/fields/" + rf);
        if (!std::isnan(val)) {
            QStringList mappedFields = extractFieldsFromRegex(rf);
            for (const QString& mappedKey : std::as_const(mappedFields)) {
                if (!cfg.fieldMathConfigs.contains(mappedKey)) {
                    cfg.fieldMathConfigs[mappedKey] = FieldMathConfig();
                }
                cfg.fieldMathConfigs[mappedKey].relaxationFactor = val;
                cfg.fieldMathConfigs[mappedKey].isFieldsRelaxation = true;
            }
        }
    }

    // "equations" sub-dictionary (typically U, k, omega)
    QStringList relEqs = dict->getDictKeys("relaxationFactors/equations");
    for (const QString& rq : std::as_const(relEqs)) {
        QStringList expandedFields = extractFieldsFromRegex(rq);
        double val = dict->getNumber("relaxationFactors/equations/" + rq);

        if (!std::isnan(val)) {
            for (const QString& f : std::as_const(expandedFields)) {
                if (!cfg.fieldMathConfigs.contains(f))
                    cfg.fieldMathConfigs[f] = FieldMathConfig();
                cfg.fieldMathConfigs[f].relaxationFactor = val;
                cfg.fieldMathConfigs[f].isFieldsRelaxation = false;
            }
        }
    }

    // ==========================================
    // 3. Parse Algorithm Controls
    // ==========================================
    // Determine which algorithm block is present
    if (!dict->getDictKeys("SIMPLE").isEmpty()) {
        SimpleConfig algo;
        algo.nNonOrthogonalCorrectors =
            dict->getNumber("SIMPLE/nNonOrthogonalCorrectors");
        if(std::isnan(algo.nNonOrthogonalCorrectors))
            algo.nNonOrthogonalCorrectors = 0;

        QString consistentStr = dict->getString("SIMPLE/consistent");
        algo.consistent = (consistentStr == "yes" || consistentStr == "true");

        algo.pRefCell = dict->getNumber("SIMPLE/pRefCell");
        if(std::isnan(algo.pRefCell))
            algo.pRefCell = 0;
        algo.pRefValue = dict->getNumber("SIMPLE/pRefValue");
        if(std::isnan(algo.pRefValue))
            algo.pRefValue = 0.0;

        cfg.algorithmConfig = algo;
    }
    else if (!dict->getDictKeys("PIMPLE").isEmpty()) {
        PimpleConfig algo;
        algo.nOuterCorrectors = dict->getNumber("PIMPLE/nOuterCorrectors");
        if(std::isnan(algo.nOuterCorrectors))
            algo.nOuterCorrectors = 1;

        algo.nCorrectors = dict->getNumber("PIMPLE/nCorrectors");
        if(std::isnan(algo.nCorrectors))
            algo.nCorrectors = 2;

        algo.nNonOrthogonalCorrectors =
            dict->getNumber("PIMPLE/nNonOrthogonalCorrectors");
        if(std::isnan(algo.nNonOrthogonalCorrectors))
            algo.nNonOrthogonalCorrectors = 0;

        QString mp = dict->getString("PIMPLE/momentumPredictor");
        algo.momentumPredictor = (mp == "yes" || mp == "true" || mp.isEmpty());

        algo.pRefCell = dict->getNumber("PIMPLE/pRefCell");
        if(std::isnan(algo.pRefCell))
            algo.pRefCell = 0;
        algo.pRefValue = dict->getNumber("PIMPLE/pRefValue");
        if(std::isnan(algo.pRefValue))
            algo.pRefValue = 0.0;

        cfg.algorithmConfig = algo;
    }
    else if (!dict->getDictKeys("PISO").isEmpty()) {
        PisoConfig algo;
        algo.nCorrectors = dict->getNumber("PISO/nCorrectors");
        if(std::isnan(algo.nCorrectors)) algo.nCorrectors = 2;

        algo.nNonOrthogonalCorrectors =
            dict->getNumber("PISO/nNonOrthogonalCorrectors");
        if(std::isnan(algo.nNonOrthogonalCorrectors))
            algo.nNonOrthogonalCorrectors = 0;

        QString mp = dict->getString("PISO/momentumPredictor");
        algo.momentumPredictor = (mp == "yes" || mp == "true" || mp.isEmpty());

        algo.pRefCell = dict->getNumber("PISO/pRefCell");
        if(std::isnan(algo.pRefCell)) algo.pRefCell = 0;
        algo.pRefValue = dict->getNumber("PISO/pRefValue");
        if(std::isnan(algo.pRefValue)) algo.pRefValue = 0.0;

        cfg.algorithmConfig = algo;
    }
}

QString CaseIO::createFvSolution(const MathConfig& cfg,
                                 const QString& openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << createFoamHeader("fvSolution", openFoamPath);

    // Standard lambda with indentation support
    auto writeEntry = [&out](const QString& keyword,
                             const QString& value, int indentLevel = 0,
                             bool addEmptyLine = false) {
        QString indent(indentLevel * 4, ' ');
        out << indent << keyword.leftJustified(20, ' ') << value << ";\n";
        if (addEmptyLine) out << "\n";
    };

    // OpenFOAM canonical switch format
    auto toFoamSwitch = [](bool val) { return val ? "yes" : "no"; };

    // Solvers sub-dictionary
    out << "// Solver settings for each variable\n";
    out << "solvers\n{\n";
    for (auto it = cfg.fieldMathConfigs.cbegin();
        it != cfg.fieldMathConfigs.cend(); ++it) {
        const QString& fieldName = it.key();
        const FieldMathConfig& fCfg = it.value();

        if (fieldName == "< default >")
            continue;

        // Local lambda to write a solver block
        auto writeSolverBlock =
            [&](const QString& name, double absTol, double relTol) {
            out << "    " << name << "\n    {\n";
            writeEntry("solver", enumToString(fCfg.solver, "GAMG"), 2);

            // Assume enumToString returns "NONE" for the NONE enum value.
            QString smoother = enumToString(fCfg.smoother, "NONE");
            if (smoother != "NONE") writeEntry("smoother", smoother, 2);

            QString precond = enumToString(fCfg.preconditioner, "NONE");
            if (precond != "NONE") writeEntry("preconditioner", precond, 2);

            writeEntry("tolerance", QString::number(absTol, 'g', 6), 2);
            writeEntry("relTol", QString::number(relTol, 'g', 6), 2);
            out << "    }\n\n";
        };

        // Write standard solver parameters
        writeSolverBlock(fieldName, fCfg.absTolerance, fCfg.relTolerance);

        // Write the 'Final' override parameters if explicitly requested
        if (fCfg.hasFinalOverride) {
            writeSolverBlock(fieldName + "Final", fCfg.finalAbsTolerance,
                             fCfg.finalRelTolerance);
        }
    }
    out << "}\n\n";

    // Algorithm Configuration
    std::visit([&](auto&& algoCfg) {
        using T = std::decay_t<decltype(algoCfg)>;

        // Local helper lambda to write the residualControl
        auto writeResidualControls = [&](const auto& config) {
            bool hasEnabled = false;
            for (const auto& rc : config.resControls) {
                if (rc.isEnabled) {
                    hasEnabled = true;
                    break;
                }
            }

            if (hasEnabled) {
                out << "    residualControl\n    {\n";
                for (const auto& rc : config.resControls) {
                    if (rc.isEnabled) {
                        writeEntry(rc.fieldName, rc.tolerance, 2);
                    }
                }
                out << "    }\n";
            }
        };

        // Print algorithm-specific settings
        out << "// Algorithm-specific settings\n";
        if constexpr (std::is_same_v<T, SimpleConfig>) {
            out << "SIMPLE\n{\n";
            writeEntry("nNonOrthogonalCorrectors   ",
                QString::number(algoCfg.nNonOrthogonalCorrectors), 1);
            writeEntry("consistent   ", toFoamSwitch(algoCfg.consistent), 1);
            if (algoCfg.pRefCell >= 0) {
                writeEntry("pRefCell   ",
                    QString::number(algoCfg.pRefCell), 1);
                writeEntry("pRefValue   ",
                    QString::number(algoCfg.pRefValue), 1);
            }

            // Generate the residualControl block for SIMPLE
            writeResidualControls(algoCfg);

            out << "}\n\n";
        }
        else if constexpr (std::is_same_v<T, PisoConfig>) {
            out << "PISO\n{\n";
            writeEntry("momentumPredictor   ",
                toFoamSwitch(algoCfg.momentumPredictor), 1);
            writeEntry("nCorrectors   ",
                QString::number(algoCfg.nCorrectors), 1);
            writeEntry("nNonOrthogonalCorrectors   ",
                QString::number(algoCfg.nNonOrthogonalCorrectors), 1);
            if (algoCfg.pRefCell >= 0) {
                writeEntry("pRefCell", QString::number(algoCfg.pRefCell), 1);
                writeEntry("pRefValue", QString::number(algoCfg.pRefValue), 1);
            }
            out << "}\n\n";
        }
        else if constexpr (std::is_same_v<T, PimpleConfig>) {
            out << "PIMPLE\n{\n";
            writeEntry("momentumPredictor",
                toFoamSwitch(algoCfg.momentumPredictor), 1);
            writeEntry("nOuterCorrectors",
                QString::number(algoCfg.nOuterCorrectors), 1);
            writeEntry("nCorrectors", QString::number(algoCfg.nCorrectors), 1);
            writeEntry("nNonOrthogonalCorrectors",
                QString::number(algoCfg.nNonOrthogonalCorrectors), 1);
            if (algoCfg.pRefCell >= 0) {
                writeEntry("pRefCell", QString::number(algoCfg.pRefCell), 1);
                writeEntry("pRefValue", QString::number(algoCfg.pRefValue), 1);
            }

            // Generate the residualControl block for PIMPLE
            writeResidualControls(algoCfg);

            out << "}\n\n";
        }
    }, cfg.algorithmConfig);

    // ---------------------------------------------------------------------
    // 3. Relaxation Factors
    // ---------------------------------------------------------------------
    out << "// Under-relaxation factors used to improve stability\n";
    out << "relaxationFactors\n{\n";
    QString fieldsStr, eqStr;
    QTextStream fOut(&fieldsStr), eOut(&eqStr);
    int fieldCount = 0, eqCount = 0;

    // Iterate through configs again to separate them into fields vs. equations
    for (auto it = cfg.fieldMathConfigs.cbegin();
         it != cfg.fieldMathConfigs.cend(); ++it) {

        if (it.key() == "< default >") {
            continue;
        }

        QString indent(8, ' ');
        QString formattedEntry = indent + it.key().leftJustified(20, ' ') +
            QString::number(it.value().relaxationFactor) + ";\n";

        if (it.value().isFieldsRelaxation) {
            fOut << formattedEntry;
            fieldCount++;
        } else {
            eOut << formattedEntry;
            eqCount++;
        }
    }

    // Only write sub-dictionaries if there are valid entries
    if (fieldCount > 0) {
        out << "    fields\n    {\n" << fieldsStr << "    }\n";
    }
    if (eqCount > 0) {
        out << "    equations\n    {\n" << eqStr << "    }\n";
    }
    out << "}\n\n";

    // Write closing separator
    out << "// ************************************************************************* //\n";

    return dictStr;
}
