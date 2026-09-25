#ifndef RENDER_DATA_H
#define RENDER_DATA_H

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct RenderHeader {
    uint32_t magicNumber;
    uint32_t dataByteSize;
    uint32_t indexByteSize;
    uint32_t lineIndexByteSize;	
	uint32_t patchesByteSize;
    std::array<float, 3> boundingBoxMin;
    std::array<float, 3> boundingBoxMax;
};

struct RenderPatch {
    char name[64];
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t firstLineIndex;
    uint32_t lineIndexCount;
};

struct RenderPayload {
    std::string jsonString;
    RenderHeader header;
    std::vector<RenderPatch> patches;
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> lineIndices;
};

struct RenderData {
    std::vector<float> data;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> lineIndices;
    std::vector<RenderPatch> patches;
    std::vector<uint32_t> pointOffsets;
    std::vector<uint32_t> faceAdjacency;	
    std::array<float, 3> boundingBoxMin;
    std::array<float, 3> boundingBoxMax;
};

struct FieldData {
    uint32_t numElements;
    std::vector<float> elementVals;
};

struct FieldDataHeader {
    uint32_t magicNumber;
    uint32_t numFields;
    uint32_t sizesByteSize;
    uint32_t dataByteSize;
};

struct FieldDataPayload {
    std::string jsonString;
    FieldDataHeader header;
    std::vector<FieldData> fields;
    std::vector<uint32_t> fieldSizes;
};

enum class FoamFormat { Ascii, Binary, Unknown };

struct FaceList {
    std::vector<int32_t> offsets;
    std::vector<int32_t> connectivity;
};

RenderData read_mesh(const std::filesystem::path& casePath);
std::vector<FieldData> read_openfoam_field(const std::string& fieldPath, const std::vector<std::string>& patchNames);

#endif  // RENDER_DATA_H
