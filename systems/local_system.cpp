#include "local_system.h"

#include <filesystem>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

namespace fs = std::filesystem;

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

    // 1. Allocate QProcess on the heap so it runs asynchronously and survives this scope
    QProcess* process = new QProcess(this);

    // Merge standard error into standard output, mimicking "2>&1"
    process->setProcessChannelMode(QProcess::MergedChannels);

    // 2. Stream the output live as the process runs
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
        // Read line-by-line to exactly match your previous socket->readLine() behavior
        while (process->canReadLine()) {
            QByteArray line = process->readLine();
            // Emit the raw output chunk just like the "running" JSON state did
            emit longUtilityOutputReceived(QString::fromUtf8(line).trimmed());
        }
    });

    // 3. Handle process completion (success or failure)
    // Using a modern lambda connection. (Note: if using Qt 5, QProcess::finished is overloaded)
    connect(process, &QProcess::finished, this, [this, caseName, utilityType](int exitCode, QProcess::ExitStatus exitStatus) {

        QString status = "error";

        // Check for a normal crash-free exit AND a bash success code (0)
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            status = "success";
        }

        // Emit the final completion state
        emit longUtilityFinished(status, caseName, utilityType);
    });

    // 4. Handle process-level execution errors (e.g., failed to start)
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        emit longUtilityError("Process Error: " + process->errorString());
    });

    // 5. Clean up memory automatically when the process finishes
    connect(process, &QProcess::finished, process, &QObject::deleteLater);

    // 6. Start the process non-blockingly
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
                    ofList.append(QString::fromStdString(entry.path().string()));
                }
            }
        }
    }
    return ofList;
}

QStringList LocalSystem::getTutorials(QString path) {
    return {};
}

void findDirectories(const QString& currentPath, int currentDepth, QStringList& result) {
    QDir dir(currentPath);

    // Filter directories
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    // Access list of files
    QFileInfoList entries = dir.entryInfoList();

    for (const QFileInfo& entry : std::as_const(entries)) {
        QString dirName = entry.fileName();

        // Skip hidden directories
        if (dirName.startsWith('.')) {
            continue;
        }
        result.append(entry.filePath());
        if (currentDepth < 2) {
            findDirectories(entry.filePath(), currentDepth + 1, result);
        }
    }
}

// Get folders in the home directory
QStringList LocalSystem::getHomeFolders() {

    // Access the home directory
    QString targetPath = std::getenv("HOME");

    // Find directories recursively
    QStringList resultList;
    QFileInfo targetInfo(targetPath);
    if (targetInfo.exists() && targetInfo.isDir()) {
        findDirectories(targetPath, 0, resultList);
    }
    return resultList;
}

QStringList LocalSystem::getFiles(QString path) {
    QStringList results;

    // Check if the path exists and is a directory
    QFileInfo targetInfo(path);
    if (targetInfo.exists() && targetInfo.isDir()) {
        QDir dir(path);

        // Create filter
        dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        // Iterate through entries
        QFileInfoList entries = dir.entryInfoList();
        for (const QFileInfo& entry : std::as_const(entries)) {
            // Extract just the name of the file or folder
            QString item_name = entry.fileName();

            // Skip hidden files and directories
            if (item_name.startsWith('.')) {
                continue;
            }

            // Skip specified STL files
            if (item_name.endsWith("_patched.stl") || item_name.endsWith("_tmp.stl")) {
                continue;
            }

            // If it's a regular file, append the pipe character
            if (entry.isFile()) {
                item_name += "|";
            }

            // Add the processed string to our container
            results.append(item_name);
        }
    }
    return results;
}

QStringList LocalSystem::copyTutorialFolders(QString tutPath, QString casePath) {
    return {};
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

bool LocalSystem::createDirectories(const QStringList& dirPaths) {

    if (dirPaths.isEmpty()) {
        return false;
    }

    // Attempt to create directories
    QDir dir;
    for (const QString& dirPath : dirPaths) {
        if (!dir.mkpath(dirPath)) {
            return false;
        }
    }
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

QString LocalSystem::checkPath(const QString& basePath) {

    // Use the static exists() method for a faster initial check
    if (!QFileInfo::exists(basePath)) {
        return "0";
    }

    int count = 1;
    while (count < 100000) {
        QString newPath = basePath + "_" + QString::number(count);

        // Check if new file exists
        if (!QFileInfo::exists(newPath)) {
            return QString::number(count);
        }
        count++;
    }
    return "-1";
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
    QFileInfoList entries = targetDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : std::as_const(entries)) {
        QString dir_name = entry.fileName();

        // Check for a valid OpenFOAM numeric time step
        bool ok = false;
        double time_val = dir_name.toDouble(&ok);

        // QString::toDouble sets 'ok' to true only if the ENTIRE string is a valid number.
        // This inherently replaces your std::stod try/catch block and length checking.
        if (ok) {
            QDir subDir(entry.filePath());

            // Second pass: Check if this time directory contains any .vtp file
            // Setting a name filter pushes the filtering down to the OS level, which is much faster.
            subDir.setNameFilters(QStringList() << "*.vtp");
            subDir.setFilter(QDir::Files);

            // If the filtered list isn't empty, it contains at least one .vtp file
            if (!subDir.entryList().isEmpty()) {
                sorted_folders.insert(time_val, dir_name);
            }
        }
    }

    // Extract the sorted string values into a list and construct the comma-separated string
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

    // 1. Ensure we read past the opening tag's closing bracket '>'
    int tagEnd = line.indexOf('>');
    while (tagEnd == -1) {
        if (!stream.readLineInto(&line)) return {};
        tagEnd = line.indexOf('>');
    }

    // Grab any Base64 data that might share the line with the '>'
    if (tagEnd + 1 < line.size()) {
        base64_payload += line.mid(tagEnd + 1);
    }

    // 2. Accumulate lines until we find the closing tag
    while (!base64_payload.contains("</DataArray>")) {
        QString nextLine;
        if (!stream.readLineInto(&nextLine)) return {};
        base64_payload += nextLine;
    }

    // 3. Strip the closing tag from the accumulated payload
    int closeTagPos = base64_payload.indexOf("</DataArray>");
    if (closeTagPos != -1) {
        base64_payload.truncate(closeTagPos);
    }

    // 4. Native Qt Base64 decoding
    QByteArray decoded = QByteArray::fromBase64(base64_payload.toUtf8());

    // 5. VTK Appended/Inline data always prepends an 8-byte (uint64) size header.
    // We remove it here so the returned array contains strictly the float/integer data.
    if (decoded.size() >= 8) {
        decoded.remove(0, 8);
    }

    return decoded;
}

std::pair<float, float> get_pressure_range(const QByteArray& pData) {
    // Check if we have at least one float
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

    bool pointsFound = false, connectivityFound = false, offsetsFound = false, pFound = false;
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

        if (!pointsFound && (line.contains("'Points'") || line.contains("\"Points\""))) {
            pointsData = extract_vtk_payload(stream, line);
            pointsFound = true;
        }
        else if (!connectivityFound && (line.contains("'connectivity'") || line.contains("\"connectivity\""))) {
            connectivityData = extract_vtk_payload(stream, line);
            connectivityFound = true;
        }
        else if (!offsetsFound && (line.contains("'offsets'") || line.contains("\"offsets\""))) {
            offsetsData = extract_vtk_payload(stream, line);
            offsetsFound = true;
        }
        else if (!pFound && (line.contains("'p'") || line.contains("\"p\""))) {
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
    if (static_cast<size_t>(pointsData.size()) != numPoints * 3 * sizeof(float)) {
        qWarning() << "Coordinate and pressure arrays contain different numbers of points.";
        return {};
    }

    // Allocate the STL interleaved vertex buffer (4 floats per vertex: x, y, z, p)
    std::vector<float> vertexBuffer(numPoints * 4);
    float* destPtr = vertexBuffer.data();

    // Cast the raw byte arrays to float pointers for direct access
    const float* rawPoints = reinterpret_cast<const float*>(pointsData.constData());
    const float* rawPressures = reinterpret_cast<const float*>(pData.constData());

    // Initialize STL array bounding boxes
    std::array<float, 3> bbMin = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    std::array<float, 3> bbMax = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

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

    // Directly interpret the byte arrays as uint32 arrays
    const uint32_t* connectivityVals = reinterpret_cast<const uint32_t*>(connectivityData.constData());
    const uint32_t* offsetsVals = reinterpret_cast<const uint32_t*>(offsetsData.constData());
    size_t numOffsets = offsetsData.size() / sizeof(uint32_t);

    // Create STL index buffer
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
        }
        else if (num_verts_in_cell > 3) {
            uint32_t anchor_index = connectivityVals[current_conn_idx];
            for (size_t j = 1; j < num_verts_in_cell - 1; ++j) {
                indexBuffer.push_back(anchor_index);
                indexBuffer.push_back(connectivityVals[current_conn_idx + j]);
                indexBuffer.push_back(connectivityVals[current_conn_idx + j + 1]);
            }
        }
        current_conn_idx = offset;
    }

    RenderData renderData;
    renderData.format = RenderType::Color;
    renderData.data = std::move(vertexBuffer);
    renderData.indices = std::move(indexBuffer);
    renderData.boundingBoxMin = bbMin;
    renderData.boundingBoxMax = bbMax;
    return renderData;
}
