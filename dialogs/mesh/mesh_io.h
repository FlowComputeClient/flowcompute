#ifndef MESH_IO_H
#define MESH_IO_H

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include "../../parser/open_foam_dictionary.h"
#include "mesh_structs.h"

namespace MeshIO {

    // Parse functions
    BlockMeshConfig parseBlockMesh(std::shared_ptr<OpenFoamDictionary> dict);
    std::map<QString, SurfaceFeatureExtractEntry> parseSurfaceFeatureData(
            const std::shared_ptr<OpenFoamDictionary> dict, const QStringList& geometryFiles);
    CastellatedMeshConfig parseCastellatedMesh(const std::shared_ptr<OpenFoamDictionary> dict);
    SnapControlConfig parseSnapControlConfig(const std::shared_ptr<OpenFoamDictionary> dict);
    LayerControlConfig parseLayerControlConfig(const std::shared_ptr<OpenFoamDictionary> dict);

    // Update functions
    QString updateBlockMeshDict(std::shared_ptr<OpenFoamDictionary> dict, const BlockMeshConfig& config);
    QString createBlockMeshDict(const BlockMeshConfig& config, QString openFoamPath);
    QString updateSurfaceFeatureExtractDict(std::shared_ptr<OpenFoamDictionary> dict,
                                            const std::map<QString, SurfaceFeatureExtractEntry>& entryMap);
    QString createSurfaceFeatureExtractDict(const std::map<QString, SurfaceFeatureExtractEntry>& entryMap,
                                            QString openFoamPath);
    QString updateSnappyHexMeshDict(std::shared_ptr<OpenFoamDictionary> dict,
        const CastellatedMeshConfig&, const SnapControlConfig&, const LayerControlConfig&);
    QString createSnappyHexMeshDict(const CastellatedMeshConfig&, const SnapControlConfig&,
        const LayerControlConfig&, QString openFoamPath);

};

#endif // MESH_IO_H
