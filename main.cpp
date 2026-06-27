#include "main_window.h"

#include <QApplication>
#include <QDate>
#include <QMessageLogContext>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStyleFactory>

// Global variables for the logger
QString g_logFilePath;
QMutex g_logMutex;

// The Custom Message Handler
void flowComputeLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {

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

    // Format the final message: [Timestamp] [Level] Message
    QString currentDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString logMessage = QString("[%1] [%2] %3").arg(currentDateTime, levelText, msg);

    // Write to the file
    QFile file(g_logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&file);
        stream << logMessage << "\n";
        file.close();
    }

    // Optional: Echo to the console so you can still see it in your IDE
    fprintf(stderr, "%s\n", logMessage.toLocal8Bit().constData());
    fflush(stderr);
}

// Copy solvers.json and turbulence.json
void initializeConfig() {

    // Determine the writable directory path
    QString configDirPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configDirPath);

    // Ensure the directory exists
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }

    QStringList configFiles = { "solvers.json", "turbulence.json", "fields.json", "boundary_conditions.json",
                                "material_properties.json" };
    for (const auto& configFile: configFiles) {

        // Define the path for the writable JSON file
        QString writableFilePath = configDir.filePath(configFile);
        QFileInfo fileInfo(writableFilePath);
        if (!fileInfo.exists()) {
            QString resourceFilePath = ":/config/" + configFile;
            if (QFile::copy(resourceFilePath, writableFilePath)) {
                QFile::setPermissions(writableFilePath,
                    QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                    QFileDevice::ReadUser | QFileDevice::WriteUser);
                qDebug() << "Deployed config file to:" << writableFilePath;
            } else {
                qCritical() << "Failed to deploy config file!";
            }
        }
    }
}

int main(int argc, char *argv[]) {

    // Create application
    QApplication app(argc, argv);

    // Set icon
    #if defined(Q_OS_WIN)
        app.setWindowIcon(QIcon(":/images/flowcompute.ico"));
    #else
        app.setWindowIcon(QIcon(":/images/flowcompute.png"));
    #endif

    // Set application properties
    app.setOrganizationName("FlowCompute");
    app.setApplicationName("FlowCompute");
    app.setApplicationVersion("0.8.0");

    /*
    app.setStyleSheet("QToolTip { color: black; background-color: #F4F1EA; border: 1px solid gray; }");
    */
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // Set QSettings format to .ini
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // Set Logging Directory
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataDir);

    // Ensure the directory exists before trying to create a file inside it
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    /*
    // Set the log file path and install the handler
    g_logFilePath = dir.absoluteFilePath("FlowCompute.log");
    qInstallMessageHandler(flowComputeLogHandler);
    */

    // Copy solvers.json and turbulence.json
    initializeConfig();

    // Test the logger
    qInfo() << "FlowCompute Client Application Started.";
    qDebug() << "Log file initialized at:" << g_logFilePath;

    // Create window
    MainWindow mainWindow;
    mainWindow.showMaximized();
    return app.exec();
}
