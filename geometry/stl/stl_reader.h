#ifndef STL_READER_H
#define STL_READER_H

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

#include "../mesh_data.h"

namespace StlReader {

    MeshData readStlFile(const QString& fileName, const QByteArray& fileData);
    GeometryMetrics readMetrics(const QByteArray& fileData);
};

#endif // STL_READER_H
