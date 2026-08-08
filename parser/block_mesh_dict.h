#ifndef PARSER_BLOCK_MESH_DICT_H_
#define PARSER_BLOCK_MESH_DICT_H_

#include <QDebug>
#include <QRegularExpression>

#include <memory>

#include "open_foam_dictionary.h"
#include "common.h"

namespace CaseIO {

struct Patch {
    QString name;
    PatchType type;
    std::vector<std::array<int, 4>> faces;
};

// Store blockMeshDict data
struct BlockMeshConfig {
    double convertToMeters = 1.0;
    std::vector<std::array<double, 3>> vertices;
    QString shape = "hex";
    int nX = 40, nY = 40, nZ = 40;
    double gradingX = 1.0, gradingY = 1.0, gradingZ = 1.0;
    std::vector<Patch> patches;
};

// Parse blockMeshDict
BlockMeshConfig parseBlockMeshDict(std::shared_ptr<OpenFoamDictionary> dict);

// Update existing blockMeshDict
QString updateBlockMeshDict(std::shared_ptr<OpenFoamDictionary> dict,
                            const BlockMeshConfig& config);

// Create new blockMeshDict
QString createBlockMeshDict(const BlockMeshConfig& config,
                            QString openFoamPath);
};

#endif  // PARSER_BLOCK_MESH_DICT_H_
