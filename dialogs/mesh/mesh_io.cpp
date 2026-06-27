#include "mesh_io.h"

#include "../../utils.h"

// Parse the block mesh file into a BlockMeshConfig structure
BlockMeshConfig MeshIO::parseBlockMesh(std::shared_ptr<OpenFoamDictionary> dict) {
    BlockMeshConfig config;

    // Scale
    double scale = dict->getNumber("scale");
    if (scale == 0.0) {
        scale = dict->getNumber("convertToMeters");
    }
    config.convertToMeters = (scale != 0.0) ? scale : 1.0;

    // Vertices
    QString verticesStr = dict->getString("vertices");
    QString numPat = "[-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?";

    // Build a pattern that looks for: ( x y z )
    QString vertexPattern = QString("\\(\\s*(%1)\\s+(%2)\\s+(%3)\\s*\\)").arg(numPat, numPat, numPat);
    QRegularExpression vertexRe(vertexPattern);
    QRegularExpressionMatchIterator vi = vertexRe.globalMatch(verticesStr);

    while (vi.hasNext()) {
        QRegularExpressionMatch match = vi.next();
        // captured(1), (2), and (3) correspond to x, y, and z
        double x = match.captured(1).toDouble();
        double y = match.captured(2).toDouble();
        double z = match.captured(3).toDouble();

        config.vertices.push_back({x, y, z});
    }

    // Blocks
    QString blocksStr = dict->getString("blocks");
    // QString numPat = "[-+]?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?";

    QString fullPattern = QString(
                              "hex\\s*\\([^)]+\\)\\s*"
                              "\\(\\s*(\\d+)\\s+(\\d+)\\s+(\\d+)\\s*\\)\\s*"
                              "[a-zA-Z]+\\s*"
                              "\\(\\s*(%1)\\s+(%2)\\s+(%3)\\s*\\)"
                              ).arg(numPat, numPat, numPat);

    QRegularExpression blockRe(fullPattern);
    QRegularExpressionMatch blockMatch = blockRe.match(blocksStr);

    if (blockMatch.hasMatch()) {
        config.shape = "hex";
        config.nX = blockMatch.captured(1).toInt();
        config.nY = blockMatch.captured(2).toInt();
        config.nZ = blockMatch.captured(3).toInt();
        config.gradingX = blockMatch.captured(4).toDouble();
        config.gradingY = blockMatch.captured(5).toDouble();
        config.gradingZ = blockMatch.captured(6).toDouble();
    }

    // Boundary patches
    QString boundaryStr = dict->getString("boundary");

    // Regex to capture the patch name (1), type (2), and the raw faces string block (3)
    // DotMatchesEverythingOption ensures we capture across multiple newlines in the faces block
    QRegularExpression patchRe(
        "([a-zA-Z0-9_]+)\\s*\\{\\s*type\\s+([a-zA-Z]+);\\s*faces\\s*\\((.*?)\\);\\s*\\}",
        QRegularExpression::DotMatchesEverythingOption
        );

    QRegularExpressionMatchIterator pi = patchRe.globalMatch(boundaryStr);

    while (pi.hasNext()) {
        QRegularExpressionMatch pMatch = pi.next();
        Patch patch;
        patch.name = pMatch.captured(1);
        QString typeStr = pMatch.captured(2);

        // Map the extracted string to your C++ Enum
        if (typeStr == "wall") patch.type = PatchType::wall;
        else if (typeStr == "symmetryPlane") patch.type = PatchType::symmetryPlane;
        else if (typeStr == "empty") patch.type = PatchType::empty;
        else if (typeStr == "wedge") patch.type = PatchType::wedge;
        else if (typeStr == "cyclic") patch.type = PatchType::cyclic;
        else patch.type = PatchType::patch; // Fallback default

        // Parse the inner faces string for this specific patch
        // Looks for sequences like: (0 1 5 4)
        QString facesStr = pMatch.captured(3);
        QRegularExpression faceRe("\\(\\s*(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s*\\)");
        QRegularExpressionMatchIterator fi = faceRe.globalMatch(facesStr);

        while (fi.hasNext()) {
            QRegularExpressionMatch fMatch = fi.next();
            std::array<int, 4> faceIndices = {
                fMatch.captured(1).toInt(),
                fMatch.captured(2).toInt(),
                fMatch.captured(3).toInt(),
                fMatch.captured(4).toInt()
            };
            patch.faces.push_back(faceIndices);
        }
        config.patches.push_back(patch);
    }
    return config;
}

// Parse surface feature extraction data
std::map<QString, SurfaceFeatureExtractEntry>
    MeshIO::parseSurfaceFeatureData(const std::shared_ptr<OpenFoamDictionary> dict,
                                const QStringList& geometryFiles) {

    // Clear existing map to ensure idempotence
    std::map<QString, SurfaceFeatureExtractEntry> surfaceFeatureMap;
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

        // Initialize with safe OpenFOAM defaults (150.0, false, true, true)
        SurfaceFeatureExtractEntry entry;

        // --- 1. Parse Booleans using the new path syntax ---
        // If the node is missing, getString returns an empty string,
        // which parseOfBool gracefully converts to our safe defaults.
        QString writeObjStr = dict->getString(stlName + "/writeObj");
        entry.writeObj = parseOfBool(writeObjStr, true);

        // QString nonManifoldStr = dict->getString(stlName + "/nonManifoldEdges");
        // entry.nonManifoldEdges = parseOfBool(nonManifoldStr, false);

        QString openEdgesStr = dict->getString(stlName + "/openEdges");
        entry.openEdges = parseOfBool(openEdgesStr, true);

        // --- 2. Parse Included Angle using the new path syntax ---
        // Look inside the standard OpenFOAM sub-dictionary first
        double angle = dict->getNumber(stlName + "/extractFromSurfaceCoeffs/includedAngle");

        if (!std::isnan(angle)) {
            entry.includedAngle = angle;
        } else {
            // Fallback: Check if the user placed it directly at the top level of the STL block
            angle = dict->getNumber(stlName + "/includedAngle");
            if (!std::isnan(angle)) {
                entry.includedAngle = angle;
            }
        }
        surfaceFeatureMap[stlName] = entry;
    }
    return surfaceFeatureMap;
}

// Parse castellation mesh data
CastellatedMeshConfig MeshIO::parseCastellatedMesh(const std::shared_ptr<OpenFoamDictionary> dict) {
    CastellatedMeshConfig config;

    /*
    if (!dict) return config;

    // Only overwrite the struct's default values if the key exists in the file
    auto parseInt = [&](const QString& path, int& target) {
        double val = dict->getNumber(path);
        if (!std::isnan(val)) {
            target = static_cast<int>(val);
        }
    };

    auto parseDouble = [&](const QString& path, double& target) {
        double val = dict->getNumber(path);
        if (!std::isnan(val)) {
            target = val;
        }
    };

    auto parseBool = [&](const QString& path, bool& target) {
        QString val = dict->getString(path).toLower();
        if (!val.isEmpty()) {
            target = (val == "true" || val == "on" || val == "yes" || val == "1");
        }
    };

    // --- 1. Scalar Controls ---
    parseInt("castellatedMeshControls/maxLocalCells", config.maxLocalCells);
    parseInt("castellatedMeshControls/maxGlobalCells", config.maxGlobalCells);
    parseInt("castellatedMeshControls/nCellsBetweenLevels", config.nCellsBetweenLevels);
    parseDouble("castellatedMeshControls/resolveFeatureAngle", config.resolveFeatureAngle);
    parseBool("castellatedMeshControls/allowFreeStandingZoneFaces", config.allowFreeStandingZoneFaces);

    // --- 2. locationInMesh (List of 3 doubles) ---
    QString locStr = dict->getString("castellatedMeshControls/locationInMesh");
    if (!locStr.isEmpty()) {
        // Strip parentheses and split by whitespace
        locStr.remove('(').remove(')');
        QStringList parts = locStr.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 3) {
            for (int i = 0; i < 3; ++i) {
                bool ok;
                double val = parts[i].toDouble(&ok);
                if (ok) config.locationInMesh[i] = val;
            }
        }
    }

    // --- 3. refinementSurfaces (Nested Dictionaries) ---
    QStringList surfaceNames = dict->getSubDictKeys("castellatedMeshControls/refinementSurfaces");

    for (const QString& surfName : std::as_const(surfaceNames)) {
        QString levelPath = "castellatedMeshControls/refinementSurfaces/" + surfName + "/level";
        QString levelStr = dict->getString(levelPath);

        if (!levelStr.isEmpty()) {
            levelStr.remove('(').remove(')');
            QStringList parts = levelStr.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

            if (parts.size() >= 2) {
                RefinementSurface ref;
                ref.min = parts[0].toInt();
                ref.max = parts[1].toInt();
                config.refinementSurfaces[surfName] = ref;
            }
        }
    }

    // --- 4. features (Edge Refinements List) ---
    // Extract the entire ( ... ) list as a raw string
    QString featuresStr = dict->getString("castellatedMeshControls/features");

    if (!featuresStr.isEmpty()) {
        // Regex looks for: file "ANY_NAME.eMesh"; level ANY_NUMBER;
        QRegularExpression re("file\\s+\"([^\"]+)\\.eMesh\"\\s*;\\s*level\\s+(\\d+)\\s*;");
        QRegularExpressionMatchIterator i = re.globalMatch(featuresStr);

        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();

            // Extract "motor_housing" from "motor_housing.eMesh"
            QString baseName = match.captured(1);
            int edgeLevel = match.captured(2).toInt();

            // Reconstruct the STL filename to match our map keys
            QString stlName = baseName + ".stl";
            // config.refinements[stlName].edgeLevel = edgeLevel;
        }
    }
    */
    return config;
}

// Parse snap control mesh configuration data
SnapControlConfig MeshIO::parseSnapControlConfig(const std::shared_ptr<OpenFoamDictionary> dict) {
    SnapControlConfig config;

    // Helper lambda to safely read integers with a fallback
    auto readInt = [&](const QString& path, int defaultVal) -> int {
        double val = dict->getNumber(path);
        return std::isnan(val) ? defaultVal : static_cast<int>(val);
    };

    // Helper lambda to safely read doubles with a fallback
    auto readDouble = [&](const QString& path, double defaultVal) -> double {
        double val = dict->getNumber(path);
        return std::isnan(val) ? defaultVal : val;
    };

    // Helper lambda to safely read OpenFOAM booleans with a fallback
    auto readBool = [&](const QString& path, bool defaultVal) -> bool {
        QString str = dict->getString(path).trimmed().toLower();
        if (str == "true" || str == "yes" || str == "on") {
            return true;
        }
        if (str == "false" || str == "no" || str == "off") {
            return false;
        }
        return defaultVal;
    };

    // Parse the parameters, falling back to recommended defaults if missing
    config.nSmoothPatch = readInt("snapControls/nSmoothPatch", 3);
    config.tolerance = readDouble("snapControls/tolerance", 2.0);
    config.nSolveIter = readInt("snapControls/nSolveIter", 30);
    config.nRelaxIter = readInt("snapControls/nRelaxIter", 5);
    config.nFeatureSnapIter = readInt("snapControls/nFeatureSnapIter", 10);
    config.explicitFeatureSnap = readBool("snapControls/explicitFeatureSnap", true);
    config.implicitFeatureSnap = readBool("snapControls/implicitFeatureSnap", false);
    return config;
}

LayerControlConfig MeshIO::parseLayerControlConfig(const std::shared_ptr<OpenFoamDictionary> dict) {
    LayerControlConfig config;

    // Helper lambda to safely read booleans with a fallback
    auto readBool = [&](const QString& path, bool defaultVal) -> bool {
        QString str = dict->getString(path).trimmed().toLower();
        if (str == "true" || str == "yes" || str == "on") {
            return true;
        }
        if (str == "false" || str == "no" || str == "off") {
            return false;
        }
        return defaultVal;
    };

    // Helper lambda to safely read doubles with a fallback
    auto readDouble = [&](const QString& path, double defaultVal) -> double {
        double val = dict->getNumber(path);
        return std::isnan(val) ? defaultVal : val;
    };

    // Helper lambda to safely read integers with a fallback
    auto readInt = [&](const QString& path, int defaultVal) -> int {
        double val = dict->getNumber(path);
        return std::isnan(val) ? defaultVal : static_cast<int>(val);
    };

    // Parse the primary parameters under addLayersControls
    config.relativeSizes         = readBool("addLayersControls/relativeSizes", true);
    config.expansionRatio        = readDouble("addLayersControls/expansionRatio", 1.2);
    config.finalLayerThickness   = readDouble("addLayersControls/finalLayerThickness", 0.3);
    config.minThickness          = readDouble("addLayersControls/minThickness", 0.1);
    config.featureAngle          = readDouble("addLayersControls/featureAngle", 130.0);
    config.nLayerIter            = readInt("addLayersControls/nLayerIter", 50);
    config.nSmoothSurfaceNormals = readInt("addLayersControls/nSmoothSurfaceNormals", 1);

    // Parse the patch-specific layer counts
    // OpenFOAM structure: addLayersControls { layers { "patchName" { nSurfaceLayers 3; } } }
    QString layersPath = "addLayersControls/layers";
    QStringList patchNames = dict->getDictKeys(layersPath);

    for (const QString& patchName : std::as_const(patchNames)) {
        // Construct the full path to the nSurfaceLayers entry for this patch
        QString patchLayerPath = layersPath + "/" + patchName + "/nSurfaceLayers";

        double val = dict->getNumber(patchLayerPath);
        if (!std::isnan(val)) {
            // Only add to the map if the value was successfully parsed
            config.nSurfaceLayers[patchName] = static_cast<int>(val);
        }
    }

    return config;
}

// Update the blockMeshDict file
QString MeshIO::updateBlockMeshDict(std::shared_ptr<OpenFoamDictionary> dict, const BlockMeshConfig& config) {
    if (!dict) {
        qWarning() << "Cannot update blockMeshDict: Dictionary pointer is null.";
        return QString();
    }

    // 1. Update scaling factor (handling ESI/Keysight vs Foundation syntax)
    if (!std::isnan(dict->getNumber("convertToMeters"))) {
        dict->setValue("convertToMeters", QString::number(config.convertToMeters));
    }
    else if (!std::isnan(dict->getNumber("scale"))) {
        dict->setValue("scale", QString::number(config.convertToMeters));
    }
    else {
        qWarning() << "Warning: Neither 'convertToMeters' nor 'scale' found in blockMeshDict. Scaling not updated.";
    }

    // 2. Update vertices
    // Build the formatted OpenFOAM list string
    QString vertsStr;
    QTextStream vertsOut(&vertsStr);
    vertsOut << "(\n";
    for (const auto& pt : config.vertices) {
        vertsOut << QString("    (%1 %2 %3)\n").arg(pt[0]).arg(pt[1]).arg(pt[2]);
    }
    vertsOut << ")";

    dict->setValue("vertices", vertsStr);

    // 3. Update blocks
    // Assuming a standard single-block setup where the vertices are strictly ordered 0-7
    QString blocksStr;
    QTextStream blocksOut(&blocksStr);
    blocksOut << "(\n";
    blocksOut << QString("    %1 (0 1 2 3 4 5 6 7) (%2 %3 %4) simpleGrading (%5 %6 %7)\n")
                     .arg(config.shape)
                     .arg(config.nX).arg(config.nY).arg(config.nZ)
                     .arg(config.gradingX).arg(config.gradingY).arg(config.gradingZ);
    blocksOut << ")";

    dict->setValue("blocks", blocksStr);

    // 4. Update boundary (Patches)
    // You will need to adjust this to match your exact Patch struct definition
    if (!config.patches.empty()) {
        QString boundaryStr;
        QTextStream boundOut(&boundaryStr);
        boundOut << "(\n";

        for (const auto& patch : config.patches) {

            // Set string for patch type
            QString patchType;
            switch (patch.type) {
            case PatchType::patch:
                patchType = "patch"; break;
            case PatchType::wall:
                patchType = "wall"; break;
            case PatchType::symmetryPlane:
                patchType = "symmetryPlane"; break;
            case PatchType::empty:
                patchType = "empty"; break;
            case PatchType::wedge:
                patchType = "wedge"; break;
            case PatchType::cyclic:
                patchType = "cyclic"; break;
            default:
                patchType = "patch";
            }

            boundOut << "    " << patch.name << "\n";
            boundOut << "    {\n";
            boundOut << "        type " << patchType << ";\n";
            boundOut << "        faces\n";
            boundOut << "        (\n";
            for (const auto& face : patch.faces) {
                boundOut << QString("            (%1 %2 %3 %4)\n").arg(face[0]).arg(face[1]).arg(face[2]).arg(face[3]);
            }
            boundOut << "        );\n";
            boundOut << "    }\n";
        }
        boundOut << ")";

        // OpenFOAM sometimes uses "boundary" and sometimes "patches".
        // "boundary" is strictly correct for blockMeshDict.
        dict->setValue("boundary", boundaryStr);
    }

    // Return the updated AST as a raw text string
    return dict->getRawText();
}

// Create a new blockMeshDict file
QString MeshIO::createBlockMeshDict(const BlockMeshConfig& config, QString openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << Utils::createFoamHeader("blockMeshDict", openFoamPath);

    // Write the scale factor
    bool isESI = false;
    QRegularExpression re("openfoam-?v?(\\d+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(openFoamPath);
    if (match.hasMatch()) {
        QString digits = match.captured(1);
        int verNumber = digits.toInt();

        if (verNumber > 100) {
            out << "scale " << config.convertToMeters << ";\n\n";
        } else {
            out << "convertToMeters " << config.convertToMeters << ";\n\n";
        }
    } else {
        qWarning() << "Warning: Could not parse OpenFOAM version from path:" << openFoamPath;
    }

    // Write vertices
    out << "vertices\n";
    out << "(\n";
    for (const auto& pt : config.vertices) {
        out << QString("    (%1 %2 %3)\n").arg(pt[0]).arg(pt[1]).arg(pt[2]);
    }
    out << ");\n\n";

    // Write blocks
    out << "blocks\n";
    out << "(\n";
    out << QString("    %1 (0 1 2 3 4 5 6 7) (%2 %3 %4) simpleGrading (%5 %6 %7)\n")
               .arg(config.shape)
               .arg(config.nX).arg(config.nY).arg(config.nZ)
               .arg(config.gradingX).arg(config.gradingY).arg(config.gradingZ);
    out << ");\n\n";

    // Write edges
    out << "edges\n";
    out << "(\n";
    out << ");\n\n";

    // Write boundary patches
    out << "boundary\n";
    out << "(\n";
    for (const auto& patch : config.patches) {

        // Determine OpenFOAM patch type string
        QString patchType;
        switch (patch.type) {
        case PatchType::patch:
            patchType = "patch"; break;
        case PatchType::wall:
            patchType = "wall"; break;
        case PatchType::symmetryPlane:
            patchType = "symmetryPlane"; break;
        case PatchType::empty:
            patchType = "empty"; break;
        case PatchType::wedge:
            patchType = "wedge"; break;
        case PatchType::cyclic:
            patchType = "cyclic"; break;
        default:
            patchType = "patch"; // Safe fallback
            qWarning() << "Warning: Unknown patch type for" << patch.name << "- defaulting to 'patch'.";
            break;
        }

        out << "    " << patch.name << "\n";
        out << "    {\n";
        out << "        type " << patchType << ";\n";
        out << "        faces\n";
        out << "        (\n";
        for (const auto& face : patch.faces) {
            out << QString("            (%1 %2 %3 %4)\n")
            .arg(face[0]).arg(face[1]).arg(face[2]).arg(face[3]);
        }
        out << "        );\n";
        out << "    }\n";
    }
    out << ");\n\n";

    // 7. Write mergePatchPairs (Required empty list)
    out << "mergePatchPairs\n";
    out << "(\n";
    out << ");\n\n";

    // Write closing separator
    out << "// ************************************************************************* //\n";

    return dictStr;
}

// Update SurfaceFeatureExtractDict file
QString MeshIO::updateSurfaceFeatureExtractDict(
    std::shared_ptr<OpenFoamDictionary> dict,
    const std::map<QString, SurfaceFeatureExtractEntry>& entries) {

    if (!dict) {
        qWarning() << "Cannot update surfaceFeatureExtractDict: Dictionary pointer is null.";
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
            << "            includedAngle   " << config.includedAngle << ";\n"
            << "        }\n"
            << "        writeObj            " << writeObjStr << ";\n"
            // << "        nonManifoldEdges    " << nonManifoldStr << ";\n"
            << "        openEdges           " << openEdgesStr << ";\n"
            << "    }";

        // Inject the updated block into the AST
        // Note: If findNode fails because the user's original dictionary uses explicit
        // string quotes (e.g., "motorBike.stl" instead of motorBike.stl), you may need
        // to query dict->setValue("\"" + fileName + "\"", blockStr);
        dict->setValue(fileName, blockStr);
    }

    return dict->getRawText();
}

QString MeshIO::createSurfaceFeatureExtractDict(
    const std::map<QString, SurfaceFeatureExtractEntry>& entryMap, QString openFoamPath) {

    QString dictStr;
    QTextStream out(&dictStr);

    // Generate the standard OpenFOAM header
    out << Utils::createFoamHeader("surfaceFeatureExtractDict", openFoamPath);

    // Iterate through the map to generate the configuration blocks
    for (const auto& [fileName, config] : entryMap) {

        // Convert booleans to OpenFOAM's preferred yes/no format
        QString writeObjStr = config.writeObj ? "yes" : "no";
        // QString nonManifoldStr = config.nonManifoldEdges ? "yes" : "no";
        QString openEdgesStr = config.openEdges ? "yes" : "no";

        // Write the block, ensuring the filename is safely wrapped in quotes
        out << fileName << "\n"
            << "{\n"
            << "    extractionMethod    extractFromSurface;\n"
            << "    extractFromSurfaceCoeffs\n"
            << "    {\n"
            << "        includedAngle   " << config.includedAngle << ";\n"
            << "    }\n"
            << "    writeObj            " << writeObjStr << ";\n"
            // << "    nonManifoldEdges    " << nonManifoldStr << ";\n"
            << "    openEdges           " << openEdgesStr << ";\n"
            << "}\n\n";
    }

    // Write the standard OpenFOAM closing separator
    out << "// ************************************************************************* //\n";

    return dictStr;
}

QString MeshIO::updateSnappyHexMeshDict(
    std::shared_ptr<OpenFoamDictionary> dict,
    const CastellatedMeshConfig& castConfig,
    const SnapControlConfig& snapConfig,
    const LayerControlConfig& layerConfig) {

    /*
    if (!dict) {
        qWarning() << "Cannot update snappyHexMeshDict: Dictionary pointer is null.";
        return QString();
    }

    // Helper lambda to convert booleans to OpenFOAM format
    auto boolToStr = [](bool b) { return b ? "true" : "false"; };

    // ==========================================
    // 1. Castellated Mesh Controls
    // ==========================================
    dict->setValue("castellatedMeshControls/maxLocalCells", QString::number(castConfig.maxLocalCells));
    dict->setValue("castellatedMeshControls/maxGlobalCells", QString::number(castConfig.maxGlobalCells));
    dict->setValue("castellatedMeshControls/allowFreeStandingZoneFaces", boolToStr(castConfig.allowFreeStandingZoneFaces));
    dict->setValue("castellatedMeshControls/nCellsBetweenLevels", QString::number(castConfig.nCellsBetweenLevels));
    dict->setValue("castellatedMeshControls/resolveFeatureAngle", QString::number(castConfig.resolveFeatureAngle));

    // Safely update locationInMesh if it has been set
    if (!std::isnan(castConfig.locationInMesh[0])) {
        QString locStr = QString("(%1 %2 %3)")
        .arg(castConfig.locationInMesh[0])
            .arg(castConfig.locationInMesh[1])
            .arg(castConfig.locationInMesh[2]);
        dict->setValue("castellatedMeshControls/locationInMesh", locStr);
    }

    // Rebuild the refinementSurfaces and features blocks from scratch
    if (!castConfig.refinements.empty()) {
        QString refSurfStr, featuresStr;
        QTextStream refOut(&refSurfStr);
        QTextStream featOut(&featuresStr);

        refOut << "{\n";
        featOut << "(\n";

        for (const auto& [geom, ref] : castConfig.refinementSurfaces) {
            // Write to refinementSurfaces
            refOut << "        \"" << geom << "\"\n"
                   << "        {\n"
                   << "           level (" << ref.surfaceMin << " " << ref.surfaceMax << ");\n"
                   << "        }\n";

            // Predict the eMesh filename for the features block
            QString eMeshFile = geom;
            eMeshFile.replace(".stl", ".eMesh", Qt::CaseInsensitive);
            eMeshFile.replace(".obj", ".eMesh", Qt::CaseInsensitive);

            // Write to features
            featOut << "        {\n"
                    << "            file \"" << eMeshFile << "\";\n"
                    << "            level " << "3" << ";\n"
                    << "        }\n";
        }

        refOut << "    }";
        featOut << "    )";

        dict->setValue("castellatedMeshControls/refinementSurfaces", refSurfStr);
        dict->setValue("castellatedMeshControls/features", featuresStr);
    }

    // ==========================================
    // 2. Snap Controls
    // ==========================================
    dict->setValue("snapControls/nSmoothPatch", QString::number(snapConfig.nSmoothPatch));
    dict->setValue("snapControls/tolerance", QString::number(snapConfig.tolerance));
    dict->setValue("snapControls/nSolveIter", QString::number(snapConfig.nSolveIter));
    dict->setValue("snapControls/nRelaxIter", QString::number(snapConfig.nRelaxIter));
    dict->setValue("snapControls/nFeatureSnapIter", QString::number(snapConfig.nFeatureSnapIter));
    dict->setValue("snapControls/explicitFeatureSnap", boolToStr(snapConfig.explicitFeatureSnap));
    dict->setValue("snapControls/implicitFeatureSnap", boolToStr(snapConfig.implicitFeatureSnap));

    // ==========================================
    // 3. Layer Controls
    // ==========================================
    dict->setValue("addLayersControls/relativeSizes", boolToStr(layerConfig.relativeSizes));
    dict->setValue("addLayersControls/expansionRatio", QString::number(layerConfig.expansionRatio));
    dict->setValue("addLayersControls/finalLayerThickness", QString::number(layerConfig.finalLayerThickness));
    dict->setValue("addLayersControls/minThickness", QString::number(layerConfig.minThickness));
    dict->setValue("addLayersControls/featureAngle", QString::number(layerConfig.featureAngle));
    dict->setValue("addLayersControls/nLayerIter", QString::number(layerConfig.nLayerIter));
    dict->setValue("addLayersControls/nSmoothSurfaceNormals", QString::number(layerConfig.nSmoothSurfaceNormals));

    // Rebuild the layers block from scratch
    if (!layerConfig.nSurfaceLayers.empty()) {
        QString layersStr;
        QTextStream layersOut(&layersStr);

        layersOut << "{\n";
        for (const auto& [patch, nLayers] : layerConfig.nSurfaceLayers) {
            layersOut << "        \"" << patch << "\"\n"
                      << "        {\n"
                      << "            nSurfaceLayers " << nLayers << ";\n"
                      << "        }\n";
        }
        layersOut << "    }";

        dict->setValue("addLayersControls/layers", layersStr);
    }
    */
    return dict->getRawText();
}

QString MeshIO::createSnappyHexMeshDict(
    const std::map<QString, SurfaceFeatureExtractEntry>& entryMap,
    const CastellatedMeshConfig& castConfig,
    const SnapControlConfig& snapConfig,
    const LayerControlConfig& layerConfig,
    QString openFoamPath) {

    QString dictStr;
    QTextStream out(&dictStr);

    // Helper lambda to convert booleans to OpenFOAM format
    auto boolToStr = [](bool b) { return b ? "true" : "false"; };

    // Write Header
    out << Utils::createFoamHeader("snappyHexMeshDict", openFoamPath);

    // 2. Global Switches
    out << "castellatedMesh    true;\n";
    out << "snap               true;\n";
    out << "addLayers          true;\n\n";

    // 3. Geometry Definition Block
    out << "geometry\n{\n";
    for (const auto& surface: castConfig.refinementSurfaces) {
        out << "    " << surface.name << "\n"
            << "    {\n"
            << "        type triSurfaceMesh;\n";
        if (!surface.regions.empty()) {
            out << "        regions\n";
            out << "        {\n";
            for (auto const& region: surface.regions) {
                out << "            " << region.name << "\n";
                out << "            {\n";
                out << "                 name " << region.name << ";\n";
                out << "            }\n";
            }
            out << "        }\n";
        }
    }
    out << "    }\n";
    out << "}\n\n";

    // ==========================================
    // 4. Castellated Mesh Controls
    // ==========================================
    out << "castellatedMeshControls\n{\n";
    out << "    maxLocalCells " << castConfig.maxLocalCells << ";\n";
    out << "    maxGlobalCells " << castConfig.maxGlobalCells << ";\n";
    out << "    minRefinementCells 10;\n";
    out << "    maxLoadUnbalance 0.10;\n";
    out << "    nCellsBetweenLevels " << castConfig.nCellsBetweenLevels << ";\n\n";

    // Features block (eMesh Extrapolation)
    out << "    features\n    (\n";
    for (auto it = entryMap.begin(); it != entryMap.end(); ++it) {
        QString fileName = it->first;
        SurfaceFeatureExtractEntry entry = it->second;
        fileName.replace(".stl", ".eMesh", Qt::CaseInsensitive);
        fileName.replace(".obj", ".eMesh", Qt::CaseInsensitive);
        out << "        {\n"
            << "            file \"" << fileName << "\";\n"
            << "            level " << entry.edgeLevel << ";\n"
            << "        }\n";
    }
    out << "    );\n\n";

    // Refinement Surfaces block
    out << "    refinementSurfaces\n    {\n";
    for (auto const& surface: castConfig.refinementSurfaces) {

        out << "        " << surface.name << "\n"
            << "        {\n"
            << "            level (" << surface.min << " " << surface.max << ");\n";
        if (!surface.regions.empty()) {
            out << "            regions\n";
            out << "            {\n";
            for(auto const& region: surface.regions) {
                out << "                " << region.name << "\n";
                out << "                {\n";
                out << "                     level (" << region.min << " " << region.max << ");\n";
                out << "                }\n";
            }
            out << "            }\n";
        }
        out << "        }\n";
    }
    out << "    }\n\n";

    out << "    resolveFeatureAngle " << castConfig.resolveFeatureAngle << ";\n";
    out << "    refinementRegions {}\n";

    // Location in Mesh
    if (!std::isnan(castConfig.locationInMesh[0])) {
        out << QString("    locationInMesh (%1 %2 %3);\n")
        .arg(castConfig.locationInMesh[0])
        .arg(castConfig.locationInMesh[1])
        .arg(castConfig.locationInMesh[2]);
    } else {
        out << "    locationInMesh (0 0 0);\n";
    }

    out << "    allowFreeStandingZoneFaces " << boolToStr(castConfig.allowFreeStandingZoneFaces) << ";\n";
    out << "}\n\n";

    // ==========================================
    // 5. Snap Controls
    // ==========================================
    out << "snapControls\n{\n";
    out << "    nSmoothPatch " << snapConfig.nSmoothPatch << ";\n";
    out << "    tolerance " << snapConfig.tolerance << ";\n";
    out << "    nSolveIter " << snapConfig.nSolveIter << ";\n";
    out << "    nRelaxIter " << snapConfig.nRelaxIter << ";\n";
    out << "    nFeatureSnapIter " << snapConfig.nFeatureSnapIter << ";\n";
    out << "    explicitFeatureSnap " << boolToStr(snapConfig.explicitFeatureSnap) << ";\n";
    out << "    implicitFeatureSnap " << boolToStr(snapConfig.implicitFeatureSnap) << ";\n";
    out << "}\n\n";

    // ==========================================
    // 6. Add Layers Controls
    // ==========================================
    out << "addLayersControls\n{\n";
    out << "    relativeSizes " << boolToStr(layerConfig.relativeSizes) << ";\n";

    // Set surface layers
    out << "    layers\n    {\n";
    if (!layerConfig.nSurfaceLayers.empty()) {
        for (auto it = layerConfig.nSurfaceLayers.begin(); it != layerConfig.nSurfaceLayers.end(); ++it) {
            out << "        " << it->first << "\n"
                << "        {\n"
                << "            nSurfaceLayers " << it->second << ";\n"
                << "        }\n";
        }
    }
    out << "    }\n";
    out << "    expansionRatio " << layerConfig.expansionRatio << ";\n";
    out << "    finalLayerThickness " << layerConfig.finalLayerThickness << ";\n";
    out << "    minThickness " << layerConfig.minThickness << ";\n";
    out << "    featureAngle " << layerConfig.featureAngle << ";\n";
    out << "    nLayerIter " << layerConfig.nLayerIter << ";\n";
    out << "    nSmoothThickness " << layerConfig.nSmoothThickness << ";\n";
    out << "    nSmoothSurfaceNormals " << layerConfig.nSmoothSurfaceNormals << ";\n";
    out << "    nSmoothNormals " << layerConfig.nSmoothNormals << ";\n";
    out << "    nGrow 0;\n";
    out << "    nRelaxIter 5;\n";
    out << "    nAlphaIter 5;\n";
    out << "    maxFaceThicknessRatio 0.5;\n";
    out << "    maxThicknessToMedialRatio 0.3;\n";
    out << "    minMedialAxisAngle 90;\n";
    out << "    nBufferCellsNoExtrude 0;\n";
    out << "}\n\n";

    // ==========================================
    // 7. Mesh Quality Controls (Required Default Fallback)
    // ==========================================
    out << "meshQualityControls\n{\n";
    out << "    maxNonOrtho 65;\n";
    out << "    maxBoundarySkewness 20;\n";
    out << "    maxInternalSkewness 4;\n";
    out << "    maxConcave 80;\n";
    out << "    minVol 1e-13;\n";
    out << "    minTetQuality 1e-30;\n";
    out << "    minArea -1;\n";
    out << "    minTwist 0.02;\n";
    out << "    minDeterminant 0.001;\n";
    out << "    minFaceWeight 0.05;\n";
    out << "    minVolRatio 0.01;\n";
    out << "    minTriangleTwist -1;\n";
    out << "    nSmoothScale 4;\n";
    out << "    errorReduction 0.75;\n";
    out << "}\n\n";

    // 8. Final tolerance and closure
    out << "mergeTolerance 1e-6;\n\n";
    out << "// ************************************************************************* //\n";

    return dictStr;
}