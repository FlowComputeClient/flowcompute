#include "solver_io.h"

QString SolverIO::createFoamHeader(const QString& objectName, const QString& foamPath) {
    QString headerStr;
    QTextStream out(&headerStr);

    // Default fallback values
    bool isESI = false;
    QString verText = "unknown";
    QRegularExpression re("openfoam-?v?(\\d+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(foamPath);

    if (match.hasMatch()) {
        QString digits = match.captured(1);
        int verNumber = digits.toInt();

        // ESI/Keysight releases use YYMM (e.g., 2312, 2412), so the number is always > 100
        if (verNumber > 100) {
            isESI = true;
            verText = "v" + digits;
        } else {
            // Foundation releases use sequential major versions (e.g., 11, 12, 13)
            isESI = false;
            verText = digits;
        }
    } else {
        qWarning() << "Warning: Could not parse OpenFOAM version from path:" << foamPath;
    }

    QString line3_right, line4_right;

    if (!isESI) {
        // Foundation (.org) format
        line3_right = "Website:  https://openfoam.org";
        line4_right = "Version:  " + verText;
    } else {
        // Keysight / ESI (.com) format
        line3_right = "Version:  " + verText;
        line4_right = "Website:  www.openfoam.com";
    }

    // Write the banner, padding the right side to exactly 47 characters
    out << "/*--------------------------------*- C++ -*----------------------------------*\\\n";
    out << "| =========                 |                                                 |\n";
    out << "| \\\\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox           |\n";
    out << "|  \\\\    /   O peration     | " << line3_right.leftJustified(47, ' ') << "|\n";
    out << "|   \\\\  /    A nd           | " << line4_right.leftJustified(47, ' ') << "|\n";
    out << "|    \\\\/     M anipulation  |                                                 |\n";
    out << "\\*---------------------------------------------------------------------------*/\n";

    // Write the required FoamFile dictionary
    out << "FoamFile\n";
    out << "{\n";
    out << "    version     2.0;\n";
    out << "    format      ascii;\n";
    out << "    class       dictionary;\n";
    out << "    object      " << objectName << ";\n";
    out << "}\n";
    out << "// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n\n";

    return headerStr;
}

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
