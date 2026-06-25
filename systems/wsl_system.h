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

struct ResultHeader {
    uint32_t magicNumber;
    uint32_t dataByteSize;
    uint32_t indexByteSize;
    std::array<float, 3> boundingBoxMin;
    std::array<float, 3> boundingBoxMax;
};

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
    RenderData getResultData(QString path) override;
    bool writeData(const QByteArray& payload, const QString& remoteFilePath) override;
    bool writeData(const QString& localPath, const QString& remoteFilePath) override;
    int launchShortUtility(const QString& cmd, QString& output) override;
    void launchLongUtility(const QString& cmd, const QString& caseName, UtilityType utilityType) override;
    bool createDirectories(QStringList dirPaths) override;
    QString getResultFolders(QString path) override;

signals:
    // Emitted every time a chunk of console output arrives
    void longUtilityOutputReceived(const QString& outputChunk);

    // Emitted when the process finally completes or fails
    void longUtilityFinished(const QString& status, const QString& caseName, UtilityType type);

    // Emitted if there is a network or parsing failure
    void longUtilityError(const QString& errorMessage);

private:

    // Add this non-virtual helper
    void terminateProcess();
    QString createSelectionDialog(const std::vector<std::string>& paths);
    QJsonObject contactServer(QString, QString);
};

#endif // WSL_SYSTEM_H