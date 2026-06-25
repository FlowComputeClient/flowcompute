#include "wsl_system.h"
#include <QDebug>

#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QDebug>

QJsonObject WslSystem::contactServer(QString action, QString message) {
    QJsonObject result;

    // Create socket
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 8080);

    // Wait for connection
    if (socket.waitForConnected(3000)) {
        QJsonObject request;
        request["action"] = action;
        request["message"] = message;

        socket.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");

        bool lineReady = false;
        while (!lineReady && socket.waitForReadyRead(3000)) {
            while (socket.canReadLine()) {
                lineReady = true;
                QByteArray data = socket.readLine();

                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

                if (parseError.error != QJsonParseError::NoError) {
                    qWarning() << "JSON Parse Error:" << parseError.errorString();
                    continue;
                }

                result = doc.object();
                if (result["status"].toString() == "success") {
                    return result;
                } else {
                    qWarning() << "Server returned error:" << result["message"].toString();
                }
            }
        }

        if (!lineReady) {
            qWarning() << "Timeout: Server never sent a complete response.";
        }
    } else {
        qWarning() << "Could not connect to WSL server on port 8080.";
    }
    return result;
}

// Launch a utility in the server
int WslSystem::launchShortUtility(const QString& cmd, QString& output) {
    QJsonObject result = contactServer("launchShortUtility", cmd);
    output = result["message"].toString();
    if (result["status"].toString() == "success") {
        return 0;
    } else {
        int code = result["exitCode"].toInt();
        return (code == 0) ? -1 : code;
    }
}

void WslSystem::launchLongUtility(const QString& cmd, const QString& caseName, UtilityType utilityType) {

    // 1. Allocate socket on the heap so it survives after the function returns
    QTcpSocket* socket = new QTcpSocket(this);

    // 2. Connect the socket's connection signal to trigger the request
    connect(socket, &QTcpSocket::connected, this, [socket, cmd]() {
        QJsonObject request;
        request["action"] = "launchLongUtility";
        request["message"] = cmd;
        socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    });

    // Connect the readyRead signal to process streaming chunks
    connect(socket, &QTcpSocket::readyRead, this, [this, socket, caseName, utilityType]() {
        while (socket->canReadLine()) {
            QByteArray data = socket->readLine();

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                emit longUtilityError("JSON Parse Error: " + parseError.errorString());
                continue;
            }

            QJsonObject result = doc.object();
            QString status = result["status"].toString();

            // Handle the streaming state
            if (status == "running") {
                emit longUtilityOutputReceived(result["output"].toString().trimmed());
            }
            // Handle the completion states
            else if (status == "success" || status == "error") {
                emit longUtilityFinished(status, caseName, utilityType);
                socket->disconnectFromHost();
            }
        }
    });

    // Handle network errors
    connect(socket, &QTcpSocket::errorOccurred, this, [this, socket](QTcpSocket::SocketError socketError) {
        emit longUtilityError("Socket Error: " + socket->errorString());
        socket->disconnectFromHost();
    });

    // Clean up the socket memory automatically when it disconnects
    connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);

    // Initiate the connection
    socket->connectToHost(QHostAddress::LocalHost, 8080);
}

QStringList WslSystem::findOpenFoam() {

    QStringList results;
    QJsonObject result = contactServer("checkOpenFoam", "");
    QJsonArray jsonArray = result["message"].toArray();
    for (const QJsonValue& value : std::as_const(jsonArray)) {
        if (value.isString()) {
            results.append(value.toString());
        }
    }
    return results;
}

QStringList WslSystem::getTutorials(QString path) {

    QStringList results;
    QJsonObject result = contactServer("findTutorials", path);
    QJsonArray jsonArray = result["message"].toArray();
    for (const QJsonValue& value : std::as_const(jsonArray)) {
        if (value.isString()) {
            results.append(value.toString());
        }
    }
    return results;
}

QStringList WslSystem::getHomeFolders() {

    QStringList results;
    QJsonObject result = contactServer("listDirectory", "");
    QJsonArray jsonArray = result["message"].toArray();
    for (const QJsonValue& value : std::as_const(jsonArray)) {
        if (value.isString()) {
            results.append(value.toString());
        }
    }
    return results;
}

QStringList WslSystem::getFiles(QString path) {
    QStringList results;
    QJsonObject result = contactServer("listFiles", path);
    QJsonArray jsonArray = result["message"].toArray();
    for (const QJsonValue& value : std::as_const(jsonArray)) {
        if (value.isString()) {
            results.append(value.toString());
        }
    }
    return results;
}

QStringList WslSystem::copyTutorialFolders(QString tutPath, QString projPath) {
    QStringList results;
    QJsonObject result = contactServer("copyTutorialFolders", tutPath + "," + projPath);
    QJsonArray jsonArray = result["message"].toArray();
    for (const QJsonValue& value : std::as_const(jsonArray)) {
        if (value.isString()) {
            results.append(value.toString());
        }
    }
    return results;
}

bool WslSystem::writeData(const QByteArray& payload, const QString& remoteFilePath) {
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 8080);

    if (!socket.waitForConnected(3000)) {
        qWarning() << "Could not connect to WSL server on port 8080.";
        return false;
    }

    // Send the JSON Header
    QJsonObject request;
    request["action"] = "writeData";
    request["message"] = remoteFilePath;
    request["byteSize"] = payload.size();

    socket.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");

    // Stream payload in chunks
    const qint64 chunkSize = 65536;
    qint64 bytesWrittenTotal = 0;

    while (bytesWrittenTotal < payload.size()) {
        qint64 bytesToWrite = qMin(chunkSize, payload.size() - bytesWrittenTotal);
        QByteArray chunk = payload.mid(bytesWrittenTotal, bytesToWrite);

        qint64 bytesWritten = socket.write(chunk);

        if (bytesWritten == -1) {
            qWarning() << "Socket write error during data transfer.";
            return false;
        }

        bytesWrittenTotal += bytesWritten;

        // Flush the socket buffer to prevent local memory from exploding
        if (!socket.waitForBytesWritten(3000)) {
            qWarning() << "Timeout waiting for socket to flush bytes.";
            return false;
        }
    }

    // Wait for response
    bool responseRead = false;
    QJsonObject response;

    // Maintain the 5-second timeout to accommodate slow WSL disk I/O on large files
    while (!responseRead && socket.waitForReadyRead(5000)) {
        while (socket.canReadLine()) {
            QByteArray data = socket.readLine();
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                response = doc.object();
                responseRead = true;
            }
        }
    }

    if (responseRead && response["status"].toString() == "success") {
        return true;
    } else {
        qWarning() << "Server failed to save data:" << response["message"].toString();
        return false;
    }
}

bool WslSystem::createDirectories(QStringList dirPaths) {

    QJsonObject result = contactServer("createDirectories", dirPaths.join("|"));
    QString status = result["status"].toString();
    QJsonArray jsonArray = result["message"].toArray();

    if (status == "success") {
        return true;
    } else {
        return false;
    }
}

bool WslSystem::writeData(const QString& localPath, const QString& remoteFilePath) {
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open local file:" << localPath;
        return false;
    }

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 8080);
    if (!socket.waitForConnected(3000)) {
        qWarning() << "Could not connect to WSL server on port 8080.";
        return false;
    }

    // Send the JSON Header
    QJsonObject request;
    request["action"] = "writeData";
    request["message"] = remoteFilePath;
    request["byteSize"] = file.size();
    socket.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");

    // Stream payload
    const qint64 chunkSize = 65536;
    while (!file.atEnd()) {
        QByteArray chunk = file.read(chunkSize);
        qint64 bytesWritten = socket.write(chunk);

        if (bytesWritten == -1) {
            qWarning() << "Socket write error during file transfer.";
            return false;
        }

        // Flush the socket buffer to prevent local memory from exploding
        if (!socket.waitForBytesWritten(3000)) {
            qWarning() << "Timeout waiting for socket to flush bytes.";
            return false;
        }
    }

    // Wait for server
    bool responseRead = false;
    QJsonObject response;

    // Use a slightly longer timeout in case the WSL disk is slow to write a massive STL
    while (!responseRead && socket.waitForReadyRead(5000)) {
        while (socket.canReadLine()) {
            QByteArray data = socket.readLine();
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

            if (parseError.error == QJsonParseError::NoError) {
                response = doc.object();
                responseRead = true;
            }
        }
    }

    if (responseRead && response["status"].toString() == "success") {
        return true;
    } else {
        qWarning() << "Server failed to save geometry file:" << response["message"].toString();
        return false;
    }
}

QString WslSystem::checkPath(QString projPath) {

    QJsonObject result = contactServer("checkPath", projPath);
    QString path = result["message"].toString();
    return path;
}

QString WslSystem::getResultFolders(QString projPath) {

    QJsonObject result = contactServer("getResultFolders", projPath);
    QString res = result["message"].toString();
    return res;
}

QByteArray WslSystem::getFileContent(QString path) {

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 8080);

    if (!socket.waitForConnected(3000)) {
        qWarning() << "Could not connect to WSL server on port 8080.";
        return QByteArray();
    }

    // Send the JSON Request
    QJsonObject request;
    request["action"] = "getFileContent";
    request["message"] = path;
    socket.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");

    // Read the JSON Header
    QJsonObject header;
    bool headerRead = false;

    while (!headerRead && socket.waitForReadyRead(3000)) {
        if (socket.canReadLine()) {
            QByteArray headerData = socket.readLine();
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(headerData, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qWarning() << "JSON Parse Error in Header:" << parseError.errorString();
                return QByteArray();
            }

            header = doc.object();
            headerRead = true;
        }
    }

    if (!headerRead || header["status"].toString() != "success") {
        qWarning() << "Server failed or returned error:" << header["message"].toString();
        return QByteArray();
    }

    // Extract Size and Prepare Buffer
    qint64 byteSize = header["byteSize"].toInt();
    QByteArray payload;

    // Pre-allocate memory to prevent reallocation overhead for large files
    payload.reserve(byteSize);

    // Read payload in chunks
    while (payload.size() < byteSize) {
        if (socket.bytesAvailable() > 0 || socket.waitForReadyRead(3000)) {
            qint64 bytesLeft = byteSize - payload.size();
            payload.append(socket.read(bytesLeft));
        } else {
            qWarning() << "Timeout: Server disconnected or stalled during payload transfer.";
            break;
        }
    }
    return payload;
}

RenderData WslSystem::getResultData(QString path) {
    RenderData result;
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 8080);

    if (!socket.waitForConnected(3000)) {
        qDebug() << "Could not connect to WSL server on port 8080.";
        return result;
    }

    // Send the JSON Request
    QJsonObject request;
    request["action"] = "getFileContent";
    request["message"] = path;
    socket.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");

    // Read the JSON Response
    bool jsonRead = false;
    while (!jsonRead && socket.waitForReadyRead(3000)) {
        if (socket.canReadLine()) {
            QByteArray jsonData = socket.readLine(); // Reads exactly up to the '\n'
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "JSON Parse Error: " + parseError.errorString();
                return result;
            }

            QJsonObject responseObj = doc.object();
            if (responseObj["status"].toString() != "success") {
                qDebug() << "Server error: " + responseObj["message"].toString();
                return result;
            }
            jsonRead = true;
        }
    }

    if (!jsonRead) {
        qDebug() << "Timeout waiting for JSON response header.";
        return result;
    }

    // Read the Binary Header
    ResultHeader binHeader;
    qint64 headerSize = sizeof(ResultHeader);
    qint64 headerBytesRead = 0;
    while (headerBytesRead < headerSize) {
        if (socket.bytesAvailable() > 0 || socket.waitForReadyRead(3000)) {
            qint64 chunk = socket.read(reinterpret_cast<char*>(&binHeader) + headerBytesRead,
                                       headerSize - headerBytesRead);
            if (chunk > 0) headerBytesRead += chunk;
        } else {
            qDebug() << "Timeout reading binary header.";
            return result;
        }
    }

    // Check the magic number
    if (binHeader.magicNumber != 0xFEEDBEEF) {
        qDebug() << "Protocol mismatch: Magic number invalid.";
        return result;
    }

    // Read the Vertex Payload
    result.data.resize(binHeader.dataByteSize);
    qint64 vertexBytesRead = 0;

    while (vertexBytesRead < binHeader.dataByteSize) {
        if (socket.bytesAvailable() > 0 || socket.waitForReadyRead(3000)) {
            qint64 chunk = socket.read(reinterpret_cast<char*>(result.data.data()) + vertexBytesRead,
                                       binHeader.dataByteSize - vertexBytesRead);
            if (chunk > 0) vertexBytesRead += chunk;
        } else {
            qDebug() << "Timeout reading vertex payload.";
            return result;
        }
    }

    // Read the Index Payload
    result.indices.resize(binHeader.indexByteSize / sizeof(uint32_t));
    qint64 indexBytesRead = 0;
    while (indexBytesRead < binHeader.indexByteSize) {
        if (socket.bytesAvailable() > 0 || socket.waitForReadyRead(3000)) {
            qint64 chunk = socket.read(reinterpret_cast<char*>(result.indices.data()) + indexBytesRead,
                                       binHeader.indexByteSize - indexBytesRead);
            if (chunk > 0) indexBytesRead += chunk;
        } else {
            qDebug() << "Timeout reading index payload.";
            return result;
        }
    }

    // Finalize
    result.format = RenderType::Color;
    result.boundingBoxMin = binHeader.boundingBoxMin;
    result.boundingBoxMax = binHeader.boundingBoxMax;
    return result;
}

// Check for installed Linux distributions
bool WslSystem::checkDistributions() {

    QString selectedDistribution;
    std::vector<std::string> distributions;

    // Use QProcess to query installed WSL distributions
    QProcess process;
    process.start("wsl", QStringList() << "--list" << "--quiet");

    if (process.waitForFinished(3000)) {
        QByteArray output = process.readAllStandardOutput();

        // wsl.exe outputs in UTF-16LE, so we must decode it accordingly
        QString strOutput = QString::fromUtf16(
            reinterpret_cast<const char16_t*>(output.constData()),
            output.size() / 2
            );

        // Split the output by newlines, skipping empty lines
        QStringList lines = strOutput.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

        for (const QString& line : std::as_const(lines)) {
            QString dist = line.trimmed();
            if (!dist.isEmpty()) {
                distributions.push_back(dist.toStdString());
            }
        }
    } else {
        qWarning() << "Failed to execute wsl --list --quiet or process timed out.";
        return false;
    }

    // Check what we found
    if (distributions.empty()) {
        qWarning() << "No WSL distributions found.";
        return false;
    }
    else if (distributions.size() == 1) {
        selectedDistribution = QString::fromStdString(distributions[0]);
    } else {
        selectedDistribution = createSelectionDialog(distributions);
    }

    // If the user cancelled the dialog, the string will be empty
    if (selectedDistribution.isEmpty()) {
        return false;
    }

    // Save the selected distribution to settings
    QSettings settings;
    settings.beginGroup("Environments");
    settings.beginGroup("WSL");
    settings.setValue("DISTRIBUTION", selectedDistribution);
    settings.endGroup();
    settings.endGroup();

    qDebug() << selectedDistribution;

    return true;
}

QString WslSystem::createSelectionDialog(const std::vector<std::string>& paths) {

    // Create the dialog
    QDialog dialog(nullptr);
    dialog.setWindowTitle(QObject::tr("Multiple WSL Distributions Detected"));

    // Layout for the dialog
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // Add label
    layout->addWidget(new QLabel(QObject::tr("Select the desired WSL distribution:"), &dialog));

    // Button group to manage exclusivity and retrieval
    QButtonGroup buttonGroup(&dialog);

    // Create radio buttons
    for (int i = 0; i < paths.size(); ++i) {
        QRadioButton *rb = new QRadioButton(QString::fromStdString(paths[i]), &dialog);
        layout->addWidget(rb);
        buttonGroup.addButton(rb, i);
        if (i == 0) {
            rb->setChecked(true);
        }
    }

    // OK/Cancel buttons
    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Execute the dialog
    if (dialog.exec() == QDialog::Accepted) {
        QAbstractButton *selected = buttonGroup.checkedButton();
        if (selected)
            return selected->text();
    }
    return "";
}