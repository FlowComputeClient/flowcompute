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

#include "solver_io.h"

#include <QMetaEnum>

#include "../../utils.h"

template<typename T>
QString enumToString(T value, const QString& fallback) {
    QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    const char* keyString = metaEnum.valueToKey(static_cast<int>(value));
    if (keyString) {
        return QString::fromLatin1(keyString);
    }
    return fallback;
}

ControlConfig SolverIO::parseControlConfig(std::shared_ptr<OpenFoamDictionary> dict) {
    ControlConfig cfg;
    if (!dict) { return cfg; }

    // application
    cfg.solver = dict->getString("application");

    // startFrom
    QString startFromStr = dict->getString("startFrom");
    if (!startFromStr.isEmpty()) {
        bool ok = false;
        int enumValue = QMetaEnum::fromType<Solver::StartSolverType>().keyToValue(
            startFromStr.toUtf8().constData(), &ok);
        if (ok) {
            cfg.startFrom = static_cast<Solver::StartSolverType>(enumValue);
        }
    }

    // startTime
    double sTime = dict->getNumber("startTime");
    if (!std::isnan(sTime)) {
        cfg.startTime = sTime;
    }

    // stopAt
    QString stopAtStr = dict->getString("stopAt");
    if (!stopAtStr.isEmpty()) {
        bool ok = false;
        int enumValue = QMetaEnum::fromType<Solver::EndSolverType>().keyToValue(
            stopAtStr.toUtf8().constData(), &ok);
        if (ok) {
            cfg.stopAt = static_cast<Solver::EndSolverType>(enumValue);
        }
    }

    // endTime and deltaT
    double eTime = dict->getNumber("endTime");
    if (!std::isnan(eTime)) {
        cfg.endTime = eTime;
    }
    double dT = dict->getNumber("deltaT");
    if (!std::isnan(dT)) {
        cfg.deltaT = dT;
    }

    /*
    // Time step adjustment
    QString adjustStr = dict->getString("adjustTimeStep").toLower();
    config.adjustTimeStep = (adjustStr == "yes" || adjustStr == "true" || adjustStr == "on");

    double maxCourant = dict->getNumber("maxCo");
    if (!std::isnan(maxCourant)) {
        config.maxCo = maxCourant;
    }
    */

    // writeControl
    QString writeControlStr = dict->getString("writeControl");
    if (!writeControlStr.isEmpty()) {
        bool ok = false;
        int enumValue = QMetaEnum::fromType<Solver::WriteControlType>().keyToValue(
            writeControlStr.toUtf8().constData(), &ok);
        if (ok) {
            cfg.writeControl = static_cast<Solver::WriteControlType>(enumValue);
        }
    }

    // purgeWrite
    double purgeW = dict->getNumber("purgeWrite");
    if (!std::isnan(purgeW)) {
        cfg.purgeWrite = static_cast<int>(purgeW);
    }

    // writeCompression
    QString writeCompressionStr = dict->getString("writeCompression").toLower();
    if (!writeCompressionStr.isEmpty()) {
        cfg.writeCompression = (writeCompressionStr == "yes"
                                   || writeCompressionStr == "true"
                                   || writeCompressionStr == "on");
    }

    // runTimeModifiable
    QString runTimeModifiableStr = dict->getString("runTimeModifiable").toLower();
    if (!runTimeModifiableStr.isEmpty()) {
        cfg.runTimeModifiable = (runTimeModifiableStr == "yes"
                                   || runTimeModifiableStr == "true"
                                   || runTimeModifiableStr == "on");
    }

    // writeFormat
    QString writeFormatStr = dict->getString("writeFormat");
    cfg.writeFormat =
        Solver::WriteFormatType(QMetaEnum::fromType<Solver::WriteFormatType>().keyToValue(
            writeFormatStr.toUtf8().constData()));

    // writeInterval
    double writeIntervalNum = dict->getNumber("writeInterval");
    if (!std::isnan(writeIntervalNum)) {
        cfg.writeInterval = writeIntervalNum;
    }
    return cfg;
}

PhysicsConfig SolverIO::parseTurbulenceProperties(std::shared_ptr<OpenFoamDictionary> dict) {
    PhysicsConfig cfg;
    if (!dict) { return cfg; }

    // Simulation Type
    QString simType = dict->getString("simulationType");
    if (!simType.isEmpty()) {
        cfg.simulationType = simType;
    }

    // Parsing depends based on simulationType
    if (cfg.simulationType == "RAS") {
        QString parsedModel = dict->getString("RAS/RASModel");
        if (!parsedModel.isEmpty()) cfg.model = parsedModel;

    } else if (cfg.simulationType == "LES") {
        QString parsedModel = dict->getString("LES/LESModel");
        if (!parsedModel.isEmpty()) cfg.model = parsedModel;

        // Extract delta specifically for LES
        QString deltaStr = dict->getString("LES/delta");
        if (!deltaStr.isEmpty()) {
            bool ok = false;
            int enumValue = QMetaEnum::fromType<Solver::DeltaModel>().keyToValue(
                deltaStr.toUtf8().constData(), &ok);
            if (ok) {
                cfg.deltaModel = static_cast<Solver::DeltaModel>(enumValue);
            }
        }
    } else if (cfg.simulationType == "laminar") {
        cfg.model = "laminar";
        return cfg;
    }

    // useTurbulence
    QString turbulenceStr = dict->getString("turbulence").toLower();
    if (!turbulenceStr.isEmpty()) {
        cfg.useTurbulence = (turbulenceStr == "yes" || turbulenceStr == "true"
                            || turbulenceStr == "on");
    }

    return cfg;
}

void SolverIO::parseTransportProperties(std::shared_ptr<OpenFoamDictionary> dict,
                                        PhysicsConfig& cfg) {
    if (!dict) { return; }

    // transportModel
    QString transportModelStr = dict->getString("transportModel");
    if (!transportModelStr.isEmpty()) {
        bool ok = false;
        int enumValue = QMetaEnum::fromType<Solver::TransportModel>().keyToValue(
            transportModelStr.toUtf8().constData(), &ok);
        if (ok) {
            cfg.transportModel = static_cast<Solver::TransportModel>(enumValue);
        }
    }

    // Standard properties
    QStringList standardProperties = {
        "nu", "rho", "Pr", "Prt", "TRef", "Cp", "Cv", "k", "n", "alpha"
    };

    // Extract properties into the dynamic map
    for (const QString& propName : standardProperties) {
        QString propValue = dict->getString(propName);
        if (!propValue.isEmpty()) {
            cfg.fluidProperties.insert(propName, propValue);
        }
    }
}

std::vector<FlowCompute::MeshPatch> SolverIO::parseBoundaryPatches(const QByteArray& fileData) {
    std::vector<FlowCompute::MeshPatch> patches;

    // Convert the raw byte array from the WSL socket into a QString.
    QString text = QString::fromUtf8(fileData);

    // Remove comments
    text.replace(QRegularExpression("/\\*.*?\\*/", QRegularExpression::DotMatchesEverythingOption), "");
    text.replace(QRegularExpression("//.*"), "");

    // Look through top-level parentheses
    int startIdx = text.indexOf('(');
    int endIdx = text.lastIndexOf(')');
    if (startIdx == -1 || endIdx == -1 || startIdx >= endIdx) {
        return patches;
    }

    QString listContent = text.mid(startIdx + 1, endIdx - startIdx - 1);

    // Step 1: Regex to capture the patch name and everything inside its { } block
    QRegularExpression reBlock("([A-Za-z0-9_\\-]+)\\s*\\{([^}]*)\\}");

    // Step 2: Regex to capture the patch type inside the block
    QRegularExpression reType("type\\s+([A-Za-z0-9_\\-]+)\\s*;");

    // Step 3: Regex to capture the number of faces
    QRegularExpression reFaces("nFaces\\s+([0-9]+)\\s*;");

    QRegularExpressionMatchIterator i = reBlock.globalMatch(listContent);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString blockContent = match.captured(2); // The text inside the { }

        // Extract nFaces and check if it's greater than 0
        QRegularExpressionMatch faceMatch = reFaces.match(blockContent);
        int nFaces = faceMatch.hasMatch() ? faceMatch.captured(1).toInt() : 0;

        if (nFaces > 0) {
            FlowCompute::MeshPatch bp;
            bp.name = match.captured(1);

            QRegularExpressionMatch typeMatch = reType.match(blockContent);
            bp.type = (typeMatch.hasMatch()) ? typeMatch.captured(1) : "patch";

            patches.push_back(bp);
        }
    }
    return patches;
}

// Update boundary file
QString SolverIO::updateBoundaryFile(std::shared_ptr<OpenFoamDictionary> dict,
                           const std::vector<FlowCompute::MeshPatch>& filtered) {

    if (!dict) { return QString(); }
    for (const auto& patch : filtered) {
        QString basePath = patch.name;
        if (patch.typeChanged) {
            QString typePath = basePath + "/type";
            dict->setValue(typePath, patch.type);
        }
        if (patch.nameChanged && !patch.newName.isEmpty()) {
            dict->renameKey(basePath, patch.newName);
        }
    }
    return QString::fromUtf8(dict->getRawText());
}

SolverIO::BoundaryFileParts SolverIO::splitBoundaryFile(const QByteArray& rawData) {
    SolverIO::BoundaryFileParts parts;

    // Find the opening parenthesis of the list and the closing parenthesis
    int openParen = rawData.indexOf('(');
    int closeParen = rawData.lastIndexOf(')');

    if (openParen == -1 || closeParen == -1 || openParen >= closeParen) {
        return parts;
    }

    parts.header = rawData.left(openParen);
    parts.payload = rawData.mid(openParen + 1, closeParen - openParen - 1);
    parts.footer = rawData.right(rawData.size() - closeParen);
    return parts;
}

QByteArray SolverIO::updateHeaderCount(const QByteArray& header, int removedCount) {
    QString headerStr = QString::fromUtf8(header);

    // Look for the last integer before any trailing whitespace
    QRegularExpression re("(\\d+)(\\s*)$");
    QRegularExpressionMatch match = re.match(headerStr);

    if (match.hasMatch()) {
        int currentCount = match.captured(1).toInt();
        int newCount = std::max(0, currentCount - removedCount);

        // Replace the old count with the new count
        QString replacement = QString::number(newCount) + match.captured(2);
        headerStr.replace(match.capturedStart(0), match.capturedLength(0), replacement);
    }

    return headerStr.toUtf8();
}

QByteArray SolverIO::removeEmptyPatches(const QByteArray& boundaryData) {

    // Split the file
    SolverIO::BoundaryFileParts parts = splitBoundaryFile(boundaryData);
    if (parts.header.isEmpty() && parts.payload.isEmpty()) {
        return {};
    }

    // Parse the payload
    auto dict = std::make_shared<OpenFoamDictionary>(parts.payload);
    int removedCount = 0;

    // Iterate over top-level patches
    QStringList patchNames = dict->getDictKeys("");

    for (const QString& patchName : std::as_const(patchNames)) {
        QString nFacesPath = patchName + "/nFaces";
        double nFaces = dict->getNumber(nFacesPath);

        // If nFaces is exactly 0, remove the patch block
        if (!std::isnan(nFaces) && qFuzzyIsNull(nFaces)) {
            dict->removeEntry(patchName);
            removedCount++;
        }
    }

    // Update the header count and stitch it back together
    if (removedCount > 0) {
        parts.header = updateHeaderCount(parts.header, removedCount);
        parts.payload = dict->getRawText();
    }

    // Remove blank lines from payload
    QList<QByteArray> lines = parts.payload.split('\n');
    QByteArray result;
    for (const QByteArray& line : std::as_const(lines)) {
        if (!line.trimmed().isEmpty()) {
            if (!result.isEmpty())
                result += '\n';
            result += line;
        }
    }

    // Construct the reassembled file
    QByteArray finalData = parts.header + "(" + result + parts.footer;
    return finalData;
}

void SolverIO::parseFieldFile(std::shared_ptr<OpenFoamDictionary> dict, FlowCompute::FieldData& cfg) {
    if (!dict) { return; }

    cfg.dimension = dict->getString("dimensions");
    cfg.internalField = dict->getString("internalField");

    // Parse Field Class
    QString classStr = dict->getString("FoamFile/class");
    if (!classStr.isEmpty()) {
        bool ok = false;
        int enumValue = QMetaEnum::fromType<FlowCompute::FieldClass>().keyToValue(
            classStr.toUtf8().constData(), &ok);
        if (ok) {
            cfg.fieldClass = static_cast<FlowCompute::FieldClass>(enumValue);
        }
    }

    // Parse Boundary Conditions
    QStringList patchNames = dict->getDictKeys("boundaryField");
    for (const QString& patchName : std::as_const(patchNames)) {
        FlowCompute::BoundaryCondition bc;
        QString patchPath = "boundaryField/" + patchName;

        // Boundary condition type
        bc.type = dict->getString(patchPath + "/type");

        // Extract all other parameters for this specific boundary condition
        QStringList paramKeys = dict->getDictKeys(patchPath);
        for (const QString& paramKey : std::as_const(paramKeys)) {
            if (paramKey == "type") { continue; }

            // Treat all parameter values as strings for the UI backend.
            QString paramValue = dict->getString(patchPath + "/" + paramKey);

            // Only insert if it actually resolved a value
            if (!paramValue.isEmpty()) {
                bc.parameters[paramKey] = paramValue;
            }
        }

        // Append the fully constructed boundary condition to the field data
        cfg.bcs.push_back({patchName, bc});
    }    
}

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

void SolverIO::parseSolutionFile(std::shared_ptr<OpenFoamDictionary> dict, MathConfig& cfg) {
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
            QString baseField = isFinal ? fieldName.left(fieldName.length() - 5) : fieldName;

            // Ensure the base field exists in our map
            if (!cfg.fieldMathConfigs.contains(baseField)) {
                cfg.fieldMathConfigs[baseField] = FieldMathConfig();
            }

            auto& fieldConfig = cfg.fieldMathConfigs[baseField];

            // If this is a final override (e.g., pFinal), map it to the base field's struct
            if (isFinal) {
                fieldConfig.hasFinalOverride = true;
                fieldConfig.finalAbsTolerance = std::isnan(tol) ? 1e-6 : tol;
                fieldConfig.finalRelTolerance = std::isnan(relTol) ? 0.0 : relTol;
            } else {

                // solver
                if (!solverStr.isEmpty()) {
                    bool ok = false;
                    int enumValue = QMetaEnum::fromType<FlowCompute::LinearSolver>().keyToValue(
                        solverStr.toUtf8().constData(), &ok);
                    if (ok) {
                        fieldConfig.solver = static_cast<FlowCompute::LinearSolver>(enumValue);
                    }
                }

                // preconditioner
                if (!precondStr.isEmpty()) {
                    bool ok = false;
                    int enumValue = QMetaEnum::fromType<FlowCompute::Preconditioner>().keyToValue(
                        precondStr.toUtf8().constData(), &ok);
                    if (ok) {
                        fieldConfig.preconditioner = static_cast<FlowCompute::Preconditioner>(enumValue);
                    }
                }

                // smoother
                if (!smootherStr.isEmpty()) {
                    bool ok = false;
                    int enumValue = QMetaEnum::fromType<FlowCompute::Smoother>().keyToValue(
                        smootherStr.toUtf8().constData(), &ok);
                    if (ok) {
                        fieldConfig.smoother = static_cast<FlowCompute::Smoother>(enumValue);
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
                if (!cfg.fieldMathConfigs.contains(f)) cfg.fieldMathConfigs[f] = FieldMathConfig();
                cfg.fieldMathConfigs[f].relaxationFactor = val;
                cfg.fieldMathConfigs[f].isFieldsRelaxation = false;
            }
        }
    }

    // ==========================================
    // 3. Parse Algorithm Controls
    // ==========================================
    // Determine which algorithm block is present by checking its sub-dictionary keys
    if (!dict->getDictKeys("SIMPLE").isEmpty()) {
        SimpleConfig algo;
        algo.nNonOrthogonalCorrectors = dict->getNumber("SIMPLE/nNonOrthogonalCorrectors");
        if(std::isnan(algo.nNonOrthogonalCorrectors)) algo.nNonOrthogonalCorrectors = 0;

        QString consistentStr = dict->getString("SIMPLE/consistent");
        algo.consistent = (consistentStr == "yes" || consistentStr == "true");

        algo.pRefCell = dict->getNumber("SIMPLE/pRefCell");
        if(std::isnan(algo.pRefCell)) algo.pRefCell = 0;
        algo.pRefValue = dict->getNumber("SIMPLE/pRefValue");
        if(std::isnan(algo.pRefValue)) algo.pRefValue = 0.0;

        cfg.algorithmConfig = algo;
    }
    else if (!dict->getDictKeys("PIMPLE").isEmpty()) {
        PimpleConfig algo;
        algo.nOuterCorrectors = dict->getNumber("PIMPLE/nOuterCorrectors");
        if(std::isnan(algo.nOuterCorrectors)) algo.nOuterCorrectors = 1;

        algo.nCorrectors = dict->getNumber("PIMPLE/nCorrectors");
        if(std::isnan(algo.nCorrectors)) algo.nCorrectors = 2;

        algo.nNonOrthogonalCorrectors = dict->getNumber("PIMPLE/nNonOrthogonalCorrectors");
        if(std::isnan(algo.nNonOrthogonalCorrectors)) algo.nNonOrthogonalCorrectors = 0;

        QString mp = dict->getString("PIMPLE/momentumPredictor");
        algo.momentumPredictor = (mp == "yes" || mp == "true" || mp.isEmpty());

        algo.pRefCell = dict->getNumber("PIMPLE/pRefCell");
        if(std::isnan(algo.pRefCell)) algo.pRefCell = 0;
        algo.pRefValue = dict->getNumber("PIMPLE/pRefValue");
        if(std::isnan(algo.pRefValue)) algo.pRefValue = 0.0;

        cfg.algorithmConfig = algo;
    }
    else if (!dict->getDictKeys("PISO").isEmpty()) {
        PisoConfig algo;
        algo.nCorrectors = dict->getNumber("PISO/nCorrectors");
        if(std::isnan(algo.nCorrectors)) algo.nCorrectors = 2;

        algo.nNonOrthogonalCorrectors = dict->getNumber("PISO/nNonOrthogonalCorrectors");
        if(std::isnan(algo.nNonOrthogonalCorrectors)) algo.nNonOrthogonalCorrectors = 0;

        QString mp = dict->getString("PISO/momentumPredictor");
        algo.momentumPredictor = (mp == "yes" || mp == "true" || mp.isEmpty());

        algo.pRefCell = dict->getNumber("PISO/pRefCell");
        if(std::isnan(algo.pRefCell)) algo.pRefCell = 0;
        algo.pRefValue = dict->getNumber("PISO/pRefValue");
        if(std::isnan(algo.pRefValue)) algo.pRefValue = 0.0;

        cfg.algorithmConfig = algo;
    }
}

void SolverIO::parseParallelFile(std::shared_ptr<OpenFoamDictionary> dict, ParallelConfig& config) {
    if (!dict || dict->hasSyntaxErrors()) {
        config.useParallel = false;
        return;
    }

    // If we are successfully reading a valid decomposeParDict,
    // it's safe to assume parallel execution is intended.
    config.useParallel = true;

    // 2. Parse numberOfSubdomains
    double numSubs = dict->getNumber("numberOfSubdomains");
    if (!std::isnan(numSubs) && numSubs > 0) {
        config.numSubdomains = static_cast<int>(numSubs);
    }

    // 3. Parse decomposition method
    QString methodStr = dict->getString("method").toLower();
    QString coeffsDictName;

    if (methodStr == "scotch") {
        config.method = FlowCompute::DecompositionMethod::Scotch;
    } else if (methodStr == "metis") {
        config.method = FlowCompute::DecompositionMethod::Metis;
    } else if (methodStr == "simple") {
        config.method = FlowCompute::DecompositionMethod::Simple;
        coeffsDictName = "simpleCoeffs";
    } else if (methodStr == "hierarchical") {
        config.method = FlowCompute::DecompositionMethod::Hierarchical;
        coeffsDictName = "hierarchicalCoeffs";
    }

    // 4. Parse method-specific coefficients
    if (!coeffsDictName.isEmpty()) {
        // Extract the subdivision vector 'n'
        QStringList nList = dict->getList(coeffsDictName + "/n");
        if (nList.size() >= 3) {
            bool okX, okY, okZ;
            int nx = nList[0].toInt(&okX);
            int ny = nList[1].toInt(&okY);
            int nz = nList[2].toInt(&okZ);

            if (okX && okY && okZ) {
                config.nx = nx;
                config.ny = ny;
                config.nz = nz;
            }
        }

        // Extract the cell skew factor 'delta'
        double delta = dict->getNumber(coeffsDictName + "/delta");
        if (!std::isnan(delta)) {
            config.delta = delta;
        }

        // Extract 'order' (Standard for hierarchical)
        if (config.method == FlowCompute::DecompositionMethod::Hierarchical) {
            QString orderStr = dict->getString(coeffsDictName + "/order");
            if (!orderStr.isEmpty()) {
                config.order = orderStr;
            }
        }
    }
}

QString SolverIO::createControlDict(const ControlConfig& cfg, const VisualizationConfig& visCfg,
                                    const QString& openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << Utils::createFoamHeader("controlDict", openFoamPath);

    auto writeEntry = [&out](const QString& keyword, const QString& value) {
        out << keyword.leftJustified(20, ' ') << value << ";\n";
    };

    // Helper lambda for booleans (optional, but keeps styling consistent)
    auto toFoamBool = [](bool val) { return val ? "yes" : "no"; };

    // Application
    out << "// Simulation executable\n";
    writeEntry("application", cfg.solver);
    out << "\n";

    // Time control
    out << "// Time control\n";
    writeEntry("startFrom", enumToString(cfg.startFrom, "startTime"));
    writeEntry("startTime", QString::number(cfg.startTime));
    writeEntry("stopAt", enumToString(cfg.stopAt, "endTime"));
    writeEntry("endTime", QString::number(cfg.endTime));
    writeEntry("deltaT", QString::number(cfg.deltaT));
    out << "\n";

    // Data writing
    out << "// Data writing\n";
    writeEntry("writeControl", enumToString(cfg.writeControl, "timeStep"));
    writeEntry("writeInterval", QString::number(cfg.writeInterval));
    writeEntry("purgeWrite", QString::number(cfg.purgeWrite));
    out << "\n";

    // Output format
    out << "// Output format\n";
    writeEntry("writeFormat", enumToString(cfg.writeFormat, "binary"));
    writeEntry("writePrecision", "6");
    writeEntry("writeCompression", cfg.writeCompression ? "on" : "off");
    writeEntry("timeFormat", "general");
    writeEntry("timePrecision", "6");
    out << "\n";

    // Standard default entries
    out << "// Runtime configuration\n";
    writeEntry("runTimeModifiable", toFoamBool(cfg.runTimeModifiable));

    // Evaluate and write functions block if visualization data is present
    if (!visCfg.fieldNames.empty() && !visCfg.surfaces.empty()) {
        out << "\n// Visualization and post-processing functions\n";
        out << "functions\n{\n";
        out << "    surfaces\n    {\n";
        out << "        type            surfaces;\n";
        out << "        libs            (\"libsampling.so\");\n";

        out << "        writeControl    " << enumToString(visCfg.writeControl, "writeTime") << ";\n";
        out << "        writeInterval   " << QString::number(visCfg.writeInterval) << ";\n";
        out << "        surfaceFormat   " << enumToString(visCfg.surfaceFormat, "vtk") << ";\n";
        out << "        interpolationScheme " << enumToString(visCfg.interpolationType, "cell") << ";\n";

        // Write fields array
        out << "        fields          (";
        for (const QString& field : visCfg.fieldNames) {
            out << field << " ";
        }
        out << ");\n";

        // Write individual surfaces
        out << "        surfaces\n        (\n";
        for (const auto& surface : visCfg.surfaces) {
            out << "            " << surface.name << "\n            {\n";
            out << "                type           " << enumToString(surface.type, "patch") << ";\n";

            // Iterate over arbitrary mapped parameters (e.g., normal vectors, base points)
            for (const auto& [paramName, paramValue] : surface.parameters) {
                out << "                " << paramName.leftJustified(15, ' ') << paramValue << ";\n";
            }
            out << "            }\n";
        }
        out << "        );\n    }\n}\n";
    }

    // Write closing separator
    out << "\n// ************************************************************************* //\n";

    return dictStr;
}

QString SolverIO::createTurbulenceProperties(const PhysicsConfig& cfg, const QString& openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << Utils::createFoamHeader("turbulenceProperties", openFoamPath);

    // Upgraded lambda with indentation support for sub-dictionaries
    auto writeEntry = [&out](const QString& keyword, const QString& value, int indentLevel = 0, bool addEmptyLine = false) {
        QString indent(indentLevel * 4, ' ');
        out << indent << keyword.leftJustified(20, ' ') << value << ";\n";
        if (addEmptyLine) out << "\n";
    };

    // Standard OpenFOAM switch formatting for turbulence settings
    auto toFoamSwitch = [](bool val) { return val ? "on" : "off"; };

    // 1. Base simulation type (RAS, LES, or laminar)
    writeEntry("simulationType", cfg.simulationType, 0, true);



    // 2. Generate the specific sub-dictionary if it's not laminar
    if (cfg.simulationType == "RAS" || cfg.simulationType == "LES") {
        out << cfg.simulationType << "\n{\n";

        // e.g., RASModel or LESModel
        writeEntry(cfg.simulationType + "Model", cfg.model, 1);

        // Turbulence toggle
        writeEntry("turbulence", toFoamSwitch(cfg.useTurbulence), 1);

        // Standard coefficient output switch
        writeEntry("printCoeffs", "on", 1);

        // If LES, write the delta model using the enumToString template
        if (cfg.simulationType == "LES") {
            writeEntry("delta", enumToString(cfg.deltaModel, "cubeRootVol"), 1);
        }

        out << "}\n\n";
    }

    // Write closing separator
    out << "// ************************************************************************* //\n";

    return dictStr;
}

QString SolverIO::createTransportProperties(const PhysicsConfig& cfg, const QString& openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << Utils::createFoamHeader("transportProperties", openFoamPath);

    // Standard lambda for formatting dictionary entries
    auto writeEntry = [&out](const QString& keyword, const QString& value, bool addEmptyLine = false) {
        out << keyword.leftJustified(20, ' ') << value << ";\n";
        if (addEmptyLine) out << "\n";
    };

    // Write the transport model using the enum template
    writeEntry("transportModel", enumToString(cfg.transportModel, "Newtonian"), true);

    // Iterate through the fluid properties map
    if (!cfg.fluidProperties.isEmpty()) {
        for (auto it = cfg.fluidProperties.cbegin(); it != cfg.fluidProperties.cend(); ++it) {
            writeEntry(it.key(), it.value());
        }
        out << "\n";
    }

    // Write closing separator
    out << "// ************************************************************************* //\n";

    return dictStr;
}

QString SolverIO::createFieldFile(const QString& fieldName,
                                  const FlowCompute::FieldData& data,
                                  const QString& openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Field class
    QString fieldClassStr = enumToString(data.fieldClass, "volScalarField");

    // Write the standard header and correctly replace the class type
    QString header = Utils::createFoamHeader(fieldName, openFoamPath);
    header.replace("dictionary", fieldClassStr);
    out << header;

    // Standard lambda with indentation support
    auto writeEntry = [&out](const QString& keyword, const QString& value, int indentLevel = 0, bool addEmptyLine = false) {
        QString indent(indentLevel * 4, ' ');
        out << indent << keyword.leftJustified(20, ' ') << value << ";\n";
        if (addEmptyLine) out << "\n";
    };

    // 1. Dimensions and internal field
    writeEntry("dimensions", data.dimension, 0, true);
    writeEntry("internalField", data.internalField, 0, true);

    // 2. Boundary field sub-dictionary
    out << "boundaryField\n{\n";

    for (const auto& [patchName, bc] : data.bcs) {
        out << "    " << patchName << "\n    {\n";

        // Write the boundary condition type
        writeEntry("type", bc.type, 2);

        // Iterate through and write any additional parameters
        for (const auto& [paramName, paramValue] : bc.parameters) {
            writeEntry(paramName, paramValue, 2);
        }

        out << "    }\n";
    }

    out << "}\n\n";

    // Write closing separator
    out << "// ************************************************************************* //\n";

    return dictStr;
}

QString SolverIO::createSolutionFile(const MathConfig& cfg, const QString& openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << Utils::createFoamHeader("fvSolution", openFoamPath);

    // Standard lambda with indentation support
    auto writeEntry = [&out](const QString& keyword, const QString& value, int indentLevel = 0, bool addEmptyLine = false) {
        QString indent(indentLevel * 4, ' ');
        out << indent << keyword.leftJustified(20, ' ') << value << ";\n";
        if (addEmptyLine) out << "\n";
    };

    // OpenFOAM canonical switch format
    auto toFoamSwitch = [](bool val) { return val ? "yes" : "no"; };

    // Solvers sub-dictionary
    out << "// Solver settings for each variable\n";
    out << "solvers\n{\n";
    for (auto it = cfg.fieldMathConfigs.cbegin(); it != cfg.fieldMathConfigs.cend(); ++it) {
        const QString& fieldName = it.key();
        const FieldMathConfig& fCfg = it.value();

        if (fieldName == "< default >")
            continue;

        // Local lambda to write a specific solver block (e.g., 'p' or 'pFinal')
        auto writeSolverBlock = [&](const QString& name, double absTol, double relTol) {
            out << "    " << name << "\n    {\n";
            writeEntry("solver", enumToString(fCfg.solver, "GAMG"), 2);

            // OpenFOAM usually omits smoother/preconditioner if they aren't used.
            // We assume enumToString returns "NONE" for the NONE enum value.
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
            writeSolverBlock(fieldName + "Final", fCfg.finalAbsTolerance, fCfg.finalRelTolerance);
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
                        // indentLevel 2 gives 8 spaces, matching OpenFOAM dictionary standards
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
            writeEntry("nNonOrthogonalCorrectors   ", QString::number(algoCfg.nNonOrthogonalCorrectors), 1);
            writeEntry("consistent   ", toFoamSwitch(algoCfg.consistent), 1);
            if (algoCfg.pRefCell >= 0) {
                writeEntry("pRefCell   ", QString::number(algoCfg.pRefCell), 1);
                writeEntry("pRefValue   ", QString::number(algoCfg.pRefValue), 1);
            }

            // Generate the residualControl block for SIMPLE
            writeResidualControls(algoCfg);

            out << "}\n\n";
        }
        else if constexpr (std::is_same_v<T, PisoConfig>) {
            out << "PISO\n{\n";
            writeEntry("momentumPredictor   ", toFoamSwitch(algoCfg.momentumPredictor), 1);
            writeEntry("nCorrectors   ", QString::number(algoCfg.nCorrectors), 1);
            writeEntry("nNonOrthogonalCorrectors   ", QString::number(algoCfg.nNonOrthogonalCorrectors), 1);
            if (algoCfg.pRefCell >= 0) {
                writeEntry("pRefCell", QString::number(algoCfg.pRefCell), 1);
                writeEntry("pRefValue", QString::number(algoCfg.pRefValue), 1);
            }
            out << "}\n\n";
        }
        else if constexpr (std::is_same_v<T, PimpleConfig>) {
            out << "PIMPLE\n{\n";
            writeEntry("momentumPredictor", toFoamSwitch(algoCfg.momentumPredictor), 1);
            writeEntry("nOuterCorrectors", QString::number(algoCfg.nOuterCorrectors), 1);
            writeEntry("nCorrectors", QString::number(algoCfg.nCorrectors), 1);
            writeEntry("nNonOrthogonalCorrectors", QString::number(algoCfg.nNonOrthogonalCorrectors), 1);
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
    for (auto it = cfg.fieldMathConfigs.cbegin(); it != cfg.fieldMathConfigs.cend(); ++it) {

        if (it.key() == "< default >") {
            continue;
        }

        QString indent(8, ' ');
        QString formattedEntry = indent + it.key().leftJustified(20, ' ') + QString::number(it.value().relaxationFactor) + ";\n";

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

QString SolverIO::createParallelFile(const ParallelConfig& cfg, const QString& openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << Utils::createFoamHeader("decomposeParDict", openFoamPath);

    // Standard lambda with indentation support
    auto writeEntry = [&out](const QString& keyword, const QString& value, int indentLevel = 0, bool addEmptyLine = false) {
        QString indent(indentLevel * 4, ' ');
        out << indent << keyword.leftJustified(20, ' ') << value << ";\n";
        if (addEmptyLine) out << "\n";
    };

    // Core decomposition parameters
    writeEntry("numberOfSubdomains", QString::number(cfg.numSubdomains), 0, true);

    // OpenFOAM expects lowercase method names (e.g., "scotch", "simple")
    QString methodStr = enumToString(cfg.method, "scotch").toLower();
    writeEntry("method", methodStr, 0, true);

    // Method-specific coefficients block
    QString coeffsDictName = methodStr + "Coeffs";
    out << coeffsDictName << "\n{\n";

    // 'simple' and 'hierarchical' require the 'n' vector and 'delta'
    if (methodStr == "simple" || methodStr == "hierarchical") {
        QString nStr = QString("(%1 %2 %3)").arg(cfg.nx).arg(cfg.ny).arg(cfg.nz);
        writeEntry("n", nStr, 1);

        // 'hierarchical' additionally requires the axis ordering
        if (methodStr == "hierarchical") {
            writeEntry("order", cfg.order, 1);
        }

        writeEntry("delta", QString::number(cfg.delta), 1);
    }

    out << "}\n\n";

    // Write closing separator
    out << "// ************************************************************************* //\n";

    return dictStr;
}