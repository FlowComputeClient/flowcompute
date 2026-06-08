#include "solver_io.h"

ControlConfig SolverIO::parseControlConfig(std::shared_ptr<OpenFoamDictionary> dict) {
    ControlConfig config;

    if (!dict) {
        return config;
    }

    // 1. Solver / Application
    // In OpenFOAM, the solver is typically specified by the "application" keyword
    config.solver = dict->getString("application");

    // 2. Run Control - startFrom
    QString startFromStr = dict->getString("startFrom");
    if (startFromStr == "latestTime") {
        config.startFrom = StartSolverType::latestTime;
    } else if (startFromStr == "firstTime") {
        config.startFrom = StartSolverType::firstTime;
    } else {
        // Defaults to startTime if missing or set to "startTime"
        config.startFrom = StartSolverType::startTime;
    }

    // Read startTime (safely check if present)
    double sTime = dict->getNumber("startTime");
    if (!std::isnan(sTime)) {
        config.startTime = sTime;
    }

    // 3. Run Control - stopAt
    QString stopAtStr = dict->getString("stopAt");
    if (stopAtStr == "writeNow") {
        config.stopAt = EndSolverType::writeNow;
    } else if (stopAtStr == "noWriteNow") {
        config.stopAt = EndSolverType::noWriteNow;
    } else if (stopAtStr == "nextWrite") {
        config.stopAt = EndSolverType::nextWrite;
    } else {
        config.stopAt = EndSolverType::endTime;
    }

    // Read endTime and deltaT
    double eTime = dict->getNumber("endTime");
    if (!std::isnan(eTime)) {
        config.endTime = eTime;
    }

    double dT = dict->getNumber("deltaT");
    if (!std::isnan(dT)) {
        config.deltaT = dT;
    }

    // 4. Time step adjustment
    QString adjustStr = dict->getString("adjustTimeStep").toLower();
    // OpenFOAM typically uses "yes"/"no", "true"/"false", or "on"/"off"
    config.adjustTimeStep = (adjustStr == "yes" || adjustStr == "true" || adjustStr == "on");

    double maxCourant = dict->getNumber("maxCo");
    if (!std::isnan(maxCourant)) {
        config.maxCo = maxCourant;
    }

    // 5. Data writing
    QString writeCtrlStr = dict->getString("writeControl");
    if (writeCtrlStr == "runTime") {
        config.writeControl = WriteControlType::runTime;
    } else if (writeCtrlStr == "adjustableRunTime" || writeCtrlStr == "adjustable") {
        config.writeControl = WriteControlType::adjustableRunTime;
    } else if (writeCtrlStr == "cpuTime") {
        config.writeControl = WriteControlType::cpuTime;
    } else if (writeCtrlStr == "clockTime") {
        config.writeControl = WriteControlType::clockTime;
    } else if (writeCtrlStr == "none") {
        config.writeControl = WriteControlType::none;
    } else {
        config.writeControl = WriteControlType::timeStep;
    }

    double writeInt = dict->getNumber("writeInterval");
    if (!std::isnan(writeInt)) {
        config.writeInterval = writeInt;
    }

    double purgeW = dict->getNumber("purgeWrite");
    if (!std::isnan(purgeW)) {
        config.purgeWrite = static_cast<int>(purgeW);
    }

    return config;
}

PhysicsConfig SolverIO::parseTurbulenceProperties(std::shared_ptr<OpenFoamDictionary> dict) {
    PhysicsConfig cfg;

    // 1. Establish safe defaults
    cfg.simulationType = "RAS";
    cfg.model = "kOmegaSST";

    // Assuming your enums are wrapped in a namespace like FlowCompute
    // cfg.delta = FlowCompute::DeltaType::UNKNOWN;
    // cfg.transportModel = FlowCompute::TransportModel::UNKNOWN;

    if (!dict) {
        return cfg;
    }

    // 2. Read the master switch
    QString simType = dict->getString("simulationType");
    if (!simType.isEmpty()) {
        cfg.simulationType = simType;
    }

    QString subDict;

    // 3. Route the parsing based on simulationType
    if (cfg.simulationType == "RAS") {
        subDict = "RAS";
        QString parsedModel = dict->getString("RAS/RASModel");
        if (!parsedModel.isEmpty()) cfg.model = parsedModel;

    } else if (cfg.simulationType == "LES") {
        subDict = "LES";
        QString parsedModel = dict->getString("LES/LESModel");
        if (!parsedModel.isEmpty()) cfg.model = parsedModel;

        // Extract delta specifically for LES
        QString deltaStr = dict->getString("LES/delta");
        // cfg.delta = stringToDeltaType(deltaStr); // Use your mapping utility

    } else if (cfg.simulationType == "laminar") {
        cfg.model = "laminar";
        return cfg;
    }

    return cfg;
}

void SolverIO::parseTransportProperties(std::shared_ptr<OpenFoamDictionary> dict,
                                        PhysicsConfig& cfg) {
    if (!dict) {
        return;
    }

    static const QMap<QString, TransportModel> transportMap = {
        {"Newtonian", TransportModel::Newtonian},
        {"CrossPowerLaw", TransportModel::CrossPowerLaw},
        {"BirdCarreau", TransportModel::BirdCarreau},
        {"HerschelBulkley", TransportModel::HerschelBulkley}
    };

    // 1. Parse the transport model
    QString modelStr = dict->getString("transportModel");
    if (!modelStr.isEmpty()) {
        // Assuming stringToTransportModel is defined in your SolverUtils.h
        cfg.transportModel = transportMap[modelStr];
    } else {
        // Fallback to the standard default
        cfg.transportModel = TransportModel::Newtonian;
    }

    // 2. Define the standard properties FlowCompute cares about
    // This covers Newtonian, heat transfer, compressibility, and non-Newtonian models
    QStringList standardProperties = {
        "nu", "rho", "Pr", "Prt", "TRef", "Cp", "Cv", "k", "n", "alpha"
    };

    // 3. Extract existing properties into the dynamic map
    for (const QString& propName : standardProperties) {
        QString propValue = dict->getString(propName);

        if (!propValue.isEmpty()) {
            // propValue will likely contain the dimension array as well
            // e.g., "[0 2 -1 0 0 0 0] 1e-6"
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