#ifndef PARSER_SURFACE_PATCH_DICT_H_
#define PARSER_SURFACE_PATCH_DICT_H_

#include <QString>

namespace CaseIO {

// Create new surfacePatchDict
QString createSurfacePatchDict(const QString& openFoamPath,
    const QString& fileName, double featureAngle);
};

#endif  // PARSER_SURFACE_PATCH_DICT_H_
