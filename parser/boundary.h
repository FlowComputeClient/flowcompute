#ifndef PARSER_BOUNDARY_H_
#define PARSER_BOUNDARY_H_

#include <memory>

#include "open_foam_dictionary.h"
#include "common.h"

namespace CaseIO {

struct BoundaryFileParts {
    QByteArray header;
    QByteArray payload;
    QByteArray footer;
};

// Parse boundary file
std::vector<MeshPatch> parseBoundary(const QByteArray& fileData);
QStringList getPatches(const QByteArray& fileData);

// Update boundary file
QString updateBoundaryFile(std::shared_ptr<OpenFoamDictionary> dict,
    const std::vector<MeshPatch>& filtered);

BoundaryFileParts splitBoundaryFile(const QByteArray& rawData);
QByteArray updateHeaderCount(const QByteArray& header, int removedCount);
QByteArray removeEmptyPatches(const QByteArray& boundaryData);
};

#endif  // PARSER_BOUNDARY_H_
