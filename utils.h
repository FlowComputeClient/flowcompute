#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QWidget>

#include "core_types.h"

namespace Utils {

    enum class ParseErrorAction {
        EditFile,
        Overwrite,
        Cancel
    };

    QString createFoamHeader(const QString& objectName, const QString& foamPath);
    QString createFoamFooter();
    QString createSurfacePatchDict(const QString& openFoamPath, const QString& fileName,
                                   double featureAngle);
    QString createDecomposeParDict(const QString& openFoamPath, int numCores);
    QString createFieldFile(const QString& openFoamPath, QString fieldName,
                            FlowCompute::FieldData data);
    ParseErrorAction showParsingErrorMessage(QString fileName, QWidget* parent);
};
#endif // UTILS_H
