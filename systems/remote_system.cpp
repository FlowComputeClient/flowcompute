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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

#include <libssh/sftp.h>
#include <fcntl.h>

namespace fs = std::filesystem;

// Free the session when the class is destroyed
RemoteSystem::~RemoteSystem() {
    if (m_session) {
        // Disconnect if the session is active
        if (ssh_is_connected(m_session)) {
            ssh_disconnect(m_session);
        }
        ssh_free(m_session);
        m_session = nullptr;
    }
}

// Establish the SSH session
bool RemoteSystem::establishSession(const QString& host, const QString& user,
        int port, const QString& passwd, QString& errorMessage,
        std::function<bool(const QString&, const QString&)> callback) {
    // Create session
    m_session = ssh_new();
    ssh_options_set(m_session, SSH_OPTIONS_HOST, host.toUtf8().constData());
    ssh_options_set(m_session, SSH_OPTIONS_USER, user.toUtf8().constData());
    ssh_options_set(m_session, SSH_OPTIONS_PORT, &port);

    // Connect to the remote system
    int rc = ssh_connect(m_session);
    if (rc != SSH_OK) {
        errorMessage = QObject::tr("Failed to connect to %1: %2").arg(
            host, QString::fromUtf8(ssh_get_error(m_session)));
        ssh_free(m_session);
        return false;
    }

    // Verify known hosts
    ssh_key serverKey = nullptr;
    int state = ssh_session_is_known_server(m_session);
    if (state == SSH_KNOWN_HOSTS_UNKNOWN || state == SSH_KNOWN_HOSTS_CHANGED) {
        // Retrieve the server's hash/fingerprint here
        unsigned char *hash = nullptr;
        size_t hlen;
        ssh_get_server_publickey(m_session, &serverKey);
        ssh_get_publickey_hash(
            serverKey, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);

        // Convert hash to hex string for the user to read
        char *hexa = ssh_get_hexa(hash, hlen);
        QString fingerprint = QString::fromUtf8(hexa);

        // Clean up libssh memory
        ssh_string_free_char(hexa);
        ssh_clean_pubkey_hash(&hash);
        ssh_key_free(serverKey);

        // This will block until the GUI thread returns a result
        if (callback && !callback(host, fingerprint)) {
            errorMessage = QObject::tr("Connection aborted: Host fingerprint "
                                       "rejected by user.");
            ssh_disconnect(m_session);
            ssh_free(m_session);
            return false;
        }

        // Update the hosts file
        if (ssh_session_update_known_hosts(m_session) != SSH_OK) {
            // Optional: Log a warning that writing to known_hosts failed
        }
    } else if (state != SSH_KNOWN_HOSTS_OK) {
        errorMessage =
            QObject::tr("Connection aborted: Host key state is invalid.");
        ssh_disconnect(m_session);
        ssh_free(m_session);
        return false;
    }

    // Attempt to authenticate with key and password
    rc = ssh_userauth_publickey_auto(m_session, nullptr, nullptr);
    if (rc != SSH_AUTH_SUCCESS && !passwd.isEmpty()) {
        rc = ssh_userauth_password(m_session, nullptr,
                                   passwd.toUtf8().constData());
    }

    // Respond to authentication error
    if (rc != SSH_AUTH_SUCCESS) {
        // Capture the authentication error
        errorMessage = QString::fromUtf8(ssh_get_error(m_session));
        ssh_disconnect(m_session);
        ssh_free(m_session);
        return false;
    }
    return true;
}

// Execute Bash command and capture output
QString RemoteSystem::execCommand(const QString& cmd) {
    ssh_channel channel = ssh_channel_new(m_session);
    if (!channel) return {};

    // Open channel
    if (ssh_channel_open_session(channel) != SSH_OK) {
        ssh_channel_free(channel);
        return {};
    }

    // Request execution of the shell command
    if (ssh_channel_request_exec(channel,
            cmd.toUtf8().constData()) != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return {};
    }

    // Read the standard output from the channel
    char buffer[1024];
    int nbytes;
    QString output;
    while ((nbytes =
            ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
        output.append(QString::fromUtf8(buffer, nbytes));
    }

    // Close and deallocate channel
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return output;
};

// Locate OpenFOAM installation(s)
QStringList RemoteSystem::findOpenFoam() {
    // Ensure the session is valid
    QStringList ofList;
    if (!m_session)
        return ofList;

    // Execute the command
    const char* cmd = "find /usr/lib/openfoam /opt -mindepth 1 -maxdepth 1 "
                      "-type d -name \"openfoam*\" 2>/dev/null";
    QStringList lines = execCommand(cmd).split('\n', Qt::SkipEmptyParts);
    for (const QString& line : std::as_const(lines)) {
        ofList.append(line.trimmed());
    }
    return ofList;
}

// Perform basic directory operations
QStringList RemoteSystem::processPaths(const QString& pathString,
                                       PathOperationType opType) {
    QStringList result;
    if (!m_session || !ssh_is_connected(m_session)) {
        return QStringList{"-2"};
    }

    // Handle LIST with an empty input string
    if (opType == PathOperationType::LIST && pathString.isEmpty()) {
        QString cmd =
            "H=$(echo $HOME); echo $H; "
            "ls -1p $H | grep -v '^\\.' | grep -v '_patched\\.stl$' | "
            "grep -v '_tmp\\.stl$' | sed 's/[^/]$/&|/' | sed 's/\\/$//'";
        return execCommand(cmd).split('\n', Qt::SkipEmptyParts);
    }

    // Error handling if input is empty for other operations
    if (pathString.isEmpty()) {
        return QStringList{"Input paths string was empty."};
    }

    // Rename or Copy operations
    if (opType == PathOperationType::RENAME ||
        opType == PathOperationType::COPY) {

        QStringList strList = pathString.split('\n', Qt::SkipEmptyParts);
        if (strList.size() < 2) {
            return QStringList{"-1", "Insufficient paths provided."};
        }

        QString srcPath = strList[0];
        QString dstPath = strList[1];

        // Sanitize inputs for remote execution
        srcPath.replace("'", "'\\''");
        dstPath.replace("'", "'\\''");

        QString cmd;
        if (opType == PathOperationType::RENAME) {
            cmd = QString("SRC='%1'; DST='%2'; "
                          "mv \"$SRC\" \"$DST\" 2>/dev/null; "
                          "if [ $? -eq 0 ]; then echo '0'; else echo '-1'; fi")
                      .arg(srcPath, dstPath);
        } else if (opType == PathOperationType::COPY) {
            // -r for recursive directory copying, -f to force overwrite
            cmd = QString("SRC='%1'; DST='%2'; "
                          "cp -rf \"$SRC\" \"$DST\" 2>/dev/null; "
                          "if [ $? -eq 0 ]; then echo '0'; else echo '-1'; fi")
                      .arg(srcPath, dstPath);
        }

        QStringList cmdResult =
            execCommand(cmd).split('\n', Qt::SkipEmptyParts);

        // Return early
        if (!cmdResult.isEmpty()) {
            return QStringList{cmdResult.first()};
        }
        return QStringList{"-1"};
    }

    // Process the delimited string using Qt string splitting
    QStringList targetPaths = pathString.split('\n', Qt::SkipEmptyParts);

    for (const QString& qPath : std::as_const(targetPaths)) {
        QString safePath = qPath;
        safePath.replace("'", "'\\''");

        QString cmd;
        switch (opType) {
        case PathOperationType::RENAME:
            break;
        case PathOperationType::COPY:
            break;

        case PathOperationType::CREATE:
            cmd = QString("P='%1'; "
                  "if [ -f \"$P\" ]; then exit 0; fi; "
                  "mkdir -p \"$P\" 2>/dev/null; "
                  "if [ -d \"$P\" ]; then echo '0'; else echo '-1'; fi")
                .arg(safePath);
            break;

        // Checks if a given file exists
        case PathOperationType::CHECK:
            cmd = QString("if [ -e '%1' ]; then echo '0'; else echo '-1'; fi")
                      .arg(safePath);
            break;

        // Delete the given file
        case PathOperationType::REMOVE:
            cmd = QString("P='%1'; "
                  "if [ ! -e \"$P\" ]; then echo '-1'; else "
                  "rm -rf \"$P\" 2>/dev/null; "
                  "if [ ! -e \"$P\" ]; then echo '0'; else echo '-1'; fi; fi")
              .arg(safePath);
            break;

        // Get list of files in directory
        case PathOperationType::LIST:
            cmd = QString("P='%1'; "
              "if [ -d \"$P\" ]; then "
              "ls -1p \"$P\" | grep -v '^\\.' | grep -v '_patched\\.stl$' | "
              "grep -v '_tmp\\.stl$' | sed 's/[^/]$/&|/' | sed 's/\\/$//'; fi")
            .arg(safePath);
            break;
        }

        QStringList cmdResult =
            execCommand(cmd).split('\n', Qt::SkipEmptyParts);
        if (opType == PathOperationType::LIST) {
            result.append(cmdResult);
        } else {
            if (!cmdResult.isEmpty()) {
                result.append(cmdResult.first());
            }
        }
    }
    return result;
}

// Launch a utility in the server
int RemoteSystem::launchShortUtility(const QString& cmd, QString& output) {
    output.clear();
    if (cmd.isEmpty()) {
        output = "Error: Command is empty";
        return -1;
    }

    if (!m_session || !ssh_is_connected(m_session)) {
        output = "Error: SSH session is not established.";
        return -2;
    }

    ssh_channel channel = ssh_channel_new(m_session);
    if (!channel) {
        output = "Error: Failed to create SSH channel.";
        return -2;
    }

    if (ssh_channel_open_session(channel) != SSH_OK) {
        output = "Error: Failed to open SSH channel session.";
        ssh_channel_free(channel);
        return -2;
    }

    QString mergedCmd = cmd + " 2>&1";
    if (ssh_channel_request_exec(
            channel, mergedCmd.toUtf8().constData()) != SSH_OK) {
        output = "Error: Failed to execute command.";
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return -2;
    }

    // Read the merged standard output and error from the channel
    char buffer[1024];
    int nbytes;
    while ((nbytes =
            ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
        output.append(QString::fromUtf8(buffer, nbytes));
    }

    // Trim whitespace to match the QProcess implementation
    output = output.trimmed();

    // Fetch the bash exit code before closing the channel
    int exitCode = ssh_channel_get_exit_status(channel);

    // Clean up channel
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);

    // If the exit status was not sent by the server, it returns -1.
    if (exitCode == -1) {
        return -3;
    }
    return exitCode;
}

void RemoteSystem::launchLongUtility(const QString& cmd,
        const QString& caseName, UtilityType utilityType) {
}

QStringList RemoteSystem::getTutorials(const QString& base_path) {
    QStringList result;

    // Sanitize the input to prevent bash injection
    QString safePath = base_path;
    safePath.replace("'", "'\\''");

    // Construct the shell command
    QString cmd = QString(
        "TARGET='%1/tutorials'; "
        "if [ ! -d \"$TARGET\" ]; then "
        "  echo 'ERROR_NO_DIR'; "
        "else "
        "  find \"$TARGET\" -type d -name \"system\""
        " -exec dirname {} \\; 2>/dev/null; "
        "fi"
        ).arg(safePath);

    // Call the provided execCommand function
    QString output = execCommand(cmd);

    // Mimic the LocalSystem error handling if the path is invalid
    if (output.trimmed() == "ERROR_NO_DIR") {
        result.append("Tutorials path does not exist.");
        return result;
    }

    // Process output
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : std::as_const(lines)) {
        result.append(line.trimmed());
    }
    return result;
}

void RemoteSystem::processAllrunScript(const QString& scriptPath,
    const QString& projectPath, const QString& originalTutorialPath) {

    QString safeScript = scriptPath;
    safeScript.replace("'", "'\\''");

    // Retrieve script contents over SSH
    QString catCmd =
        QString("if [ -f '%1' ]; then cat '%1'; fi").arg(safeScript);
    QString content = execCommand(catCmd);

    if (content.isEmpty()) {
        return;
    }

    // Line continuation removal[cite: 14]
    QRegularExpression continuationRegex(R"(\\[ \t]*\r?\n\s*)");
    content.replace(continuationRegex, " ");

    // Find the base "tutorials" folder using string manipulation[cite: 14]
    int tutIndex = originalTutorialPath.lastIndexOf("/tutorials");
    if (tutIndex == -1) {
        emit logMessage("Could not deduce base 'tutorials' directory from: "
                        + originalTutorialPath);
        return;
    }

    QString tutorialsBase = originalTutorialPath.left(tutIndex + 10);
    QString sourceDir = tutorialsBase + "/resources/geometry";

    // Regex to find 'cp' commands targeting resources/geometry
    QRegularExpression cpRegex(
        R"REGEX(cp\s+(?:-[^\s]+\s+)*"?\$)REGEX"
        R"REGEX((?:FOAM_TUTORIALS|[{]FOAM_TUTORIALS[}])"?/)REGEX"
        R"REGEX(resources/geometry/([^\s"]+)"?\s+([^\s"]+)?"?)REGEX");
    QRegularExpressionMatchIterator i = cpRegex.globalMatch(content);

    // If no geometry copy commands are found, exit early
    if (!i.hasNext()) {
        return;
    }

    QString safeSourceDir = sourceDir;
    safeSourceDir.replace("'", "'\\''");
    QString safeProjPath = projectPath;
    safeProjPath.replace("'", "'\\''");

    // Build bash script to execute all matches at once
    QString compositeCmd = QString(
       "SRC='%1'; PROJ='%2'; "
       "if [ ! -d \"$SRC\" ]; then echo \"ERROR_NO_GEOM_DIR\"; exit 0; fi; "
       ).arg(safeSourceDir, safeProjPath);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString geomPattern = match.captured(1);
        QString destDirStr = match.captured(2);
        compositeCmd += QString(
            "DEST=\"$PROJ/%2\"; "
            "mkdir -p \"$DEST\"; "
            "MATCHED=0; "
            "for f in \"$SRC\"/%1; do "
            "  if [ -e \"$f\" ]; then "
            "    MATCHED=1; "
            "    fname=$(basename \"$f\"); "
            "    if [ -d \"$f\" ]; then "
            "      cp -r \"$f\" \"$DEST/$fname\"; "
            "      echo \"Copied geometry dir: $fname\"; "
            "    elif [[ \"$f\" == *.gz ]]; then "
            "      outname=\"${fname%.gz}\"; "
            "      gunzip -c \"$f\" > \"$DEST/$outname\"; "
            "      if [ $? -eq 0 ]; then "
            "        echo \"Decompressed: $outname\"; "
            "      else "
            "        echo \"Failed to decompress: $fname\"; "
            "      fi; "
            "    else "
            "      cp \"$f\" \"$DEST/$fname\"; "
            "      echo \"Copied geometry file: $fname\"; "
            "    fi; "
            "  fi; "
            "done; "
            "if [ $MATCHED -eq 0 ]; "
            "then echo \"No geometry files matched pattern: %1\"; fi; "
            ).arg(geomPattern, destDirStr);
    }

    // Execute the composite script
    QString cpOutput = execCommand(compositeCmd);

    // Handle geometry directory missing error
    if (cpOutput.startsWith("ERROR_NO_GEOM_DIR")) {
        emit logMessage("Geometry resources directory not found: " + sourceDir);
        return;
    }

    // Emit the logs generated by the remote Bash script
    QStringList logs = cpOutput.split('\n', Qt::SkipEmptyParts);
    for (const QString& log : std::as_const(logs)) {
        emit logMessage(log.trimmed());
    }
}

QStringList RemoteSystem::copyTutorialFolders(const QString& tutPath,
                                              const QString& projPath) {
    QStringList result;

    // Sanitize paths to prevent bash injection
    QString safeTut = tutPath;
    safeTut.replace("'", "'\\''");
    QString safeProj = projPath;
    safeProj.replace("'", "'\\''");

    // Prepare project directory and copy essential files
    QString setupCmd = QString(
       "TUT='%1'; PROJ='%2'; "
       "if [ ! -d \"$TUT\" ]; then echo 'ERROR_NO_TUT'; exit 0; fi; "
       "mkdir -p \"$PROJ\" && rm -rf \"$PROJ\"/*; "
       "for item in 0 0.orig constant system Allrun Allrun.pre Allclean; do "
       "  if [ -e \"$TUT/$item\" ]; then "
       "    cp -r \"$TUT/$item\" \"$PROJ/\"; "
       "    if [ -f \"$TUT/$item\" ]; then "
       "echo \"$item|\"; else echo \"$item\"; fi; "
       "  fi; "
       "done;"
       ).arg(safeTut, safeProj);

    QString setupOutput = execCommand(setupCmd);

    if (setupOutput.trimmed() == "ERROR_NO_TUT") {
        emit logMessage("Tutorial path does not exist or is not a directory.");
        return result;
    }

    // Capture the copied items to build the return list[cite: 14]
    QStringList copiedItems = setupOutput.split('\n', Qt::SkipEmptyParts);
    for (const QString& item : std::as_const(copiedItems)) {
        result.append(item.trimmed());
    }

    // 2. Parse both scripts to auto-resolve geometry dependencies[cite: 14]
    processAllrunScript(projPath + "/Allrun", projPath, tutPath);
    processAllrunScript(projPath + "/Allrun.pre", projPath, tutPath);

    // 3. Perform final system and constant checks[cite: 14]
    QString checkCmd = QString(
       "PROJ='%1'; "
       "if [ ! -d \"$PROJ/system\" ]; then echo 'WARN_NO_SYSTEM'; fi; "
       "if [ -d \"$PROJ/constant\" ]; then echo 'HAS_CONSTANT'; fi;"
       ).arg(safeProj);

    QString checkOutput = execCommand(checkCmd);

    if (checkOutput.contains("WARN_NO_SYSTEM")) {
        emit logMessage("Warning: No 'system' directory was found "
                        "in the tutorial folder.");
    }

    // Check and prepend "constant" if necessary
    if (checkOutput.contains("HAS_CONSTANT") && !result.contains("constant")) {
        result.prepend("constant");
    }
    return result;
}

bool RemoteSystem::writeData(const QByteArray& data, const QString& filePath) {
    // Ensure the session is valid
    if (!m_session || !ssh_is_connected(m_session)) {
        qWarning() << "SSH session is not established.";
        return false;
    }

    // 1. Allocate and initialize a new SFTP session
    sftp_session sftp = sftp_new(m_session);
    if (sftp == nullptr) {
        qWarning() << "Error allocating SFTP session:"
                   << ssh_get_error(m_session);
        return false;
    }

    if (sftp_init(sftp) != SSH_OK) {
        qWarning() << "Error initializing SFTP session:"
                   << sftp_get_error(sftp);
        sftp_free(sftp);
        return false;
    }

    // Open the remote file for writing
    sftp_file file = sftp_open(sftp, filePath.toUtf8().constData(),
        O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (file == nullptr) {
        qWarning() << "Failed to open remote file for writing:" << filePath;
        sftp_free(sftp);
        return false;
    }

    // Write data to the file
    const void* buffer = data.constData();
    size_t length = data.size();
    ssize_t written = sftp_write(file, buffer, length);

    // Close it
    sftp_close(file);
    sftp_free(sftp);

    // Make sure all bytes were written successfully
    if (written != static_cast<ssize_t>(length)) {
        qWarning() << "Failed to write all bytes to the remote file.";
        return false;
    }
    return true;
}

bool RemoteSystem::writeData(const QString& srcPath, const QString& dstPath) {
    QString actualSrcPath = srcPath;
    bool makeExecutable = false;

    // Check if the source path ends with '|' and remove it
    if (actualSrcPath.endsWith('|')) {
        actualSrcPath.chop(1);
        makeExecutable = true;
    }

    // Make sure the local source file exists
    QFileInfo srcInfo(actualSrcPath);
    if (!srcInfo.exists() || !srcInfo.isFile()) {
        return false;
    }

    if (!m_session || !ssh_is_connected(m_session)) {
        qWarning() << "SSH session is not established.";
        return false;
    }

    // Ensure the destination directory exists
    QFileInfo dstInfo(dstPath);
    QString dirPath = dstInfo.path();
    QString safeDir = dirPath;
    safeDir.replace("'", "'\\''");

    QString mkdirCmd = QString("mkdir -p '%1'").arg(safeDir);
    execCommand(mkdirCmd);

    // Initialize the SFTP session
    sftp_session sftp = sftp_new(m_session);
    if (sftp == nullptr) {
        return false;
    }

    if (sftp_init(sftp) != SSH_OK) {
        sftp_free(sftp);
        return false;
    }

    // Open remote file: 0755 for executable, 0644 otherwise
    int fileMode = makeExecutable ? 0755 : 0644;
    sftp_file remoteFile = sftp_open(sftp, dstPath.toUtf8().constData(),
                                     O_WRONLY | O_CREAT | O_TRUNC, fileMode);

    if (remoteFile == nullptr) {
        qWarning() << "Failed to open remote file:" << dstPath;
        sftp_free(sftp);
        return false;
    }

    // Open the local file for reading
    QFile localFile(actualSrcPath);
    if (!localFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open local file:" << actualSrcPath;
        sftp_close(remoteFile);
        sftp_free(sftp);
        return false;
    }

    // Transfer the file data in 16 KB chunks
    const qint64 chunkSize = 16384;
    char buffer[chunkSize];
    qint64 bytesRead;
    bool success = true;

    while ((bytesRead = localFile.read(buffer, chunkSize)) > 0) {
        ssize_t written = sftp_write(remoteFile, buffer, bytesRead);
        if (written != bytesRead) {
            qWarning() << "Error writing to remote file.";
            success = false;
            break;
        }
    }

    // Clean up resources
    localFile.close();
    sftp_close(remoteFile);
    sftp_free(sftp);

    // Make the file executable if necessary
    if (success && makeExecutable) {
        QString safeDst = dstPath;
        safeDst.replace("'", "'\\''");
        QString chmodCmd = QString("chmod +x '%1'").arg(safeDst);
        execCommand(chmodCmd);
    }

    return success;
}

std::pair<QStringList, QStringList>
    RemoteSystem::getTimesAndFields(const QString& projPath) {
    QStringList timeFolders, fieldFiles;

    // List project directory contents.
    QString cmd1 = QString("ls -1p \"%1\" 2>/dev/null").arg(projPath);
    QString output1 = execCommand(cmd1);
    if (output1.isEmpty()) {
        qWarning() << "Remote directory unavailable:" << projPath;
        return std::make_pair(timeFolders, fieldFiles);
    }

    // Split standard output by newline
    QStringList allEntries = output1.split('\n', Qt::SkipEmptyParts);
    std::map<double, QString> sortedFolders;

    for (QString entry : std::as_const(allEntries)) {
        entry = entry.trimmed();

        // Check if the entry is a directory
        if (entry.endsWith('/')) {
            entry.chop(1);
            bool isNumeric = false;
            double timeVal = entry.toDouble(&isNumeric);

            // Check for numeric folders
            if (isNumeric) {
                sortedFolders[timeVal] = entry;
            }
        }
    }

    // Populate the time folders list in sorted order
    for (const auto& [timeVal, dirName] : sortedFolders) {
        timeFolders.append(dirName);
    }

    // Find files in the latest time directory
    if (!sortedFolders.empty()) {
        QString latestTimeDir = sortedFolders.rbegin()->second;
        QString cmd2 = QString("ls -1p \"%1/%2\" 2>/dev/null")
                           .arg(projPath, latestTimeDir);
        QString output2 = execCommand(cmd2);

        if (!output2.isEmpty()) {
            QStringList latestEntries = output2.split('\n', Qt::SkipEmptyParts);
            for (QString entry : std::as_const(latestEntries)) {
                entry = entry.trimmed();

                // Check for regular file
                if (!entry.endsWith('/')) {
                    fieldFiles.append(entry);
                }
            }
        }
    }
    return std::make_pair(timeFolders, fieldFiles);
}

QByteArray RemoteSystem::getFileContent(const QString& path) {
    QByteArray fileData;

    // Check the session is valid
    if (!m_session || !ssh_is_connected(m_session)) {
        qWarning() << "SSH session is not established.";
        return {};
    }

    // Initialize the SFTP session
    sftp_session sftp = sftp_new(m_session);
    if (sftp == nullptr) {
        return {};
    }

    if (sftp_init(sftp) != SSH_OK) {
        sftp_free(sftp);
        return {};
    }

    // Make sure the path exists and is a file
    sftp_attributes attributes = sftp_stat(sftp, path.toUtf8().constData());
    if (attributes == nullptr) {
        sftp_free(sftp);
        return {};
    }

    // Path exists but is not a regular file
    if (attributes->type != SSH_FILEXFER_TYPE_REGULAR) {
        sftp_attributes_free(attributes);
        sftp_free(sftp);
        return {};
    }
    sftp_attributes_free(attributes);

    // Open the file as ReadOnly
    sftp_file file = sftp_open(sftp, path.toUtf8().constData(), O_RDONLY, 0);
    if (file == nullptr) {
        sftp_free(sftp);
        return {};
    }

    // Read data if it's a file
    char buffer[16384];
    ssize_t bytesRead;

    // Loop until EOF
    while ((bytesRead = sftp_read(file, buffer, sizeof(buffer))) > 0) {
        fileData.append(buffer, bytesRead);
    }

    if (bytesRead < 0) {
        qWarning() << "Error reading data from remote file:" << path;
    }

    // Close it
    sftp_close(file);
    sftp_free(sftp);
    return fileData;
}

RenderData RemoteSystem::getMeshData(const QString& path) {
    RenderData renderData;
    return renderData;
}

RenderData RemoteSystem::getResultData(const QString& path) {

    // Populate the render data structure
    RenderData renderData;
    return renderData;
}
