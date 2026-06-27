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

void LocalSystem::launchLongUtility(const QString& cmd, const QString& caseName, UtilityType utilityType) {

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

QStringList LocalSystem::copyTutorialFolders(QString tutPath, QString projPath) {
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

QString LocalSystem::getResultFolders(QString projPath) {
    return "";
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

RenderData LocalSystem::getResultData(QString path) {
    RenderData result;
    return result;
}
