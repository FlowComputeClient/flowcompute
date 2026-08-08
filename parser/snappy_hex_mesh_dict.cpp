#include "snappy_hex_mesh_dict.h"

#include <QDebug>
#include <QDir>
#include <QRegularExpression>

#include "common.h"

// Parse the block mesh file into a BlockMeshConfig structure
// Parse castellation mesh data
CaseIO::CastellatedMeshConfig CaseIO::parseCastellatedMesh(
    const std::shared_ptr<OpenFoamDictionary> dict) {

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
CaseIO::SnapControlConfig CaseIO::parseSnapControlConfig(
    const std::shared_ptr<OpenFoamDictionary> dict) {
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
    config.explicitFeatureSnap =
        readBool("snapControls/explicitFeatureSnap", true);
    config.implicitFeatureSnap =
        readBool("snapControls/implicitFeatureSnap", false);
    return config;
}

CaseIO::LayerControlConfig CaseIO::parseLayerControlConfig(
    const std::shared_ptr<OpenFoamDictionary> dict) {

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
    config.relativeSizes =
        readBool("addLayersControls/relativeSizes", true);
    config.expansionRatio =
        readDouble("addLayersControls/expansionRatio", 1.2);
    config.finalLayerThickness =
        readDouble("addLayersControls/finalLayerThickness", 0.3);
    config.minThickness =
        readDouble("addLayersControls/minThickness", 0.1);
    config.featureAngle =
        readDouble("addLayersControls/featureAngle", 130.0);
    config.nLayerIter =
        readInt("addLayersControls/nLayerIter", 50);
    config.nSmoothSurfaceNormals =
        readInt("addLayersControls/nSmoothSurfaceNormals", 1);

    // Parse the patch-specific layer counts
    QString layersPath = "addLayersControls/layers";
    QStringList patchNames = dict->getDictKeys(layersPath);

    for (const QString& patchName : std::as_const(patchNames)) {
        // Construct the full path to the nSurfaceLayers entry for this patch
        QString patchLayerPath = layersPath + "/" +
                                 patchName + "/nSurfaceLayers";

        double val = dict->getNumber(patchLayerPath);
        if (!std::isnan(val)) {
            // Only add to the map if the value was successfully parsed
            config.nSurfaceLayers[patchName] = static_cast<int>(val);
        }
    }
    return config;
}

QString CaseIO::updateSnappyHexMeshDict(
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

QString CaseIO::createSnappyHexMeshDict(
    const std::map<QString, SurfaceFeatureEntry>& entryMap,
    const CastellatedMeshConfig& castConfig,
    const SnapControlConfig& snapConfig, const LayerControlConfig& layerConfig,
    const QString& openFoamPath) {

    QString dictStr;
    QTextStream out(&dictStr);

    // Determine which OpenFOAM release is used
    QString dirName = QDir(openFoamPath).dirName();
    const QRegularExpression foundationRegex("^openfoam\\d{2}$",
                                             QRegularExpression::CaseInsensitiveOption);
    bool isFoundation = foundationRegex.match(dirName).hasMatch();

    // Helper lambda to convert booleans to OpenFOAM format
    auto boolToStr = [](bool b) { return b ? "true" : "false"; };

    // Write Header
    out << createFoamHeader("snappyHexMeshDict", openFoamPath);

    // Global switches
    out << "castellatedMesh    true;\n";
    out << "snap               true;\n";
    out << "addLayers          true;\n\n";

    // Geometry definition
    out << "geometry\n{\n";
    for (const auto& surface: castConfig.refinementSurfaces) {
        out << "    " << surface.name << "\n"
            << "    {\n"
            << "        type triSurfaceMesh;\n";
        if (isFoundation) {
            out << "        file \"" << surface.name << "\";\n";
        }
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
        SurfaceFeatureEntry entry = it->second;
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
