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

#include "remote_system.h"

#include <fstream>

#include "mesh_utils.h"

// Parses boundary file to extract patches
std::vector<RenderPatch> RemoteSystem::parseBoundary(const QString& path) {
    std::vector<RenderPatch> patches;

    // Ensure forward slashes for the remote SSH/Linux environment
    std::optional<QByteArray> remoteData = getFileContent(path);
    if (!remoteData.has_value()) {
        qWarning() << "Failed to fetch remote boundary file.";
        return patches;
    }

    // Pre-process directly from the QByteArray to save memory
    const QByteArray& data = remoteData.value();
    std::stringstream ss;

    for (char c : data) {
        if (c == '(' || c == ')' || c == '{' || c == '}' || c == ';') {
            ss << ' ' << c << ' ';
        } else {
            ss << c;
        }
    }

    std::string word;
    // Skip header until the first '('
    while (ss >> word && word != "(") {}

    RenderPatch currentPatch;
    bool inPatch = false;

    // Parse the pre-processed stringstream
    while (ss >> word) {
        // Break at the final closing parenthesis
        if (!inPatch && word == ")") break;

        // Safely ignore parentheses that belong to fields like inGroups
        if (word == "(" || word == ")") continue;

        if (word == "{") {
            inPatch = true;
        } else if (word == "}") {
            if (currentPatch.indexCount > 0) {
                patches.push_back(currentPatch);
            }
            inPatch = false;
        } else if (inPatch && word == "nFaces") {
            ss >> currentPatch.indexCount;
            std::string semi;
            ss >> semi;
        } else if (inPatch && word == "startFace") {
            ss >> currentPatch.firstIndex;
            std::string semi;
            ss >> semi;
        } else if (!inPatch &&
                   !std::isdigit(static_cast<unsigned char>(word[0]))) {
            // Read patch name
            std::strncpy(currentPatch.name, word.c_str(),
                         sizeof(currentPatch.name) - 1);
            currentPatch.name[sizeof(currentPatch.name) - 1] = '\0';
            currentPatch.indexCount = 0;
            currentPatch.firstIndex = 0;
        }
    }
    return patches;
}

std::vector<double> RemoteSystem::readPoints(const QString& path) {
    // Access the file through SFTP
    std::optional<QByteArray> remoteData = getFileContent(path);
    if (!remoteData.has_value()) {
        emit logMessage(QString(tr("Failed to fetch remote points file.")));
        return {};
    }

    // Create stream from the file data
    ByteArrayBuffer buffer(remoteData.value());
    std::istream stream(&buffer);

    // Parse header and leave the stream pointer at the separator
    FoamFormat format = FoamFormat::Unknown;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("format") != std::string::npos) {
            if (line.find("ascii") != std::string::npos)
                format = FoamFormat::Ascii;
            else if (line.find("binary") != std::string::npos)
                format = FoamFormat::Binary;
        }
        if (line.find("// * * * *") != std::string::npos) {
            break;
        }
    }

    std::vector<double> rawPoints;
    if (format == FoamFormat::Binary) {
        size_t numPoints = 0;
        if (MeshUtils::skipToBinary(stream, numPoints)) {
            char c = stream.get();
            if (c == '\r') stream.get();
            else if (c != '\n') stream.unget();

            rawPoints.resize(numPoints * 3);
            stream.read(reinterpret_cast<char*>(rawPoints.data()),
                        numPoints * 3 * sizeof(double));
        }
    } else if (format == FoamFormat::Ascii) {
        std::string token;
        size_t numPoints = 0;
        while (stream >> token) {
            if (std::isdigit(static_cast<unsigned char>(token[0]))) {
                numPoints = std::stoull(token);
                break;
            }
        }

        if (numPoints > 0) {
            rawPoints.resize(numPoints * 3);
            char c;

            // Consume the primary opening '(' for the main list
            while (stream >> c && c != '(') {}

            // 3. Optimized, direct-index extraction loop
            for (size_t i = 0; i < numPoints; ++i) {
                // Extracts the structure: ( x y z )
                stream >> c
                    >> rawPoints[i * 3]
                    >> rawPoints[i * 3 + 1]
                    >> rawPoints[i * 3 + 2]
                    >> c;
            }
        }
    }
    return rawPoints;
}

FaceList RemoteSystem::readFaces(const QString& path) {
    // Access the file through SFTP
    std::optional<QByteArray> remoteData = getFileContent(path);

    // Handle network or file access failures safely
    FaceList result;
    if (!remoteData.has_value()) {
        emit logMessage(QString(tr("Failed to fetch remote faces file.")));
        return result;
    }

    // Create stream from the file data
    ByteArrayBuffer buffer(remoteData.value());
    std::istream stream(&buffer);

    // Parse header and leave the stream pointer at the separator
    FoamFormat format = FoamFormat::Unknown;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("format") != std::string::npos) {
            if (line.find("ascii") != std::string::npos)
                format = FoamFormat::Ascii;
            else if (line.find("binary") != std::string::npos)
                format = FoamFormat::Binary;
        }
        if (line.find("// * * * *") != std::string::npos)
            break;
    }

    // Read data
    if (format == FoamFormat::Binary) {
        size_t totalOffsets = 0;
        if (MeshUtils::skipToBinary(stream, totalOffsets)) {
            result.offsets.resize(totalOffsets);
            stream.read(reinterpret_cast<char*>(
                  result.offsets.data()), totalOffsets * sizeof(int32_t));
        }
        size_t totalIndices = 0;
        if (MeshUtils::skipToBinary(stream, totalIndices)) {
            result.connectivity.resize(totalIndices);
            stream.read(reinterpret_cast<char*>(
                  result.connectivity.data()), totalIndices * sizeof(int32_t));
        }
    } else if (format == FoamFormat::Ascii) {
        std::string line;
        while (std::getline(stream, line)) {
            if (line.find("// * * * *") != std::string::npos) break;
        }

        size_t size1 = 0;
        std::string token;
        while (stream >> token) {
            if (std::isdigit(token[0])) {
                size1 = std::stoull(token);
                break;
            }
        }

        char c;
        while (stream >> c && c != '(') {}

        int32_t firstVal;
        stream >> firstVal;
        stream >> std::ws;
        c = stream.peek();

        if (c == '(') {
            // Parses a Standard OpenFOAM Face List
            result.offsets.reserve(size1 + 1);
            result.offsets.push_back(0);

            stream >> c; // consume '('
            for(int i = 0; i < firstVal; ++i) {
                int32_t idx; stream >> idx;
                result.connectivity.push_back(idx);
            }
            while (stream >> c && c != ')') {}
            result.offsets.push_back(result.connectivity.size());

            for (size_t f = 1; f < size1; ++f) {
                int32_t numVerts;
                stream >> numVerts >> c;
                for(int i = 0; i < numVerts; ++i) {
                    int32_t idx; stream >> idx;
                    result.connectivity.push_back(idx);
                }
                while (stream >> c && c != ')') {}
                result.offsets.push_back(result.connectivity.size());
            }
        } else {
            // Parses a Compact OpenFOAM Face List
            result.offsets.reserve(size1);
            result.offsets.push_back(firstVal);
            for (size_t i = 1; i < size1; ++i) {
                int32_t val;
                stream >> val;
                result.offsets.push_back(val);
            }
            while (stream >> c && c != ')') {}

            size_t size2 = 0;
            while (stream >> token) {
                if (std::isdigit(token[0])) {
                    size2 = std::stoull(token);
                    break;
                }
            }
            while (stream >> c && c != '(') {}
            result.connectivity.reserve(size2);
            for (size_t i = 0; i < size2; ++i) {
                int32_t val;
                stream >> val;
                result.connectivity.push_back(val);
            }
        }
    }
    return result;
}

RenderData RemoteSystem::getMeshData(const QString& path) {
    RenderData renderData;
    // Read boundary data
    QString polyDir = path + "/constant/polyMesh";
    renderData.patches = parseBoundary(polyDir + "/boundary");
    if (renderData.patches.empty()) {
        emit logMessage(QString(
            tr("Failed to parse boundary file or no patches found.\n")));
        return renderData;
    }

    std::vector<double> rawPoints = readPoints(polyDir + "/points");
    if (rawPoints.empty()) {
        emit logMessage(QString(tr("Failed to parse points file.\n")));
        return renderData;
    }

    FaceList faces = readFaces(polyDir + "/faces");
    if (faces.offsets.empty() || faces.connectivity.empty()) {
        emit logMessage(QString(tr("Failed to parse faces file.\n")));
        return renderData;
    }

    // Declare containers
    const uint32_t totalRawPoints = static_cast<uint32_t>(rawPoints.size() / 3);
    std::vector<uint32_t> globalToLocal(totalRawPoints, UINT32_MAX);
    std::vector<std::pair<uint32_t, uint32_t>> flatAdjacency;

    float minX = (std::numeric_limits<float>::max)(), minY = minX, minZ = minX;
    float maxX = std::numeric_limits<float>::lowest(), maxY = maxX, maxZ = maxX;

    for (auto& patch : renderData.patches) {
        uint32_t foamStartFace = patch.firstIndex;
        uint32_t foamNFaces = patch.indexCount;

        patch.firstIndex = static_cast<uint32_t>(renderData.indices.size());
        patch.firstLineIndex =
            static_cast<uint32_t>(renderData.lineIndices.size());

        for (uint32_t f = foamStartFace; f < foamStartFace + foamNFaces; ++f) {

            // Ensure face index doesn't exceed offset array
            if (f + 1 >= faces.offsets.size()) {
                // std::cerr <<
                // "Error: Face index " << f << " out of bounds.\n";
                break;
            }

            int32_t startOffset = faces.offsets[f];
            int32_t endOffset   = faces.offsets[f + 1];

            // Make sure connectivity array is large enough
            if (static_cast<size_t>(endOffset) > faces.connectivity.size()) {
                // std::cerr << "Error: Connectivity offset out of bounds.\n";
                break;
            }

            std::vector<uint32_t> localFaceLoop;
            renderData.data.reserve(rawPoints.size());
            flatAdjacency.reserve(faces.connectivity.size());
            localFaceLoop.reserve(endOffset - startOffset);

            for (int32_t idx = startOffset; idx < endOffset; ++idx) {
                uint32_t globalPtIdx =
                    static_cast<uint32_t>(faces.connectivity[idx]);

                // Don't read past the end of rawPoints or globalToLocal
                if (globalPtIdx >= totalRawPoints) {
                    // std::cerr << "Error: Global point index " <<
                    // globalPtIdx << " out of bounds.\n";
                    continue;
                }

                uint32_t localPtIdx;
                if (globalToLocal[globalPtIdx] == UINT32_MAX) {
                    // Point not yet inserted
                    localPtIdx =
                        static_cast<uint32_t>(renderData.data.size() / 3);
                    globalToLocal[globalPtIdx] = localPtIdx;

                    float x = static_cast<float>(rawPoints[globalPtIdx * 3]);
                    float y =
                        static_cast<float>(rawPoints[globalPtIdx * 3 + 1]);
                    float z =
                        static_cast<float>(rawPoints[globalPtIdx * 3 + 2]);

                    renderData.data.push_back(x);
                    renderData.data.push_back(y);
                    renderData.data.push_back(z);

                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                    if (z < minZ) minZ = z;
                    if (z > maxZ) maxZ = z;
                } else {
                    // Point already exists
                    localPtIdx = globalToLocal[globalPtIdx];
                }

                localFaceLoop.push_back(localPtIdx);

                // Add pair to flat adjacency list
                flatAdjacency.push_back({localPtIdx, f});
            }

            const size_t numVerts = localFaceLoop.size();
            for (size_t i = 0; i < numVerts; ++i) {
                renderData.lineIndices.push_back(localFaceLoop[i]);
                renderData.lineIndices.push_back(
                    localFaceLoop[(i + 1) % numVerts]);
            }

            MeshUtils::triangulatePolygon(std::move(localFaceLoop),
                renderData.data, renderData.indices);
        }

        patch.indexCount =
            static_cast<uint32_t>(renderData.indices.size()) -
            patch.firstIndex;
        patch.lineIndexCount =
            static_cast<uint32_t>(renderData.lineIndices.size()) -
            patch.firstLineIndex;
    }

    renderData.boundingBoxMin = {minX, minY, minZ};
    renderData.boundingBoxMax = {maxX, maxY, maxZ};

    const uint32_t numLocalPoints =
        static_cast<uint32_t>(renderData.data.size() / 3);

    // Count the degree of each point
    std::vector<uint32_t> pointDegrees(numLocalPoints, 0);
    for (const auto& pair : flatAdjacency) {
        pointDegrees[pair.first]++;
    }

    // Compute the prefix sum to build the pointOffsets array
    renderData.pointOffsets.reserve(numLocalPoints + 1);
    renderData.pointOffsets.push_back(0);

    uint32_t currentSum = 0;
    for (uint32_t degree : pointDegrees) {
        currentSum += degree;
        renderData.pointOffsets.push_back(currentSum);
    }

    // Create a working copy of the offsets
    std::vector<uint32_t> currentInsertPos = renderData.pointOffsets;

    // Pre-allocate the adjacency array
    renderData.faceAdjacency.resize(flatAdjacency.size());

    for (const auto& pair : flatAdjacency) {
        uint32_t ptIdx = pair.first;
        uint32_t faceIdx = pair.second;

        // Find the correct destination index for this face,
        uint32_t insertIdx = currentInsertPos[ptIdx]++;
        renderData.faceAdjacency[insertIdx] = faceIdx;
    }

    // Set the format
    renderData.format = RenderType::Mesh;
    return renderData;
}
