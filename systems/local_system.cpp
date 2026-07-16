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

#include "local_system.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

#include <algorithm>
#include <fstream>
#include <limits>
#include <regex>
#include <string>
#include <utility>
#include <vector>
#include <zlib.h>

QStringList LocalSystem::processPaths(const QString& pathString,
                                      PathOperationType opType) {
    QStringList result;

    auto ends_with = [](const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() &&
               str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    };

    // Handle LIST with an empty input string
    if (opType == PathOperationType::LIST && pathString.isEmpty()) {
        const char* home_env = std::getenv("HOME");
        std::string home_dir = home_env ? home_env : "/";

        result.append(QString::fromStdString(home_dir));
        auto options = fs::directory_options::skip_permission_denied;
        for (const auto& entry : fs::directory_iterator(home_dir, options)) {
            std::string item_name = entry.path().filename().string();
            if (!item_name.empty() && item_name.front() == '.') continue;

            if (ends_with(item_name, "_patched.stl") ||
                ends_with(item_name, "_tmp.stl")) continue;

            if (entry.is_regular_file()) item_name += "|";
            result.append(QString::fromStdString(item_name));
        }

        return result;
    }

    // Error handling if input is empty for other operations
    if (pathString.isEmpty()) {
        return QStringList{"Input paths string was empty."};
    }

    // Process the delimited string using Qt string splitting
    QStringList targetPaths = pathString.split('\n', Qt::SkipEmptyParts);

    for (const QString& q_path : std::as_const(targetPaths)) {
        std::string target_path = q_path.toStdString();
        std::error_code ec;
        fs::path p(target_path);
        bool exists = fs::exists(p, ec);

        switch (opType) {
        case PathOperationType::CREATE: {
            if (exists && fs::is_regular_file(p))
                continue;
            if (fs::create_directories(p, ec) || fs::exists(p)) {
                result.append("0");
            } else {
                result.append("-1");
            }
            break;
        }

        case PathOperationType::DELETE: {
            if (!exists) {
                result.append("-1");
            } else {
                fs::remove_all(p, ec);
                if (!ec) {
                    result.append("0");
                } else {
                    result.append("-1");
                }
            }
            break;
        }

        case PathOperationType::CHECK: {
            if (exists) {
                result.append("0");
            } else {
                result.append("-1");
            }
            break;
        }

        case PathOperationType::LIST: {
            if (exists && fs::is_regular_file(p)) {
                continue;
            }
            if (exists && fs::is_directory(p)) {
                auto options = fs::directory_options::skip_permission_denied;
                for (const auto& entry : fs::directory_iterator(p, options)) {
                    std::string item_name = entry.path().filename().string();
                    if (!item_name.empty() && item_name.front() == '.') continue;

                    if (ends_with(item_name, "_patched.stl") ||
                        ends_with(item_name, "_tmp.stl")) continue;

                    if (entry.is_regular_file()) item_name += "|";
                    result.append(QString::fromStdString(item_name));
                }
            }
            break;
        }}
    }
    return result;
}

// Launch a utility in the server
int LocalSystem::launchShortUtility(const QString& cmd, QString& output) {
    output.clear();
    if (cmd.isEmpty()) {
        output = "Error: Command is empty";
        return -1;
    }

    // Launch command in process
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("bash", QStringList() << "-c" << cmd);

    // Wait for the process to finish
    if (!process.waitForFinished(-1)) {
        output = "Error: Process failed to execute or timed out.";
        return -2;
    }

    // Populate the reference string with the stdout/stderr
    output = QString::fromUtf8(process.readAll()).trimmed();

    // Check if the process crashed
    if (process.exitStatus() != QProcess::NormalExit) {
        return -3;
    }

    // Return the bash exit code
    return process.exitCode();
}

void LocalSystem::launchLongUtility(const QString& cmd, const QString& caseName,
                                    UtilityType utilityType) {
    // Allocate on the heap so it runs asynchronously
    QProcess* process = new QProcess(this);

    // Merge standard error into standard output, mimicking "2>&1"
    process->setProcessChannelMode(QProcess::MergedChannels);

    // Stream the output as the process runs
    connect(process, &QProcess::readyReadStandardOutput, this,
        [this, process]() {
        while (process->canReadLine()) {
            QByteArray line = process->readLine();
            emit logMessage(QString::fromUtf8(line).trimmed());
        }
    });

    // Handle process completion (success or failure)
    connect(process, &QProcess::finished, this, [this, caseName, utilityType] (
            int exitCode, QProcess::ExitStatus exitStatus) {
        // Set status
        QString status = "error";

        // Check for a normal crash-free exit and success code (0)
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            status = "success";
        }

        // Emit the final completion state
        emit longUtilityFinished(status, caseName, utilityType);
    });

    // Handle process-level execution errors
    connect(process, &QProcess::errorOccurred, this,
            [this, process](QProcess::ProcessError error) {
        emit longUtilityError("Process Error: " + process->errorString());
    });

    // Clean up memory automatically when the process finishes
    connect(process, &QProcess::finished, process, &QObject::deleteLater);

    // Start process
    process->start("bash", QStringList() << "-c" << cmd);
}

// Locate OpenFOAM installation(s)
QStringList LocalSystem::findOpenFoam() {
    QStringList ofList;
    std::vector<fs::path> base_paths = {"/usr/lib/openfoam", "/opt"};
    for (const auto& base : base_paths) {
        if (fs::exists(base) && fs::is_directory(base)) {
            for (const auto& entry : fs::directory_iterator(base)) {
                std::string dirname = entry.path().filename().string();
                if (entry.is_directory() && dirname.rfind("openfoam", 0) == 0) {
                    ofList.append(
                        QString::fromStdString(entry.path().string()));
                }
            }
        }
    }
    return ofList;
}

QStringList LocalSystem::getTutorials(const QString& base_path) {
    QStringList result;
    fs::path target_path = fs::path(base_path.toStdString()) / "tutorials";

    // Return the error message as the first element
    if (!fs::exists(target_path) || !fs::is_directory(target_path)) {
        result.append("Tutorials path does not exist.");
        return result;
    }

    try {
        auto options = fs::directory_options::skip_permission_denied;
        for (const auto& entry :
             fs::recursive_directory_iterator(target_path, options)) {
            if (entry.is_directory() && entry.path().filename() == "system") {
                result.append(QString::fromStdString(
                    entry.path().parent_path().string()));
            }
        }
    } catch (const std::exception& e) {
        // Return the exception message as the first element
        result.clear();
        result.append(QString("Filesystem error: ") + e.what());
    }
    return result;
}

// ZLIB extraction using standard C++ I/O and zlib
bool extractGzGeometry(const std::string& gzFilePath,
                       const std::string& outFilePath) {
    gzFile inFile = gzopen(gzFilePath.c_str(), "rb");
    if (!inFile) return false;

    std::ofstream outFile(outFilePath, std::ios::binary);
    if (!outFile.is_open()) {
        gzclose(inFile);
        return false;
    }

    const int bufferSize = 128 * 1024;
    std::vector<char> buffer(bufferSize);
    int bytesRead = 0;

    while ((bytesRead = gzread(inFile, buffer.data(), bufferSize)) > 0) {
        outFile.write(buffer.data(), bytesRead);
    }

    gzclose(inFile);
    outFile.close();
    return (bytesRead >= 0);
}

// Evaluate wildcards (*, ?) natively via Qt
bool matchGlob(const std::string& pattern, const std::string& text) {
    QRegularExpression re =
        QRegularExpression::fromWildcard(QString::fromStdString(pattern));
    return re.match(QString::fromStdString(text)).hasMatch();
}

void LocalSystem::processAllrunScript(const fs::path& scriptPath,
    const fs::path& projectPath, const fs::path& originalTutorialPath) {

    if (!fs::exists(scriptPath)) return;
    std::ifstream file(scriptPath);
    if (!file.is_open()) return;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Line continuation removal
    std::regex continuationRegex(R"(\\[ \t]*\r?\n\s*)");
    content = std::regex_replace(content, continuationRegex, " ");

    // Find the base "tutorials" folder
    fs::path tutorialsBase;
    fs::path current = originalTutorialPath;
    while (current.has_parent_path()) {
        if (current.filename() == "tutorials") {
            tutorialsBase = current;
            break;
        }
        current = current.parent_path();
    }

    if (tutorialsBase.empty()) {
        emit logMessage(QString::fromStdString("Could not deduce base "
            "'tutorials' directory from: " + originalTutorialPath.string()));
        return;
    }

    // Regex to find 'cp' commands targeting resources/geometry
    std::regex cpRegex(
        R"regex(cp\s+(?:-[^\s]+\s+)*)regex"
        R"regex("?\$(?:FOAM_TUTORIALS|[{]FOAM_TUTORIALS[}])"?)regex"
        R"regex(/resources/geometry/([^\s"]+)"?\s+([^\s"]+)"?)regex");
    std::smatch match;

    std::string::const_iterator searchStart(content.cbegin());
    while (std::regex_search(searchStart, content.cend(), match, cpRegex)) {
        std::string geomPattern = match[1].str();
        std::string destDirStr = match[2].str();

        fs::path sourceDir = tutorialsBase / "resources" / "geometry";
        fs::path destDir = projectPath / destDirStr;

        if (!fs::exists(destDir)) {
            fs::create_directories(destDir);
        }

        // Make sure the resources/geometry directory exists
        if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir)) {
            emit logMessage(QString::fromStdString("Geometry resources "
                "directory not found: " + sourceDir.string()));
            searchStart = match.suffix().first;
            continue;
        }

        bool fileMatched = false;

        // Iterate through the resources/geometry directory
        for (const auto& entry : fs::directory_iterator(sourceDir)) {
            std::string filename = entry.path().filename().string();

            // Check if the file matches the script's pattern
            if (matchGlob(geomPattern, filename)) {
                fileMatched = true;
                fs::path sourcePath = entry.path();

                // Handle Directory, GZ file, or Standard File
                if (fs::is_directory(sourcePath)) {
                    fs::copy(sourcePath, destDir / filename,
                        fs::copy_options::recursive |
                            fs::copy_options::overwrite_existing);
                    emit logMessage(QString::fromStdString(
                        "Copied geometry dir: " + filename));
                } else if (sourcePath.extension() == ".gz") {
                    fs::path outPath = destDir / sourcePath.stem();
                    if (extractGzGeometry(sourcePath.string(),
                                          outPath.string())) {
                        emit logMessage(QString::fromStdString(
                            "Decompressed: " + outPath.filename().string()));
                    } else {
                        emit logMessage(QString::fromStdString(
                            "Failed to decompress: " + filename));
                    }
                } else {
                    fs::copy_file(sourcePath, destDir / filename,
                                  fs::copy_options::overwrite_existing);
                    emit logMessage(QString::fromStdString(
                        "Copied geometry file: " + filename));
                }
            }
        }

        if (!fileMatched) {
            emit logMessage(QString::fromStdString(
                "No geometry files matched pattern: " + geomPattern));
        }
        searchStart = match.suffix().first;
    }
}

// Copy tutorials locally
QStringList LocalSystem::copyTutorialFolders(const QString& tutPath,
                                            const QString& projPath) {
    QStringList result;

    fs::path tutorial_path = fs::path(tutPath.toStdString());
    fs::path project_path = fs::path(projPath.toStdString());

    if (!fs::exists(tutorial_path) || !fs::is_directory(tutorial_path)) {
        emit logMessage("Tutorial path does not exist or is not a directory.");
        return result;
    }

    try {
        if (!fs::exists(project_path)) {
            fs::create_directories(project_path);
        } else if (fs::is_directory(project_path)) {
            for (const auto& entry : fs::directory_iterator(project_path)) {
                fs::remove_all(entry.path());
            }
        } else {
            emit logMessage("Project path exists but is not a directory.");
            return result;
        }
    } catch (const fs::filesystem_error& e) {
        emit logMessage(
            QString("Failed to prepare project directory. Error: ") + e.what());
        return result;
    }

    // Essential files to copy
    std::vector<std::string> items = { "0", "0.orig", "constant", "system",
                                      "Allrun", "Allrun.pre", "Allclean" };

    const auto copyOptions = fs::copy_options::recursive |
                             fs::copy_options::overwrite_existing;
    for (const auto& item_name : items) {
        fs::path source_item = tutorial_path / item_name;
        fs::path dest_item   = project_path / item_name;

        if (fs::exists(source_item)) {
            try {
                fs::copy(source_item, dest_item, copyOptions);
                std::string formatted_name = item_name;
                if (fs::is_regular_file(source_item)) {
                    formatted_name += "|";
                }
                result.append(QString::fromStdString(formatted_name));
            } catch (const fs::filesystem_error& e) {
                emit logMessage(QString::fromStdString("Failed to copy " +
                    item_name + ". Error: " + std::string(e.what())));
                return result;
            }
        }
    }

    // Parse both scripts to auto-resolve geometry dependencies
    processAllrunScript(project_path/"Allrun", project_path, tutorial_path);
    processAllrunScript(project_path/"Allrun.pre", project_path, tutorial_path);

    if (!fs::exists(project_path / "system")) {
        emit logMessage("Warning: No 'system' directory was found in "
                        "the tutorial folder.");
    }

    // Use Qt containers to check and prepend "constant" if necessary
    if (fs::exists(project_path/"constant") && !result.contains("constant")) {
        result.prepend("constant");
    }

    return result;
}

bool LocalSystem::writeData(const QByteArray& data, const QString& filePath) {
    // Create and open the file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing: " << file.errorString();
        return false;
    }

    // Write data to the file and close it
    file.write(data);
    file.close();
    return true;
}

bool LocalSystem::writeData(const QString& srcPath, const QString& dstPath) {
    // Make sure source file exists
    QFileInfo srcInfo(srcPath);
    if (!srcInfo.exists() || !srcInfo.isFile())
        return false;

    // Ensure the destination directory exists.
    QFileInfo dstInfo(dstPath);
    QDir dir = dstInfo.dir();
    if (!dir.exists() && !dir.mkpath("."))
        return false;

    // QFile::copy() fails if the destination already exists.
    if (QFile::exists(dstPath) && !QFile::remove(dstPath))
        return false;

    return QFile::copy(srcPath, dstPath);
}

QString LocalSystem::getResultFolders(QString casePath) {
    // Verify the path exists and is a directory
    QFileInfo targetInfo(casePath);
    if (!targetInfo.exists() || !targetInfo.isDir()) {
        return "Directory does not exist.";
    }

    // Sort folders numerically (e.g., 0.5, 1, 2, 10)
    QMap<double, QString> sorted_folders;

    // Iterate through directories except "." and ".."
    QDir targetDir(casePath);
    QFileInfoList entries = targetDir.entryInfoList(QDir::Dirs |
                                                    QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : std::as_const(entries)) {
        QString dir_name = entry.fileName();

        // Check for a valid OpenFOAM numeric time step
        bool ok = false;
        double time_val = dir_name.toDouble(&ok);

        // Check if string is a valid number
        if (ok) {
            QDir subDir(entry.filePath());

            // Second pass: Check if this time directory contains any .vtp file
            subDir.setNameFilters(QStringList() << "*.vtp");
            subDir.setFilter(QDir::Files);

            // Check if the filtered list is empty
            if (!subDir.entryList().isEmpty()) {
                sorted_folders.insert(time_val, dir_name);
            }
        }
    }

    // Extract the sorted string values into a list
    QStringList folder_names = sorted_folders.values();
    return folder_names.join(",");
}

QByteArray LocalSystem::getFileContent(const QString& path) {
    QByteArray fileData;

    // Make sure the path exists
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        return {};
    }

    // Read data if it's a file
    QFile file(path);
    if (fileInfo.isFile()) {
        if (file.open(QIODevice::ReadOnly)) {
            fileData = file.readAll();
        }
    }

    file.close();
    return fileData;
}

inline QByteArray extract_vtk_payload(QTextStream& stream, QString& line) {
    QString base64_payload;

    // Read past the opening tag's closing bracket
    int tagEnd = line.indexOf('>');
    while (tagEnd == -1) {
        if (!stream.readLineInto(&line)) return {};
        tagEnd = line.indexOf('>');
    }

    // Grab Base64 data that might share the line with the '>'
    if (tagEnd + 1 < line.size()) {
        base64_payload += line.mid(tagEnd + 1);
    }

    // Accumulate lines until we find the closing tag
    while (!base64_payload.contains("</DataArray>")) {
        QString nextLine;
        if (!stream.readLineInto(&nextLine)) return {};
        base64_payload += nextLine;
    }

    // Strip the closing tag from the accumulated payload
    int closeTagPos = base64_payload.indexOf("</DataArray>");
    if (closeTagPos != -1) {
        base64_payload.truncate(closeTagPos);
    }

    // Decode Base64 data
    QByteArray decoded = QByteArray::fromBase64(base64_payload.toUtf8());

    // Remove header
    if (decoded.size() >= 8) {
        decoded.remove(0, 8);
    }

    return decoded;
}

std::pair<float, float> get_pressure_range(const QByteArray& pData) {
    // Check for at least one float
    if (pData.size() < static_cast<int>(sizeof(float))) {
        return {0.0f, 0.0f};
    }

    size_t floatCount = pData.size() / sizeof(float);

    // With the header cleanly removed, we can safely start at index 0
    const float* floatData = reinterpret_cast<const float*>(pData.constData());

    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();

    for (size_t i = 0; i < floatCount; ++i) {
        float currentFloat = floatData[i];

        if (std::isfinite(currentFloat)) {
            minVal = std::min(minVal, currentFloat);
            maxVal = std::max(maxVal, currentFloat);
        }
    }

    if (minVal > maxVal) {
        return {0.0f, 0.0f};
    }

    return {minVal, maxVal};
}

RenderData LocalSystem::getResultData(const QString& path) {
    bool pointsFound = false, connectivityFound = false,
        offsetsFound = false, pFound = false;
    QByteArray pointsData, connectivityData, offsetsData, pData;

    QFile file(path + "/pressure.vtp");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open VTK file:" << file.fileName();
        return {};
    }

    QTextStream stream(&file);
    QString line;

    while (stream.readLineInto(&line)) {
        if (!line.contains("<DataArray")) continue;

        if (!pointsFound && (line.contains("'Points'") ||
                             line.contains("\"Points\""))) {
            pointsData = extract_vtk_payload(stream, line);
            pointsFound = true;
        } else if (!connectivityFound && (line.contains("'connectivity'") ||
                                        line.contains("\"connectivity\""))) {
            connectivityData = extract_vtk_payload(stream, line);
            connectivityFound = true;
        } else if (!offsetsFound && (line.contains("'offsets'") ||
                                   line.contains("\"offsets\""))) {
            offsetsData = extract_vtk_payload(stream, line);
            offsetsFound = true;
        } else if (!pFound && (line.contains("'p'") ||
                               line.contains("\"p\""))) {
            pData = extract_vtk_payload(stream, line);
            pFound = true;
        }

        if (pointsFound && connectivityFound && offsetsFound && pFound) {
            break;
        }
    }
    file.close();

    // Find min/max of pressure
    auto [minP, maxP] = get_pressure_range(pData);
    float range = maxP - minP;
    float invRange = (range > 1e-8f) ? (1.0f / range) : 0.0f;

    // Validate array sizes
    const std::size_t numPoints = pData.size() / sizeof(float);
    if (static_cast<size_t>(pointsData.size()) !=
            numPoints * 3 * sizeof(float)) {
        qWarning() << "Coordinate and pressure arrays have different sizes.";
        return {};
    }

    // Allocate interleaved vertex buffer
    std::vector<float> vertexBuffer(numPoints * 4);
    float* destPtr = vertexBuffer.data();

    // Cast the raw byte arrays to float pointers for direct access
    const float* rawPoints =
        reinterpret_cast<const float*>(pointsData.constData());
    const float* rawPressures =
        reinterpret_cast<const float*>(pData.constData());

    // Initialize STL array bounding boxes
    std::array<float, 3> bbMin = { std::numeric_limits<float>::max(),
                                  std::numeric_limits<float>::max(),
                                  std::numeric_limits<float>::max() };
    std::array<float, 3> bbMax = { std::numeric_limits<float>::lowest(),
                                  std::numeric_limits<float>::lowest(),
                                  std::numeric_limits<float>::lowest() };

    // Interleave and normalize directly into the float vector
    for (std::size_t i = 0; i < numPoints; i++) {
        const float* coords = rawPoints + (i * 3);

        // Update bounding box
        bbMin[0] = std::min(bbMin[0], coords[0]);
        bbMin[1] = std::min(bbMin[1], coords[1]);
        bbMin[2] = std::min(bbMin[2], coords[2]);
        bbMax[0] = std::max(bbMax[0], coords[0]);
        bbMax[1] = std::max(bbMax[1], coords[1]);
        bbMax[2] = std::max(bbMax[2], coords[2]);

        // Copy coordinates
        destPtr[0] = coords[0];
        destPtr[1] = coords[1];
        destPtr[2] = coords[2];

        // Extract and normalize pressure
        float p_raw = rawPressures[i];
        destPtr[3] = (p_raw - minP) * invRange;

        // Advance the destination pointer by 4 floats
        destPtr += 4;
    }

    // Interpret byte arrays as uint32 arrays
    const uint32_t* connectivityVals =
        reinterpret_cast<const uint32_t*>(connectivityData.constData());
    const uint32_t* offsetsVals =
        reinterpret_cast<const uint32_t*>(offsetsData.constData());
    size_t numOffsets = offsetsData.size() / sizeof(uint32_t);

    // Create index buffer
    std::vector<uint32_t> indexBuffer;
    indexBuffer.reserve(connectivityData.size() / sizeof(uint32_t) * 1.5);

    // Populate index buffer using std::vector's push_back
    size_t current_conn_idx = 0;
    for (size_t i = 0; i < numOffsets; ++i) {
        size_t offset = offsetsVals[i];
        size_t num_verts_in_cell = offset - current_conn_idx;

        if (num_verts_in_cell == 3) {
            indexBuffer.push_back(connectivityVals[current_conn_idx]);
            indexBuffer.push_back(connectivityVals[current_conn_idx + 1]);
            indexBuffer.push_back(connectivityVals[current_conn_idx + 2]);
        } else if (num_verts_in_cell > 3) {
            uint32_t anchor_index = connectivityVals[current_conn_idx];
            for (size_t j = 1; j < num_verts_in_cell - 1; ++j) {
                indexBuffer.push_back(anchor_index);
                indexBuffer.push_back(connectivityVals[current_conn_idx + j]);
                indexBuffer.push_back(
                    connectivityVals[current_conn_idx + j + 1]);
            }
        }
        current_conn_idx = offset;
    }

    // Populate the render data structure
    RenderData renderData;
    renderData.format = RenderType::Color;
    renderData.data = std::move(vertexBuffer);
    renderData.indices = std::move(indexBuffer);
    renderData.boundingBoxMin = bbMin;
    renderData.boundingBoxMax = bbMax;
    return renderData;
}
