#include "mesh_reader.h"

#include <cstring>

MeshData MeshReader::readMesh(const QByteArray& fileData) {
    
    MeshData mesh;    
    if (fileData.size() < static_cast<int>(sizeof(MeshHeader))) {
        qWarning() << "Received payload is too small to contain a MeshHeader.";
        return mesh;
    }

    const char* ptr = fileData.constData();

    // Read Header
    const MeshHeader* meshHeader = reinterpret_cast<const MeshHeader*>(ptr);
    ptr += sizeof(MeshHeader);

    // Verify payload size matches expected byte sizes in the header
    size_t expectedSize = sizeof(MeshHeader) + meshHeader->dataByteSize +
      meshHeader->indexByteSize + (meshHeader->patchCount * sizeof(MeshPatch));

    if (static_cast<size_t>(fileData.size()) < expectedSize) {
        qWarning() << "Payload size mismatch. Expected at least:" << expectedSize << "Got:" << fileData.size();
        return mesh;
    }

    // Assign Header Metadata
    mesh.format = static_cast<VertexFormat>(meshHeader->vertexFormat);
    mesh.boundingBoxMin = { meshHeader->bounds[0], meshHeader->bounds[1], meshHeader->bounds[2] };
    mesh.boundingBoxMax = { meshHeader->bounds[3], meshHeader->bounds[4], meshHeader->bounds[5] };

    // Copy Vertex Data (Floats)
    uint32_t numFloats = meshHeader->dataByteSize / sizeof(float);
    mesh.data.resize(numFloats);
    std::memcpy(mesh.data.data(), ptr, meshHeader->dataByteSize);
    ptr += meshHeader->dataByteSize;

    // Copy Index Data (Uint32)
    uint32_t numIndices = meshHeader->indexByteSize / sizeof(uint32_t);
    mesh.indices.resize(numIndices);
    std::memcpy(mesh.indices.data(), ptr, meshHeader->indexByteSize);
    ptr += meshHeader->indexByteSize;

    // 5.Copy Patch Data
    mesh.patches.resize(meshHeader->patchCount);
    std::memcpy(mesh.patches.data(), ptr, meshHeader->patchCount * sizeof(MeshPatch));
    // ptr += meshHeader->patchCount * sizeof(MeshPatch); // (Optional, just for pointer correctness)

    return mesh;
}
