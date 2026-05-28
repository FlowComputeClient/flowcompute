#ifndef WSL_SYSTEM_H
#define WSL_SYSTEM_H

#include "target_system.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include <QTcpSocket>
#include <QVBoxLayout>

#include <string>
#include <vector>

class WslSystem : public TargetSystem {
    Q_OBJECT

public:
    WslSystem() {};
    ~WslSystem() {};

    bool checkDistributions();
    QStringList findOpenFoam() override;
    QStringList getTutorials(QString path) override;
    QStringList getHomeFolders() override;
    QStringList getFiles(QString path) override;
    QStringList copyTutorialFolders(QString tutPath, QString projPath) override;
    QString checkPath(QString projPath) override;
    QByteArray getFileContent(QString path) override;
    bool writeData(const QByteArray& payload, const QString& remoteFilePath) override;
    bool writeData(const QString& localPath, const QString& remoteFilePath) override;
    int launchShortUtility(const QString& cmd, QString& output) override;
    bool createDirectories(QStringList dirPaths) override;

signals:
    void newLogLineReceived(QString line, UtilityType type);

private:

    // Add this non-virtual helper
    void terminateProcess();
    QString createSelectionDialog(const std::vector<std::string>& paths);
    QJsonObject contactServer(QString, QString);
};

#endif // WSL_SYSTEM_H