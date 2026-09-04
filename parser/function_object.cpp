#include "function_object.h"

#include <QDebug>
#include <QMetaEnum>
#include <QString>
#include <QTextStream>

int getTaskTypeIndex(const QString& taskType) {
    QMetaEnum metaEnum = QMetaEnum::fromType<CaseIO::FunctionObject::FuncObjType>();
    QByteArray typeBytes = taskType.toUtf8();
    int index = metaEnum.keyToValue(typeBytes.constData());
    return index;
}

// Helper to safely parse OpenFOAM boolean strings (true/false, yes/no, on/off)
static bool parseFoamBool(const QString& val, bool defaultValue = false) {
    if (val.isEmpty()) return defaultValue;
    QString lower = val.toLower();
    return (lower == "true" || lower == "yes" || lower == "on");
}

// Helper to extract a 3-element array from a flat QStringList
static std::array<double, 3> parseArray3(const QStringList& list,
                            const std::array<double, 3>& defaultArr) {
    if (list.size() >= 3) {
        bool okX, okY, okZ;
        double x = list[0].toDouble(&okX);
        double y = list[1].toDouble(&okY);
        double z = list[2].toDouble(&okZ);
        if (okX && okY && okZ) return {x, y, z};
    }
    return defaultArr;
}

// Helper to map string to Q_ENUM values via QMetaEnum
template<typename T>
static T parseEnum(const QString& str, T defaultValue) {
    if (str.isEmpty()) return defaultValue;
    QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    bool ok = false;
    int val = metaEnum.keyToValue(str.toUtf8().constData(), &ok);
    return ok ? static_cast<T>(val) : defaultValue;
}

// Helper to populate base FunctionObject properties
static void parseBaseProperties(CaseIO::FunctionObject* obj,
    const QString& path, const std::shared_ptr<OpenFoamDictionary>& dict) {
    obj->name = path.split('/').last();

    QString activeStr = dict->getString(path + "/active");
    if (!activeStr.isEmpty()) obj->active = parseFoamBool(activeStr, true);

    obj->executeControl = parseEnum<CaseIO::FunctionObject::ControlType>(
        dict->getString(path + "/executeControl"),
        CaseIO::FunctionObject::ControlType::timeStep);

    double execInt = dict->getNumber(path + "/executeInterval");
    if (!std::isnan(execInt)) obj->executeInterval = execInt;

    obj->writeControl = parseEnum<CaseIO::FunctionObject::ControlType>(
        dict->getString(path + "/writeControl"),
        CaseIO::FunctionObject::ControlType::writeTime);

    double writeInt = dict->getNumber(path + "/writeInterval");
    if (!std::isnan(writeInt)) obj->writeInterval = writeInt;

    obj->logOutput = parseFoamBool(dict->getString(path + "/log"), false);
}

std::vector<std::unique_ptr<CaseIO::FunctionObject>>
    CaseIO::parsePostProcessDict(std::shared_ptr<OpenFoamDictionary> dict) {
    std::vector<std::unique_ptr<FunctionObject>> functionObjects;
    if (!dict)
        return functionObjects;

    // Check if objects are wrapped in a "functions" block
    QString basePath = "functions";
    QStringList objectNames = dict->getDictKeys(basePath);
    if (objectNames.isEmpty()) {
        basePath = "";
        objectNames = dict->getDictKeys("");
    }

    for (const QString& objName : std::as_const(objectNames)) {
        QString path =
            basePath.isEmpty() ? objName : (basePath + "/" + objName);
        QString typeStr = dict->getString(path + "/type");

        if (typeStr == "forces") {
            auto obj = std::make_unique<ForcesConfig>();
            obj->type = FunctionObject::FuncObjType::forces;
            parseBaseProperties(obj.get(), path, dict);

            obj->writeFields =
                parseFoamBool(dict->getString(path + "/writeFields"), false);
            obj->patches = dict->getList(path + "/patches");

            QString pName = dict->getString(path + "/pName");
            if (!pName.isEmpty()) obj->pName = pName;

            QString rhoName = dict->getString(path + "/rhoName");
            if (!rhoName.isEmpty()) obj->rhoName = rhoName;

            double rhoInf = dict->getNumber(path + "/rhoInf");
            if (!std::isnan(rhoInf)) obj->rhoInf = rhoInf;

            QString uName = dict->getString(path + "/UName");
            if (!uName.isEmpty()) obj->UName = uName;

            double pRef = dict->getNumber(path + "/pRef");
            if (!std::isnan(pRef)) obj->pRef = pRef;

            obj->centerOfRotation = parseArray3(
                dict->getList(path + "/CofR"), obj->centerOfRotation);
            obj->includePorosity =
                parseFoamBool(dict->getString(path + "/porosity"), false);

            functionObjects.push_back(std::move(obj));

        } else if (typeStr == "forceCoeffs") {
            auto obj = std::make_unique<ForceCoeffsConfig>();
            obj->type = FunctionObject::FuncObjType::forceCoeffs;
            parseBaseProperties(obj.get(), path, dict);

            obj->writeFields =
                parseFoamBool(dict->getString(path + "/writeFields"), false);
            obj->patches = dict->getList(path + "/patches");

            QString pName = dict->getString(path + "/pName");
            if (!pName.isEmpty()) obj->pName = pName;

            double magUInf = dict->getNumber(path + "/magUInf");
            if (!std::isnan(magUInf)) obj->magUInf = magUInf;

            double lRef = dict->getNumber(path + "/lRef");
            if (!std::isnan(lRef)) obj->lRef = lRef;

            double aRef = dict->getNumber(path + "/Aref");
            if (!std::isnan(aRef)) obj->aRef = aRef;

            obj->centerOfRotation = parseArray3(
                dict->getList(path + "/CofR"), obj->centerOfRotation);
            obj->liftDir =
                parseArray3(dict->getList(path + "/liftDir"), obj->liftDir);
            obj->dragDir =
                parseArray3(dict->getList(path + "/dragDir"), obj->dragDir);
            obj->pitchAxis =
                parseArray3(dict->getList(path + "/pitchAxis"), obj->pitchAxis);

            functionObjects.push_back(std::move(obj));

        } else if (typeStr == "fieldMinMax") {
            auto obj = std::make_unique<FieldMinMaxConfig>();
            obj->type = FunctionObject::FuncObjType::fieldMinMax;
            parseBaseProperties(obj.get(), path, dict);

            obj->fields = dict->getList(path + "/fields");
            obj->mode = parseEnum<FieldMinMaxConfig::Mode>(
                dict->getString(path + "/mode"),
                FieldMinMaxConfig::Mode::Magnitude);
            obj->location = parseFoamBool(
                dict->getString(path + "/location"), true);

            functionObjects.push_back(std::move(obj));

        } else if (typeStr == "probes") {
            auto obj = std::make_unique<ProbesConfig>();
            obj->type = FunctionObject::FuncObjType::probes;
            parseBaseProperties(obj.get(), path, dict);

            obj->fields = dict->getList(path + "/fields");
            obj->interpolationScheme =
                parseEnum<FunctionObject::InterpolationType>(
                    dict->getString(path + "/interpolationScheme"),
                    FunctionObject::InterpolationType::cell);
            obj->fixedLocations = parseFoamBool(
                dict->getString(path + "/fixedLocations"), true);
            obj->includeOutOfBounds = parseFoamBool(
                dict->getString(path + "/includeOutOfBounds"), true);
            obj->verbose = parseFoamBool(
                dict->getString(path + "/verbose"), false);
            obj->sampleOnExecute = parseFoamBool(
                dict->getString(path + "/sampleOnExecute"), false);

            // Extract flat coordinate list and convert to QVector3D
            QStringList locList = dict->getList(path + "/probeLocations");
            for (int i = 0; i <= locList.size() - 3; i += 3) {
                bool okX, okY, okZ;
                float x = locList[i].toFloat(&okX);
                float y = locList[i + 1].toFloat(&okY);
                float z = locList[i + 2].toFloat(&okZ);
                if (okX && okY && okZ) {
                    obj->probeLocations.push_back(QVector3D(x, y, z));
                }
            }

            functionObjects.push_back(std::move(obj));

        } else if (typeStr == "surfaces") {
            auto obj = std::make_unique<SurfacesConfig>();
            obj->type = FunctionObject::FuncObjType::surfaces;
            parseBaseProperties(obj.get(), path, dict);

            obj->surfaceFormat = parseEnum<SurfacesConfig::SurfaceFormat>(
                dict->getString(path + "/surfaceFormat"),
                SurfacesConfig::SurfaceFormat::vtk);
            obj->interpolationScheme =
                parseEnum<FunctionObject::InterpolationType>(
                    dict->getString(path + "/interpolationScheme"),
                    FunctionObject::InterpolationType::cell);
            obj->fields = dict->getList(path + "/fields");

            QString surfacesPath = path + "/surfaces";
            QStringList surfaceKeys = dict->getDictKeys(surfacesPath);

            for (const QString& surfKey : std::as_const(surfaceKeys)) {
                SurfaceDef sDef;
                sDef.name = surfKey;

                QString surfPath = surfacesPath + "/" + surfKey;
                sDef.type = parseEnum<SurfaceDef::SurfaceType>(
                    dict->getString(surfPath + "/type"),
                    SurfaceDef::SurfaceType::patch);

                // Store parameters as key-value pairs (excluding "type")
                QStringList paramKeys = dict->getDictKeys(surfPath);
                for (const QString& pKey : std::as_const(paramKeys)) {
                    if (pKey != "type") {
                        sDef.parameters[pKey] =
                            dict->getString(surfPath + "/" + pKey);
                    }
                }
                obj->surfaces.push_back(sDef);
            }

            functionObjects.push_back(std::move(obj));

        } else if (typeStr == "yPlus") {
            auto obj = std::make_unique<YPlusConfig>();
            obj->type = FunctionObject::FuncObjType::yPlus;
            parseBaseProperties(obj.get(), path, dict);

            obj->patches = dict->getList(path + "/patches");

            functionObjects.push_back(std::move(obj));
        }
    }
    return functionObjects;
}

// Create functions block
QString CaseIO::createFunctionsBlock(
    const std::vector<std::unique_ptr<FunctionObject>>& functions) {
    // Create string and stream
    QString blockString;
    QTextStream blockStream(&blockString);

    // Initialize and populate the functions block
    blockStream << "functions\n{\n";
    for (const auto& function: functions) {
        FunctionObject* funcPtr = function.get();
        switch(funcPtr->type) {
        case FunctionObject::FuncObjType::forces:
            createForcesBlock(blockStream,
                static_cast<const ForcesConfig*>(funcPtr));
            break;
        case FunctionObject::FuncObjType::forceCoeffs:
            createForceCoeffsBlock(blockStream,
                static_cast<const ForceCoeffsConfig*>(funcPtr));
            break;
        case FunctionObject::FuncObjType::fieldMinMax:
            createFieldMinMaxBlock(blockStream,
                static_cast<const FieldMinMaxConfig*>(funcPtr));
            break;
        case FunctionObject::FuncObjType::probes:
            createProbesBlock(blockStream,
                static_cast<const ProbesConfig*>(funcPtr));
            break;
        case FunctionObject::FuncObjType::surfaces:
            createSurfacesBlock(blockStream,
                static_cast<const SurfacesConfig*>(funcPtr));
            break;
        case FunctionObject::FuncObjType::yPlus:
            createYPlusBlock(blockStream,
                static_cast<const YPlusConfig*>(funcPtr));
            break;
        default:
            qWarning() << "Warning: Unhandled FunctionObject type.";
            break;
        }
    }

    // Close the functions block
    blockStream << "}\n";
    return blockString;
}

void CaseIO::createForcesBlock(QTextStream& stream,
                               const ForcesConfig* config) {
    if (!config)
        return;

    auto boolStr = [](bool val) { return val ? "true" : "false"; };

    QMetaEnum controlEnum = QMetaEnum::fromType<FunctionObject::ControlType>();
    QString execControl =
        controlEnum.valueToKey(static_cast<int>(config->executeControl));
    QString writeControl =
        controlEnum.valueToKey(static_cast<int>(config->writeControl));

    // Write directly to the parent stream
    stream << "    " << config->name << "\n"
           << "    {\n"
           << "        type            forces;\n"
           << "        libs            (\"libforces.so\");\n\n";

    stream << "        active          " << boolStr(config->active) << ";\n"
           << "        executeControl  " << execControl << ";\n"
           << "        executeInterval " << config->executeInterval << ";\n"
           << "        writeControl    " << writeControl << ";\n"
           << "        writeInterval   " << config->writeInterval << ";\n"
           << "        log             " <<
        boolStr(config->logOutput) << ";\n\n";

    stream << "        writeFields     " <<
        boolStr(config->writeFields) << ";\n";

    stream << "        patches         (";
    for (int i = 0; i < config->patches.size(); ++i) {
        stream << config->patches[i];
        if (i < config->patches.size() - 1) {
            stream << " ";
        }
    }
    stream << ");\n\n";

    stream << "        p               " << config->pName << ";\n"
           << "        U               " << config->UName << ";\n"
           << "        rho             " << config->rhoName << ";\n";
    if (config->rhoName == "rhoInf") {
        stream << "        rhoInf          " << config->rhoInf << ";\n";
    }
    stream << "        pRef            " << config->pRef << ";\n"
           << "        porosity        " <<
        boolStr(config->includePorosity) << ";\n";

    stream << "        CofR            ("
           << config->centerOfRotation[0] << " "
           << config->centerOfRotation[1] << " "
           << config->centerOfRotation[2] << ");\n";

    stream << "    }\n";
}

// Create text of forceCoeffs function object
void CaseIO::createForceCoeffsBlock(QTextStream& stream,
                                    const ForceCoeffsConfig* config) {
    if (!config)
        return;

    // Lambda to convert boolean values to OpenFOAM "true"/"false" syntax
    auto boolToStr = [](bool val) { return val ? "true" : "false"; };

    // Map base ControlType enums to string representations using Qt reflection
    QMetaEnum controlEnum = QMetaEnum::fromType<FunctionObject::ControlType>();
    QString execControl =
        controlEnum.valueToKey(static_cast<int>(config->executeControl));
    QString writeControl =
        controlEnum.valueToKey(static_cast<int>(config->writeControl));

    // Write dictionary header directly to the parent stream
    stream << "    " << config->name << "\n"
           << "    {\n"
           << "        type            forceCoeffs;\n"
           << "        libs            (\"libforces.so\");\n\n";

    // Write base FunctionObject parameters
    stream << "        active          " << boolToStr(config->active) << ";\n"
           << "        executeControl  " << execControl << ";\n"
           << "        executeInterval " << config->executeInterval << ";\n"
           << "        writeControl    " << writeControl << ";\n"
           << "        writeInterval   " << config->writeInterval << ";\n"
           << "        log             " <<
        boolToStr(config->logOutput) << ";\n\n";

    // Write ForceCoeffsConfig specific parameters
    stream << "        writeFields     " <<
        boolToStr(config->writeFields) << ";\n";

    // Format the patch list
    stream << "        patches         (";
    for (int i = 0; i < config->patches.size(); ++i) {
        stream << config->patches[i];
        if (i < config->patches.size() - 1) {
            stream << " ";
        }
    }
    stream << ");\n\n";

    // Output physical and reference fields
    stream << "        p               " << config->pName << ";\n"
           << "        U               " << config->UName << ";\n"
           << "        rho             " << config->rhoName << ";\n";
    if (config->rhoName == "rhoInf") {
        stream << "        rhoInf          " << config->rhoInf << ";\n";
    }
    stream << "        pRef            " << config->pRef << ";\n"
           << "        magUInf         " << config->magUInf << ";\n"
           << "        lRef            " << config->lRef << ";\n"
           << "        Aref            " << config->aRef << ";\n"
           << "        porosity        " <<
        boolToStr(config->includePorosity) << ";\n\n";

    // Format arrays as: (x y z)
    stream << "        CofR            ("
           << config->centerOfRotation[0] << " "
           << config->centerOfRotation[1] << " "
           << config->centerOfRotation[2] << ");\n";

    stream << "        liftDir         ("
           << config->liftDir[0] << " "
           << config->liftDir[1] << " "
           << config->liftDir[2] << ");\n";

    stream << "        dragDir         ("
           << config->dragDir[0] << " "
           << config->dragDir[1] << " "
           << config->dragDir[2] << ");\n";

    stream << "        pitchAxis       ("
           << config->pitchAxis[0] << " "
           << config->pitchAxis[1] << " "
           << config->pitchAxis[2] << ");\n";

    // Close the dictionary block
    stream << "    }\n";
}

// Create text for fieldMinMax function object
void CaseIO::createFieldMinMaxBlock(QTextStream& stream,
                                    const FieldMinMaxConfig* config) {
    if (!config)
        return;

    // Lambda to convert boolean values to OpenFOAM "true"/"false" syntax
    auto boolToStr = [](bool val) { return val ? "true" : "false"; };

    // Map base ControlType enums to string representations using Qt reflection
    QMetaEnum controlEnum = QMetaEnum::fromType<FunctionObject::ControlType>();
    QString execControl =
        controlEnum.valueToKey(static_cast<int>(config->executeControl));
    QString writeControl =
        controlEnum.valueToKey(static_cast<int>(config->writeControl));

    // Map Mode enum to string and convert to lowercase for OpenFOAM syntax
    QMetaEnum modeEnum = QMetaEnum::fromType<FieldMinMaxConfig::Mode>();
    QString modeStr = modeEnum.valueToKey(static_cast<int>(config->mode));
    modeStr = modeStr.toLower();

    // Write dictionary header
    stream << "    " << config->name << "\n"
           << "    {\n"
           << "        type            fieldMinMax;\n"
           << "        libs            (\"libfieldFunctionObjects.so\");\n\n";

    // Write base FunctionObject parameters
    stream << "        active          " << boolToStr(config->active) << ";\n"
           << "        executeControl  " << execControl << ";\n"
           << "        executeInterval " << config->executeInterval << ";\n"
           << "        writeControl    " << writeControl << ";\n"
           << "        writeInterval   " << config->writeInterval << ";\n"
           << "        log             " <<
        boolToStr(config->logOutput) << ";\n\n";

    // Write FieldMinMaxConfig specific parameters
    stream << "        mode            " << modeStr << ";\n"
           << "        location        " <<
        boolToStr(config->location) << ";\n\n";

    // Format the fields list as: (field1 field2 ...)
    stream << "        fields          (";
    for (int i = 0; i < config->fields.size(); ++i) {
        stream << config->fields[i];
        if (i < config->fields.size() - 1) {
            stream << " ";
        }
    }
    stream << ");\n";

    // Close the dictionary block
    stream << "    }\n";
}

// Create text for probes function object
void CaseIO::createProbesBlock(QTextStream& stream, const ProbesConfig* config) {
    if (!config)
        return;

    // Lambda to convert boolean values to OpenFOAM "true"/"false" syntax
    auto boolToStr = [](bool val) { return val ? "true" : "false"; };

    // Map base ControlType enums to string representations using Qt reflection
    QMetaEnum controlEnum =
        QMetaEnum::fromType<FunctionObject::ControlType>();
    QString execControl =
        controlEnum.valueToKey(static_cast<int>(config->executeControl));
    QString writeControl =
        controlEnum.valueToKey(static_cast<int>(config->writeControl));

    // Map InterpolationType enum to string
    QMetaEnum interpEnum =
        QMetaEnum::fromType<FunctionObject::InterpolationType>();
    QString interpScheme =
        interpEnum.valueToKey(static_cast<int>(config->interpolationScheme));

    // Write dictionary header
    stream << "    " << config->name << "\n"
           << "    {\n"
           << "        type            probes;\n"
           << "        libs            (\"libsampling.so\");\n\n";

    // Write base FunctionObject parameters
    stream << "        active          " << boolToStr(config->active) << ";\n"
           << "        executeControl  " << execControl << ";\n"
           << "        executeInterval " << config->executeInterval << ";\n"
           << "        writeControl    " << writeControl << ";\n"
           << "        writeInterval   " << config->writeInterval << ";\n"
           << "        log             " <<
        boolToStr(config->logOutput) << ";\n\n";

    // Write ProbesConfig specific parameters
    stream << "        interpolationScheme " << interpScheme << ";\n"
           << "        fixedLocations      " <<
        boolToStr(config->fixedLocations) << ";\n"
           << "        includeOutOfBounds  " <<
        boolToStr(config->includeOutOfBounds) << ";\n"
           << "        verbose             " <<
        boolToStr(config->verbose) << ";\n"
           << "        sampleOnExecute     " <<
        boolToStr(config->sampleOnExecute) << ";\n\n";

    // Format the fields list as: (field1 field2 ...)
    stream << "        fields              (";
    for (int i = 0; i < config->fields.size(); ++i) {
        stream << config->fields[i];
        if (i < config->fields.size() - 1) {
            stream << " ";
        }
    }
    stream << ");\n\n";

    // Format probeLocations as a list of coordinate vectors
    stream << "        probeLocations\n"
           << "        (\n";
    for (const auto& loc : config->probeLocations) {
        stream << "            (" <<
            loc.x() << " " << loc.y() << " " << loc.z() << ")\n";
    }
    stream << "        );\n";

    // Close the dictionary block
    stream << "    }\n";
}

// Create text for surfaces function object
void CaseIO::createSurfacesBlock(QTextStream& stream,
                                 const SurfacesConfig* config) {
    if (!config)
        return;

    // Convert boolean values to "true"/"false"
    auto boolToStr = [](bool val) { return val ? "true" : "false"; };

    // Map base ControlType enums to string representations using Qt reflection
    QMetaEnum controlEnum = QMetaEnum::fromType<FunctionObject::ControlType>();
    QString execControl =
        controlEnum.valueToKey(static_cast<int>(config->executeControl));
    QString writeControl =
        controlEnum.valueToKey(static_cast<int>(config->writeControl));

    // Map SurfacesConfig enums to strings
    QMetaEnum formatEnum = QMetaEnum::fromType<SurfacesConfig::SurfaceFormat>();
    QString surfFormat =
        formatEnum.valueToKey(static_cast<int>(config->surfaceFormat));

    QMetaEnum interpEnum =
        QMetaEnum::fromType<FunctionObject::InterpolationType>();
    QString interpScheme =
        interpEnum.valueToKey(static_cast<int>(config->interpolationScheme));

    // Write dictionary header
    stream << "    " << config->name << "\n"
           << "    {\n"
           << "        type            surfaces;\n"
           << "        libs            (\"libsampling.so\");\n\n";

    // Write base FunctionObject parameters
    stream << "        active          " << boolToStr(config->active) << ";\n"
           << "        executeControl  " << execControl << ";\n"
           << "        executeInterval " << config->executeInterval << ";\n"
           << "        writeControl    " << writeControl << ";\n"
           << "        writeInterval   " << config->writeInterval << ";\n"
           << "        log             " <<
        boolToStr(config->logOutput) << ";\n\n";

        // Write SurfacesConfig specific parameters
        stream << "        surfaceFormat   " << surfFormat << ";\n"
        << "        interpolationScheme " << interpScheme << ";\n\n";

        // Format the fields list as: (field1 field2 ...)
        stream << "        fields          (";
    for (int i = 0; i < config->fields.size(); ++i) {
            stream << config->fields[i];
            if (i < config->fields.size() - 1) {
                stream << " ";
        }
    }
    stream << ");\n\n";

    // Format the surfaces sub-dictionary
    stream << "        surfaces\n"
           << "        {\n";

    QMetaEnum surfaceTypeEnum = QMetaEnum::fromType<SurfaceDef::SurfaceType>();

    for (const auto& surface : config->surfaces) {
            QString surfType =
                surfaceTypeEnum.valueToKey(static_cast<int>(surface.type));

            stream << "            " << surface.name << "\n"
            << "            {\n"
            << "                type            " << surfType << ";\n";

            // Output custom parameters
            for (const auto& [key, value] : surface.parameters) {
                stream << "                " << key << " " << value << ";\n";
        }
        stream << "            }\n";
    }

    stream << "        }\n";

    // Close the parent dictionary block
    stream << "    }\n";
}

// Create text for yPlus function object
void CaseIO::createYPlusBlock(QTextStream& stream, const YPlusConfig* config) {
    if (!config) {
        return;
    }

    // Convert boolean values to "true"/"false"
    auto boolToStr = [](bool val) { return val ? "true" : "false"; };

    // Map base ControlType enums to string representations using Qt reflection
    QMetaEnum controlEnum =
        QMetaEnum::fromType<FunctionObject::ControlType>();
    QString execControl =
        controlEnum.valueToKey(static_cast<int>(config->executeControl));
    QString writeControl =
        controlEnum.valueToKey(static_cast<int>(config->writeControl));

    // Write dictionary header
    stream << "    " << config->name << "\n"
           << "    {\n"
           << "        type            yPlus;\n"
           << "        libs            (\"libfieldFunctionObjects.so\");\n\n";

    // Write base FunctionObject parameters
    stream << "        active          " << boolToStr(config->active) << ";\n"
           << "        executeControl  " << execControl << ";\n"
           << "        executeInterval " << config->executeInterval << ";\n"
           << "        writeControl    " << writeControl << ";\n"
           << "        writeInterval   " << config->writeInterval << ";\n"
           << "        log             " <<
        boolToStr(config->logOutput) << ";\n\n";

    // Write YPlusConfig specific parameters
    stream << "        patches         (";
    for (int i = 0; i < config->patches.size(); ++i) {
        stream << config->patches[i];
        if (i < config->patches.size() - 1) {
            stream << " ";
        }
    }
    stream << ");\n";

    // Close the dictionary block
    stream << "    }\n";
}
