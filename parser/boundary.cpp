#include <QByteArray>
#include <QDebug>
#include <QRegularExpression>
#include <QString>

#include "boundary.h"

// Parse the boundary file into a vector of patches
std::vector<CaseIO::MeshPatch> CaseIO::parseBoundary(
    const QByteArray& fileData) {
    std::vector<CaseIO::MeshPatch> patches;

    // Convert the raw byte array from the WSL socket into a QString.
    QString text = QString::fromUtf8(fileData);

    // Remove comments
    text.replace(QRegularExpression("/\\*.*?\\*/",
        QRegularExpression::DotMatchesEverythingOption), "");
    text.replace(QRegularExpression("//.*"), "");

    // Look through top-level parentheses
    int startIdx = text.indexOf('(');
    int endIdx = text.lastIndexOf(')');
    if (startIdx == -1 || endIdx == -1 || startIdx >= endIdx) {
        return patches;
    }

    QString listContent = text.mid(startIdx + 1, endIdx - startIdx - 1);

    // Step 1: Regex to capture the patch name and everything in {}
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
            CaseIO::MeshPatch bp;
            bp.name = match.captured(1);

            QRegularExpressionMatch typeMatch = reType.match(blockContent);
            bp.type = (typeMatch.hasMatch()) ? typeMatch.captured(1) : "patch";

            patches.push_back(bp);
        }
    }
    return patches;
}

// Get patch names
QStringList CaseIO::getPatches(const QByteArray& fileData) {
    QStringList patchNames;

    // Convert the raw byte array into a QString
    QString text = QString::fromUtf8(fileData);

    // Remove comments
    text.replace(QRegularExpression("/\\*.*?\\*/",
                                    QRegularExpression::DotMatchesEverythingOption), "");
    text.replace(QRegularExpression("//.*"), "");

    // Look through top-level parentheses
    int startIdx = text.indexOf('(');
    int endIdx = text.lastIndexOf(')');
    if (startIdx == -1 || endIdx == -1 || startIdx >= endIdx) {
        return patchNames;
    }

    QString listContent = text.mid(startIdx + 1, endIdx - startIdx - 1);

    // Simplified Regex: Capture only the patch name preceding a '{'
    QRegularExpression reName("([A-Za-z0-9_\\-]+)\\s*\\{");
    QRegularExpressionMatchIterator i = reName.globalMatch(listContent);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        patchNames.append(match.captured(1)); // Add the extracted name to the list
    }

    return patchNames;
}

// Update boundary file
QString CaseIO::updateBoundaryFile(std::shared_ptr<OpenFoamDictionary> dict,
                            const std::vector<CaseIO::MeshPatch>& filtered) {

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

CaseIO::BoundaryFileParts CaseIO::splitBoundaryFile(const QByteArray& rawData) {
    CaseIO::BoundaryFileParts parts;

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

QByteArray CaseIO::updateHeaderCount(const QByteArray& header,
                                     int removedCount) {
    QString headerStr = QString::fromUtf8(header);

    // Look for the last integer before any trailing whitespace
    QRegularExpression re("(\\d+)(\\s*)$");
    QRegularExpressionMatch match = re.match(headerStr);

    if (match.hasMatch()) {
        int currentCount = match.captured(1).toInt();
        int newCount = std::max(0, currentCount - removedCount);

        // Replace the old count with the new count
        QString replacement = QString::number(newCount) + match.captured(2);
        headerStr.replace(match.capturedStart(0),
                          match.capturedLength(0), replacement);
    }

    return headerStr.toUtf8();
}

QByteArray CaseIO::removeEmptyPatches(const QByteArray& boundaryData) {

    // Split the file
    CaseIO::BoundaryFileParts parts = splitBoundaryFile(boundaryData);
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
