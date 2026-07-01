// Copyright 2026 FlowCompute LLC
//
// This file is part of FlowCompute.
//
// FlowCompute is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// FlowCompute is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with FlowCompute. If not, see <https://www.gnu.org/licenses/>.

#include "mesh_reader.h"

#include <cstring>

RenderData MeshReader::readMesh(const QString& fileName, const QByteArray& fileData) {

    RenderData mesh;
    mesh.format = RenderType::Mesh;
    if (fileData.isEmpty()) return mesh;
    mesh.boundingBoxMin = { std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max() };
    mesh.boundingBoxMax = { std::numeric_limits<float>::lowest(),
                           std::numeric_limits<float>::lowest(),
                           std::numeric_limits<float>::lowest() };

    auto resolvePatchName = [&](const std::string& name) -> std::string {
        if (name.empty()) {
            return fileName.toStdString();
        }
        return name;
    };

    QTextStream stream(fileData);
    QString line;
    std::string currentPatchName;
    bool inSolid = false;
    std::vector<std::string> patchOrder;
    std::unordered_map<std::string, std::vector<float>> patchBuckets;
    static const QRegularExpression wsRe("\\s+");

    while (stream.readLineInto(&line)) {
        line = line.trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith("solid", Qt::CaseInsensitive)) {
            QString namePart = line.mid(5).trimmed();
            currentPatchName = resolvePatchName(namePart.toStdString());

            // If this is the first time seeing this patch name, record its order
            if (patchBuckets.find(currentPatchName) == patchBuckets.end()) {
                patchOrder.push_back(currentPatchName);
            }
            inSolid = true;
        }
        else if (line.startsWith("endsolid", Qt::CaseInsensitive)) {
            inSolid = false;
        }
        else if (inSolid && line.startsWith("vertex", Qt::CaseInsensitive)) {
            QStringList parts = line.split(wsRe, Qt::SkipEmptyParts);
            if (parts.size() >= 4) {
                float x = parts[1].toFloat();
                float y = parts[2].toFloat();
                float z = parts[3].toFloat();

                auto& bucket = patchBuckets[currentPatchName];
                size_t vertexIndex = bucket.size() / 6;
                int triVert = vertexIndex % 3;

                float bx = (triVert == 0) ? 1.0f : 0.0f;
                float by = (triVert == 1) ? 1.0f : 0.0f;
                float bz = (triVert == 2) ? 1.0f : 0.0f;

                // Push position
                bucket.push_back(x);
                bucket.push_back(y);
                bucket.push_back(z);

                // Push barycentric coordinates
                bucket.push_back(bx);
                bucket.push_back(by);
                bucket.push_back(bz);

                // Update bounding box
                mesh.boundingBoxMin[0] = std::min(mesh.boundingBoxMin[0], x);
                mesh.boundingBoxMin[1] = std::min(mesh.boundingBoxMin[1], y);
                mesh.boundingBoxMin[2] = std::min(mesh.boundingBoxMin[2], z);

                mesh.boundingBoxMax[0] = std::max(mesh.boundingBoxMax[0], x);
                mesh.boundingBoxMax[1] = std::max(mesh.boundingBoxMax[1], y);
                mesh.boundingBoxMax[2] = std::max(mesh.boundingBoxMax[2], z);
            }
        }
    }

    // Flatten the buckets into the final contiguous mesh data
    for (const std::string& name : patchOrder) {
        const std::vector<float>& bucketData = patchBuckets[name];

        if (!bucketData.empty()) {
            RenderPatch patch;

            // Safely copy the bucket name into the char[64] array
            std::strncpy(patch.name, name.c_str(), sizeof(patch.name) - 1);
            patch.name[sizeof(patch.name) - 1] = '\0'; // Guarantee null-termination

            // Calculate vertex counts based on 6 floats per vertex
            patch.first = static_cast<uint32_t>(mesh.data.size() / 6);
            patch.count = static_cast<uint32_t>(bucketData.size() / 6);

            // Append the entire bucket to the final data vector
            mesh.data.insert(mesh.data.end(), bucketData.begin(), bucketData.end());
            mesh.patches.push_back(patch);
        }
    }
    return mesh;
}
