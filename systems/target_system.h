#ifndef TARGET_SYSTEM_H
#define TARGET_SYSTEM_H

#include <QObject>
#include <QString>
#include <QStringList>

// Abstract class that represents a target running OpenFOAM
class TargetSystem: public QObject {
    Q_OBJECT

public:
    virtual ~TargetSystem() = default;

    virtual QStringList findOpenFoam() = 0;
    virtual QStringList getTutorials(QString path) = 0;
    virtual QStringList getHomeFolders() = 0;
    virtual QStringList getFiles(QString path) = 0;
    virtual QStringList copyTutorialFolders(QString tutPath, QString projPath) = 0;
    virtual QString checkPath(QString projPath) = 0;
    virtual QByteArray getFileContent(QString path) = 0;
    virtual bool writeData(const QByteArray& payload, const QString& remoteFilePath) = 0;
    virtual bool writeData(const QString& localPath, const QString& remoteFilePath) = 0;
    virtual void launchUtility(const QString& cmd) = 0;
    virtual bool createDirectories(QStringList dirPaths) = 0;
};

#endif // TARGET_SYSTEM_H