#ifndef MESH_READER_H
#define MESH_READER_H

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

#include "../graphic_data.h"

namespace MeshReader {
    RenderData readMesh(const QString& fileName, const QByteArray& fileData);
};

#endif // MESH_READER_H
