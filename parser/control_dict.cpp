#include <QDebug>
#include <QMetaEnum>
#include <QRegularExpression>

#include "control_dict.h"

CaseIO::ControlConfig CaseIO::parseControlDict(
    std::shared_ptr<OpenFoamDictionary> dict) {

    ControlConfig cfg;
    if (!dict) { return cfg; }

    // application
    cfg.solver = dict->getString("application");

    // startFrom
    QString startFromStr = dict->getString("startFrom");
    if (!startFromStr.isEmpty()) {
        bool ok = false;
        int enumValue =
            QMetaEnum::fromType<CaseIO::StartSolverType>().keyToValue(
                startFromStr.toUtf8().constData(), &ok);
        if (ok) {
            cfg.startFrom = static_cast<CaseIO::StartSolverType>(enumValue);
        }
    }

    // startTime
    double sTime = dict->getNumber("startTime");
    if (!std::isnan(sTime)) {
        cfg.startTime = sTime;
    }

    // stopAt
    QString stopAtStr = dict->getString("stopAt");
    if (!stopAtStr.isEmpty()) {
        bool ok = false;
        int enumValue = QMetaEnum::fromType<CaseIO::EndSolverType>().keyToValue(
            stopAtStr.toUtf8().constData(), &ok);
        if (ok) {
            cfg.stopAt = static_cast<CaseIO::EndSolverType>(enumValue);
        }
    }

    // endTime and deltaT
    double eTime = dict->getNumber("endTime");
    if (!std::isnan(eTime)) {
        cfg.endTime = eTime;
    }
    double dT = dict->getNumber("deltaT");
    if (!std::isnan(dT)) {
        cfg.deltaT = dT;
    }

    /*
    // Time step adjustment
    QString adjustStr = dict->getString("adjustTimeStep").toLower();
    config.adjustTimeStep = (adjustStr == "yes" || adjustStr == "true" || adjustStr == "on");

    double maxCourant = dict->getNumber("maxCo");
    if (!std::isnan(maxCourant)) {
        config.maxCo = maxCourant;
    }
    */

    // writeControl
    QString writeControlStr = dict->getString("writeControl");
    if (!writeControlStr.isEmpty()) {
        bool ok = false;
        int enumValue =
            QMetaEnum::fromType<CaseIO::WriteControlType>().keyToValue(
                writeControlStr.toUtf8().constData(), &ok);
        if (ok) {
            cfg.writeControl = static_cast<CaseIO::WriteControlType>(enumValue);
        }
    }

    // purgeWrite
    double purgeW = dict->getNumber("purgeWrite");
    if (!std::isnan(purgeW)) {
        cfg.purgeWrite = static_cast<int>(purgeW);
    }

    // writeCompression
    QString writeCompressionStr = dict->getString("writeCompression").toLower();
    if (!writeCompressionStr.isEmpty()) {
        cfg.writeCompression = (writeCompressionStr == "yes"
                                || writeCompressionStr == "true"
                                || writeCompressionStr == "on");
    }

    // runTimeModifiable
    QString runTimeModifiableStr = dict->getString("runTimeModifiable").toLower();
    if (!runTimeModifiableStr.isEmpty()) {
        cfg.runTimeModifiable = (runTimeModifiableStr == "yes"
                                 || runTimeModifiableStr == "true"
                                 || runTimeModifiableStr == "on");
    }

    // writeFormat
    QString writeFormatStr = dict->getString("writeFormat");
    cfg.writeFormat =
        CaseIO::WriteFormatType(
            QMetaEnum::fromType<CaseIO::WriteFormatType>().keyToValue(
                writeFormatStr.toUtf8().constData()));

    // writeInterval
    double writeIntervalNum = dict->getNumber("writeInterval");
    if (!std::isnan(writeIntervalNum)) {
        cfg.writeInterval = writeIntervalNum;
    }
    return cfg;
}

QString CaseIO::updateControlDict(std::shared_ptr<OpenFoamDictionary> dict,
                          ControlConfig& cfg, QString funcString) {
    if (!dict)
        return QString();

    auto boolToString = [](bool value) {
        return value ? QString("true") : QString("false");
    };

    // Update solver application
    dict->setValue("application", cfg.solver);

    // Update Run Control
    dict->setValue("startFrom", enumToString(cfg.startFrom, "startTime"));
    dict->setValue("startTime", QString::number(cfg.startTime));
    dict->setValue("stopAt", enumToString(cfg.stopAt, "endTime"));
    dict->setValue("endTime", QString::number(cfg.endTime));
    dict->setValue("deltaT", QString::number(cfg.deltaT));

    // Update Time Step Adjustment
    dict->setValue("adjustTimeStep", boolToString(cfg.adjustTimeStep));
    dict->setValue("maxCo", QString::number(cfg.maxCo));

    // Update Data Writing
    dict->setValue("writeCompression", boolToString(cfg.writeCompression));
    dict->setValue("runTimeModifiable", boolToString(cfg.runTimeModifiable));
    dict->setValue("writeFormat", enumToString(cfg.writeFormat, "binary"));
    dict->setValue("writeControl", enumToString(cfg.writeControl, "timeStep"));
    dict->setValue("writeInterval", QString::number(cfg.writeInterval));
    dict->setValue("purgeWrite", QString::number(cfg.purgeWrite));

    // Handle the functions block
    dict->removeEntry("functions");

    // Extract the updated raw text
    QString updatedText = QString::fromUtf8(dict->getRawText());

    // Append the new functions block if a valid string is provided
    if (!funcString.isEmpty()) {
        if (!updatedText.endsWith('\n')) {
            updatedText += "\n";
        }
        updatedText += "\n" + funcString + "\n";
    }

    return updatedText;
}

QString CaseIO::createControlDict(const ControlConfig& cfg,
    const QString& openFoamPath, const QString& functionObjects) {

    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << createFoamHeader("controlDict", openFoamPath);

    auto writeEntry = [&out](const QString& keyword, const QString& value) {
        out << keyword.leftJustified(20, ' ') << value << ";\n";
    };

    // Helper lambda for booleans
    auto toFoamBool = [](bool val) { return val ? "yes" : "no"; };

    // Application
    out << "// Simulation executable\n";
    writeEntry("application", cfg.solver);
    out << "\n";

    // Time control
    out << "// Time control\n";
    writeEntry("startFrom", enumToString(cfg.startFrom, "startTime"));
    writeEntry("startTime", QString::number(cfg.startTime));
    writeEntry("stopAt", enumToString(cfg.stopAt, "endTime"));
    writeEntry("endTime", QString::number(cfg.endTime));
    writeEntry("deltaT", QString::number(cfg.deltaT));
    out << "\n";

    // Data writing
    out << "// Data writing\n";
    writeEntry("writeControl", enumToString(cfg.writeControl, "timeStep"));
    writeEntry("writeInterval", QString::number(cfg.writeInterval));
    writeEntry("purgeWrite", QString::number(cfg.purgeWrite));
    out << "\n";

    // Output format
    out << "// Output format\n";
    writeEntry("writeFormat", enumToString(cfg.writeFormat, "binary"));
    writeEntry("writePrecision", "6");
    writeEntry("writeCompression", cfg.writeCompression ? "on" : "off");
    writeEntry("timeFormat", "general");
    writeEntry("timePrecision", "6");
    out << "\n";

    // Standard default entries
    out << "// Runtime configuration\n";
    writeEntry("runTimeModifiable", toFoamBool(cfg.runTimeModifiable));

    // Write function objects
    out << "\n\n" << functionObjects;

    // Write closing separator
    out << "\n\n" << createFoamFooter();
    return dictStr;
}