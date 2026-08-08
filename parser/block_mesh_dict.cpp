#include "block_mesh_dict.h"

// Parse the block mesh file into a BlockMeshConfig structure
CaseIO::BlockMeshConfig
CaseIO::parseBlockMeshDict(std::shared_ptr<OpenFoamDictionary> dict) {
    CaseIO::BlockMeshConfig config;

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
    QString vertexPattern =
        QString("\\(\\s*(%1)\\s+(%2)\\s+(%3)\\s*\\)").arg(numPat,
                                                          numPat, numPat);
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
    QRegularExpression patchRe(
        "([a-zA-Z0-9_]+)\\s*\\{\\s*type\\s+([a-zA-Z]+);\\s*faces\\s*"
        "\\((.*?)\\);\\s*\\}",
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
        QRegularExpression
            faceRe("\\(\\s*(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s*\\)");
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

// Update blockMeshDict
QString CaseIO::updateBlockMeshDict(std::shared_ptr<OpenFoamDictionary> dict,
                                    const BlockMeshConfig& config) {
    if (!dict) {
        qWarning() << "Cannot update blockMeshDict: Dictionary is null.";
        return QString();
    }

    // Update scaling factor (handling ESI/Keysight vs Foundation syntax)
    if (!std::isnan(dict->getNumber("convertToMeters"))) {
        dict->setValue("convertToMeters",
                       QString::number(config.convertToMeters));
    }
    else if (!std::isnan(dict->getNumber("scale"))) {
        dict->setValue("scale", QString::number(config.convertToMeters));
    }
    else {
        qWarning() << "Warning: Neither 'convertToMeters' nor 'scale' found."
                      "Scaling not updated.";
    }

    // Update vertices
    QString vertsStr;
    QTextStream vertsOut(&vertsStr);
    vertsOut << "(\n";
    for (const auto& pt : config.vertices) {
        vertsOut <<
            QString("    (%1 %2 %3)\n").arg(pt[0]).arg(pt[1]).arg(pt[2]);
    }
    vertsOut << ")";

    dict->setValue("vertices", vertsStr);

    // Update blocks
    QString blocksStr;
    QTextStream blocksOut(&blocksStr);
    blocksOut << "(\n";
    blocksOut <<
        QString("    %1 (0 1 2 3 4 5 6 7) (%2 %3 %4) simpleGrading (%5 %6 %7)\n")
            .arg(config.shape)
            .arg(config.nX).arg(config.nY).arg(config.nZ)
            .arg(config.gradingX).arg(config.gradingY).arg(config.gradingZ);
    blocksOut << ")";

    dict->setValue("blocks", blocksStr);

    // Update boundary (Patches)
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
                boundOut << QString("            (%1 %2 %3 %4)\n")
                .arg(face[0]).arg(face[1]).arg(face[2]).arg(face[3]);
            }
            boundOut << "        );\n";
            boundOut << "    }\n";
        }
        boundOut << ")";

        // OpenFOAM sometimes uses "boundary" and sometimes "patches".
        dict->setValue("boundary", boundaryStr);
    }

    // Return the updated AST as a raw text string
    return dict->getRawText();
}

// Create a new blockMeshDict file
QString CaseIO::createBlockMeshDict(const BlockMeshConfig& config,
                                    QString openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << CaseIO::createFoamHeader("blockMeshDict", openFoamPath);

    // Write the scale factor
    bool isESI = false;
    QRegularExpression re("openfoam-?v?(\\d+)",
                          QRegularExpression::CaseInsensitiveOption);
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
        qWarning() << "Warning: Could not parse OpenFOAM version from path:"
                   << openFoamPath;
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
    out << QString("    %1 (0 1 2 3 4 5 6 7) (%2 %3 %4)"
                   " simpleGrading (%5 %6 %7)\n")
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
            qWarning() << "Warning: Unknown patch type for" << patch.name <<
                "- defaulting to 'patch'.";
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
