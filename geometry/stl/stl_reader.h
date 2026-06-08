#ifndef STL_READER_H
#define STL_READER_H

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

#include "../graphic_data.h"

namespace StlReader {

    std::pair<RenderData, bool> readStlFile(const QString& fileName, const QByteArray& fileData);
    GeometryMetrics readMetrics(const QByteArray& fileData);
};

#endif // STL_READER_H
