#ifndef MESH_READER_H
#define MESH_READER_H

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

#include "../mesh_data.h"

namespace MeshReader {
    MeshData readMesh(const QByteArray& fileData);
};

#endif // MESH_READER_H
