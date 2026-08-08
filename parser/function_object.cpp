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

// Create overall "functions" block
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
           << "        rho             " << config->rhoName << ";\n"
           << "        rhoInf          " << config->rhoInf << ";\n"
           << "        pRef            " << config->pRef << ";\n"
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
           << "        rho             " << config->rhoName << ";\n"
           << "        rhoInf          " << config->rhoInf << ";\n"
           << "        pRef            " << config->pRef << ";\n"
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
