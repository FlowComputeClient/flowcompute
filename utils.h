#ifndef UTILS_H
#define UTILS_H

#include <QRegularExpression>
#include <QString>
#include <QTextStream>

#include "core_types.h"

namespace Utils {

    QString createFoamHeader(const QString& objectName, const QString& foamPath);
    QString createFoamFooter();
    QString createSurfacePatchDict(const QString& openFoamPath, const QString& fileName,
                                   double featureAngle);
    QString createDecomposeParDict(const QString& openFoamPath, int numCores);
    QString createFieldFile(const QString& openFoamPath, QString fieldName,
                            FlowCompute::FieldData data);
};
#endif // UTILS_H
