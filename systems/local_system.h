#ifndef LOCAL_SYSTEM_H
#define LOCAL_SYSTEM_H

#include "target_system.h"

class LocalSystem : public TargetSystem {
    Q_OBJECT

public:
    LocalSystem() {};
    ~LocalSystem() {};

    QStringList findOpenFoam() override;
    QStringList getTutorials(QString path) override;
    QStringList getHomeFolders() override;
    QStringList getFiles(QString path) override;
    QStringList copyTutorialFolders(QString tutPath, QString projPath) override;
    QString checkPath(const QString& basePath) override;
    QByteArray getFileContent(const QString& path) override;
    RenderData getResultData(const QString& path) override;
    bool writeData(const QByteArray& payload, const QString& remoteFilePath) override;
    bool writeData(const QString& localPath, const QString& remoteFilePath) override;
    int launchShortUtility(const QString& cmd, QString& output) override;
    void launchLongUtility(const QString& cmd, const QString& caseName, UtilityType utilityType) override;
    bool createDirectories(const QStringList& dirPaths) override;
    QString getResultFolders(QString path) override;

signals:
    // Emitted every time a chunk of console output arrives
    void longUtilityOutputReceived(const QString& outputChunk);

    // Emitted when the process finally completes or fails
    void longUtilityFinished(const QString& status, const QString& caseName, UtilityType type);

    // Emitted if there is a network or parsing failure
    void longUtilityError(const QString& errorMessage);
};

#endif // LOCAL_SYSTEM_H
