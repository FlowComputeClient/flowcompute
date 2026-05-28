#ifndef UTILS_H
#define UTILS_H

#include <QRegularExpression>
#include <QString>
#include <QTextStream>


namespace Utils {

QString createFoamHeader(const QString& objectName, const QString& foamPath);
QString createFoamFooter();
QString createSurfacePatchDict(const QString& openFoamPath, const QString& fileName,
                               const QString& stem, double featureAngle);
};
#endif // UTILS_H
