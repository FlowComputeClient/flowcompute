#include "render_data.h"

#include <iostream>
#include <fstream>
#include <map>

std::vector<FieldData> read_openfoam_field(const std::string& fieldPath, const std::vector<std::string>& patchNames) {
    std::vector<FieldData> fieldData;
    std::ifstream file(fieldPath, std::ios::binary);
    if (!file.is_open()) return fieldData;
	
	// Determine if data is ascii or binary
    std::string line;
	FoamFormat format = FoamFormat::Unknown;
    while (std::getline(file, line)) {
        if (line.find("format") != std::string::npos) {
            if (line.find("ascii") != std::string::npos) { std::cout << "ascii" << std::endl; format = FoamFormat::Ascii; break; }
            if (line.find("binary") != std::string::npos) { std::cout << "ascii" << std::endl; format = FoamFormat::Binary; break; }
        }
        if (line.find("// * * * *") != std::string::npos) break;
    }
	if (format == FoamFormat::Unknown) {
		std::cerr << "Couldn't read file format" << std::endl;
		return fieldData;
	}
	
	// Skip everything until the 'boundaryField' designation
	std::string word;
    while (file >> word && word != "boundaryField") {}
	
	// Populate map
    std::map<std::string, FieldData> fieldMap;
    
    int braceDepth = 0;
    std::string currentPatch = "";
    
    while (file >> word) {
        // Track dictionary depth
        if (word == "{") {
            braceDepth++;
            continue;
        } else if (word == "}") {
            braceDepth--;
            if (braceDepth == 0) break;
            if (braceDepth == 1) currentPatch = "";
            continue;
        }

        // Depth 1: We are looking at a patch name
        if (braceDepth == 1 && currentPatch.empty()) {
            currentPatch = word;
            fieldMap[currentPatch] = FieldData{0, {}};
        }
        // Depth 2: We are inside a patch, look for the 'value' entry
        else if (braceDepth == 2 && !currentPatch.empty() && word == "value") {
            std::string valType;
            file >> valType;

            if (valType == "uniform") {
                std::string uniformVal;
                file >> uniformVal;
                
                // If it's a scalar (e.g., "0;"), strip the semicolon
                if (uniformVal.back() == ';') {
                    uniformVal.pop_back();
                    fieldMap[currentPatch].numElements = 1;
                    fieldMap[currentPatch].elementVals.push_back(std::stof(uniformVal));
                } 
                // If it's a vector (e.g., "(0" "0" "0);")
                else if (uniformVal.front() == '(') {
                    std::string val2, val3;
                    file >> val2 >> val3; // Read the next two components
                    
                    uniformVal.erase(0, 1); // Remove '('
                    val3.pop_back(); // Remove ';'
                    val3.pop_back(); // Remove ')'
                    
                    fieldMap[currentPatch].numElements = 3; // 3 floats per element
                    fieldMap[currentPatch].elementVals.push_back(std::stof(uniformVal));
                    fieldMap[currentPatch].elementVals.push_back(std::stof(val2));
                    fieldMap[currentPatch].elementVals.push_back(std::stof(val3));
                }
            } 
            else if (valType == "nonuniform") {
                std::string listType;
                file >> listType; // e.g., "List<scalar>" or "List<vector>"
                
                uint32_t numItems;
                file >> numItems;
                
                // Read the opening '('
                std::string paren;
                file >> paren; 

                uint32_t floatsPerItem = (listType.find("vector") != std::string::npos) ? 3 : 1;
                fieldMap[currentPatch].numElements = floatsPerItem;
                uint32_t totalFloats = numItems * floatsPerItem;
                
                fieldMap[currentPatch].elementVals.resize(totalFloats);

                if (format == FoamFormat::Ascii) {
                    for (uint32_t i = 0; i < totalFloats; ++i) {
                        std::string valStr;
                        file >> valStr;
                        // Strip '(' or ')' from vectors if necessary
                        if (valStr.front() == '(') valStr.erase(0, 1);
                        if (valStr.back() == ')') valStr.pop_back();
                        fieldMap[currentPatch].elementVals[i] = std::stof(valStr);
                    }
                    file >> paren; // Consume closing ')'
                    file >> paren; // Consume ';'
                } 
                else if (format == FoamFormat::Binary) {
                    // OpenFOAM binary files write a single char (usually '\n') after the '('
                    char dummy;
                    file.read(&dummy, 1);
                    
                    // NOTE: OpenFOAM usually compiles with double precision (8 bytes) by default.
                    // If your cases are double precision, you must read into a std::vector<double> 
                    // first, then cast to float, or read sizeof(double) chunks.
                    std::vector<double> tempDouble(totalFloats);
                    file.read(reinterpret_cast<char*>(tempDouble.data()), totalFloats * sizeof(double));
                    
                    for(uint32_t i = 0; i < totalFloats; ++i) {
                        fieldMap[currentPatch].elementVals[i] = static_cast<float>(tempDouble[i]);
                    }
                    
                    // Skip over the closing bytes and semicolon to realign the stream
                    file >> paren; 
                }
            }
        }
    }
	
	// Return FieldData structures in order
	for (const std::string& patchName: patchNames) {
		auto it = fieldMap.find(patchName);
		if (it != fieldMap.end()) {
			fieldData.push_back(fieldMap[patchName]);
		} else {
			std::cerr << "Patch " << patchName << " not found in " << fieldPath << std::endl;
		}		
	}
	return fieldData;
}