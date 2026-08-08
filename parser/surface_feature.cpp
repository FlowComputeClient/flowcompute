#include "surface_feature.h"

#include <QDebug>
#include <QDir>
#include <QRegularExpression>

#include "common.h"

// Parse surface feature extraction data
std::map<QString, CaseIO::SurfaceFeatureEntry>
CaseIO::parseSurfaceFeatureData(
    const std::shared_ptr<OpenFoamDictionary> dict,
    const QStringList& geometryFiles) {

    // Clear existing map to ensure idempotence
    std::map<QString, CaseIO::SurfaceFeatureEntry> surfaceFeatureMap;
    if ((!dict) || (geometryFiles.empty())) return surfaceFeatureMap;

    // Helper lambda to safely parse OpenFOAM boolean syntax
    auto parseOfBool = [](QString val, bool defaultVal) -> bool {
        if (val.isEmpty()) return defaultVal;
        val = val.toLower().trimmed();
        if (val == "yes" || val == "true" || val == "on") return true;
        if (val == "no" || val == "false" || val == "off") return false;
        return defaultVal;
    };

    // Iterate through geometry files
    for (const QString& stlName : std::as_const(geometryFiles)) {

        // Initialize with safe OpenFOAM defaults
        CaseIO::SurfaceFeatureEntry entry;

        // Parse Booleans
        QString writeObjStr = dict->getString(stlName + "/writeObj");
        entry.writeObj = parseOfBool(writeObjStr, true);

        // QString nonManifoldStr = dict->getString(stlName + "/nonManifoldEdges");
        // entry.nonManifoldEdges = parseOfBool(nonManifoldStr, false);

        QString openEdgesStr = dict->getString(stlName + "/openEdges");
        entry.openEdges = parseOfBool(openEdgesStr, true);

        // --- 2. Parse Included Angle using the new path syntax ---
        // Look inside the standard OpenFOAM sub-dictionary first
        double angle = dict->getNumber(stlName +
                "/extractFromSurfaceCoeffs/includedAngle");

        if (!std::isnan(angle)) {
            entry.angle = angle;
        } else {
            // Check if the user placed it at the top level of the STL block
            angle = dict->getNumber(stlName + "/includedAngle");
            if (!std::isnan(angle)) {
                entry.angle = angle;
            }
        }
        surfaceFeatureMap[stlName] = entry;
    }
    return surfaceFeatureMap;
}

// Update surfaceFeatureExtractDict or surfaceFeaturesDict
QString CaseIO::updateSurfaceFeatureDict(
    std::shared_ptr<OpenFoamDictionary> dict,
    const std::map<QString, SurfaceFeatureEntry>& entries) {

    if (!dict) {
        qWarning() << "Cannot update surfaceFeatureExtractDict.";
        return QString();
    }

    for (const auto& [fileName, config] : entries) {

        // Build the configuration block for this specific surface
        QString blockStr;
        QTextStream out(&blockStr);

        // Use standard OpenFOAM boolean syntax (yes/no)
        QString writeObjStr = config.writeObj ? "yes" : "no";
        // QString nonManifoldStr = config.nonManifoldEdges ? "yes" : "no";
        QString openEdgesStr = config.openEdges ? "yes" : "no";

        out << "\n"
            << "    {\n"
            << "        extractionMethod    extractFromSurface;\n"
            << "        extractFromSurfaceCoeffs\n"
            << "        {\n"
            << "            includedAngle   " << config.angle << ";\n"
            << "        }\n"
            << "        writeObj            " << writeObjStr << ";\n"
            // << "        nonManifoldEdges    " << nonManifoldStr << ";\n"
            << "        openEdges           " << openEdgesStr << ";\n"
            << "    }";

        // Inject the updated block into AST
        dict->setValue(fileName, blockStr);
    }

    return dict->getRawText();
}

QString CaseIO::createSurfaceFeatureDict(
    const std::map<QString, SurfaceFeatureEntry>& entryMap, QString ofPath) {

    QString dictStr;
    QTextStream out(&dictStr);

    // Determine which OpenFOAM release is used
    QString dirName = QDir(ofPath).dirName();
    const QRegularExpression foundationRegex("^openfoam\\d{2}$",
        QRegularExpression::CaseInsensitiveOption);
    bool isFoundation = foundationRegex.match(dirName).hasMatch();

    // Generate the standard OpenFOAM header
    if (isFoundation) {
        out << createFoamHeader("surfaceFeaturesDict", ofPath);

        // Generate a block for each file (Foundation version)
        for (const auto& [fileName, config] : entryMap) {
            // Remove the extension for the block identifier
            QString identifier = QFileInfo(fileName).baseName();
            if (identifier.isEmpty()) {
                identifier = fileName;
            }

            QString writeObjStr = config.writeObj ? "yes" : "no";

            out << identifier << "\n"
                << "{\n"
                << "    surfaces\n"
                << "    (\n"
                << "        \"" << fileName << "\"\n"
                << "    );\n"
                << "    includedAngle           " << config.angle << ";\n"
                << "    geometricTestOnly       yes;\n"
                << "    writeObj                " << writeObjStr << ";\n"
                << "}\n\n";
        }
    } else {
        out << createFoamHeader("surfaceFeatureExtractDict", ofPath);

        // Generate a block for each file
        for (const auto& [fileName, config] : entryMap) {

            // Write the block for the given file
            QString writeObjStr = config.writeObj ? "yes" : "no";
            QString openEdgesStr = config.openEdges ? "yes" : "no";
            out << fileName << "\n"
                << "{\n"
                << "    extractionMethod    extractFromSurface;\n"
                << "    extractFromSurfaceCoeffs\n"
                << "    {\n"
                << "        includedAngle   " << config.angle << ";\n"
                << "    }\n"
                << "    writeObj            " << writeObjStr << ";\n"
                << "    nonManifoldEdges    yes;\n"
                << "    openEdges           " << openEdgesStr << ";\n"
                << "}\n\n";
        }
    }
    out << createFoamFooter();
    return dictStr;
}
