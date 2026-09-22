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

#include <cstdio>

#include <QApplication>
#include <QDate>
#include <QMessageLogContext>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStyleFactory>
#include <QTranslator>

#include "./main_window.h"

// Global variables for the logger
QString g_logFilePath;
QMutex g_logMutex;

// The Custom Message Handler
void flowComputeLogHandler(QtMsgType type, const QMessageLogContext &context,
                           const QString &msg) {
    // Lock the mutex to ensure thread safety
    QMutexLocker locker(&g_logMutex);

    // Format the log level string
    QString levelText;
    switch (type) {
        case QtDebugMsg:    levelText = "DEBUG"; break;
        case QtInfoMsg:     levelText = "INFO "; break;
        case QtWarningMsg:  levelText = "WARN "; break;
        case QtCriticalMsg: levelText = "CRIT "; break;
        case QtFatalMsg:    levelText = "FATAL"; break;
    }

    // Format the final message
    QString currentDateTime =
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString logMessage =
        QString("[%1] [%2] %3").arg(currentDateTime, levelText, msg);

    // Write to the file
    QFile file(g_logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&file);
        stream << logMessage << "\n";
        file.close();
    }

    fprintf(stderr, "%s\n", logMessage.toLocal8Bit().constData());
    fflush(stderr);
}

// Copy configuration files
void initializeConfig(QApplication& app) {
    // Determine the writable directory path for JSON configs
    QString configDirPath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configDirPath);

    if (!configDir.exists()) {
        configDir.mkpath(".");
    }

    // Write locale to QSettings if missing
    QSettings settings;
    QString localeCode;
    if (!settings.contains("Preferences/language")) {
        localeCode = QLocale::system().name();
        settings.setValue("Preferences/language", localeCode);
    } else {
        localeCode = settings.value("Preferences/language").toString();
    }

    // Load translation efficiently
    if (!localeCode.startsWith("en")) {
        QTranslator* appTranslator = new QTranslator();
        if (appTranslator->load(localeCode + ".qm", ":/translations")) {
            appTranslator->setParent(&app);
            app.installTranslator(appTranslator);
        } else {
            qWarning() << "Failed to load translation for" << localeCode;
            delete appTranslator;
        }
    }

    QStringList configFiles = {
        "solvers.json", "turbulence.json", "fields.json",
        "boundary_conditions.json", "material_properties.json"
    };

    // Deploy configuration files
    for (const auto& configFile : configFiles) {
        QString writableFilePath = configDir.filePath(configFile);
        QFileInfo fileInfo(writableFilePath);

        if (!fileInfo.exists()) {
            QString resourceFilePath = ":/config/" + configFile;
            if (QFile::copy(resourceFilePath, writableFilePath)) {
                QFile::setPermissions(writableFilePath,
                  QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                      QFileDevice::ReadUser | QFileDevice::WriteUser);
            } else {
                qCritical() << "Failed to deploy config file:" << configFile;
            }
        }
    }
}

int main(int argc, char *argv[]) {

    // Set application properties
    QApplication app(argc, argv);
    app.setOrganizationName("FlowCompute");
    app.setApplicationName("FlowCompute");
    app.setApplicationVersion(APP_VERSION);

    // Global defaults
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // Set icon
    #if defined(Q_OS_WIN)
        app.setWindowIcon(QIcon(":/images/flowcompute.ico"));
    #else
        app.setWindowIcon(QIcon(":/images/flowcompute.png"));
    #endif

    /*
    // Set Logging Directory
    QString appDataDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataDir);

    // Ensure the directory exists before trying to create a file inside it
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Set the log file path and install the handler
    g_logFilePath = dir.absoluteFilePath("FlowCompute.log");
    qInstallMessageHandler(flowComputeLogHandler);

    // Test the logger
    qInfo() << "FlowCompute Client Application Started.";
    qDebug() << "Log file initialized at:" << g_logFilePath;
    */

    // Copy solvers.json and turbulence.json
    initializeConfig(app);

    // Create window
    MainWindow mainWindow;
    mainWindow.showMaximized();
    return app.exec();
}
