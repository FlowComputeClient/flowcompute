#ifndef PARSER_SURFACE_FEATURE_H_
#define PARSER_SURFACE_FEATURE_H_

#include <map>
#include <memory>

#include "open_foam_dictionary.h"

namespace CaseIO {

struct SurfaceFeatureEntry {
    double angle = 150.0;
    int edgeLevel = 3;
    bool openEdges = true;
    bool writeObj = true;
};

// Parse surface feature file
std::map<QString, SurfaceFeatureEntry> parseSurfaceFeatureData(
    const std::shared_ptr<OpenFoamDictionary> dict,
    const QStringList& geometryFiles);

// Update existing surface feature file
QString updateSurfaceFeatureDict(
    std::shared_ptr<OpenFoamDictionary> dict,
    const std::map<QString, SurfaceFeatureEntry>& entryMap);

// Create new surface feature file
QString createSurfaceFeatureDict(
    const std::map<QString, SurfaceFeatureEntry>& entryMap,
    QString openFoamPath);
};

#endif  // PARSER_SURFACE_FEATURE_H_
