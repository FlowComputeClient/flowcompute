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

#include <QDebug>
#include <QMetaEnum>
#include <QRegularExpression>

#include "control_dict.h"

CaseIO::ControlConfig CaseIO::parseControlDict(
    std::shared_ptr<OpenFoamDictionary> dict) {
    ControlConfig cfg;
    if (!dict)
        return cfg;

    // application
    cfg.application = dict->getString("application");
    if (cfg.application.startsWith("foam")) {
        cfg.solver = dict->getString("solver");
    } else {
        cfg.solver = "";
    }

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

    // Time step adjustment
    QString adjustStr = dict->getString("adjustTimeStep").toLower();
    cfg.adjustTimeStep =
        (adjustStr == "yes" || adjustStr == "true" || adjustStr == "on");

    // Max Courant number
    double maxCourant = dict->getNumber("maxCo");
    if (!std::isnan(maxCourant)) {
        cfg.maxCo = maxCourant;
    }

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
    ControlConfig& cfg, const QString& funcString) {
    if (!dict)
        return QString();

    auto boolToString = [](bool value) {
        return value ? QString("true") : QString("false");
    };

    // Update solver application
    dict->setValue("application", cfg.application);
    if (!cfg.solver.isEmpty()) {
        dict->setValue("solver", cfg.solver);
    }

    // Update Run Control
    dict->setValue("startFrom", enumToString(cfg.startFrom, "startTime"));
    dict->setValue("startTime", QString::number(cfg.startTime));
    dict->setValue("stopAt", enumToString(cfg.stopAt, "endTime"));
    dict->setValue("endTime", QString::number(cfg.endTime));
    dict->setValue("deltaT", QString::number(cfg.deltaT));

    // Transient fields
    dict->setValue("adjustTimeStep", boolToString(cfg.adjustTimeStep), true);
    dict->setValue("maxCo", QString::number(cfg.maxCo), true);

    // Update Data Writing
    dict->setValue("writeCompression", boolToString(cfg.writeCompression));
    dict->setValue("runTimeModifiable", boolToString(cfg.runTimeModifiable));
    dict->setValue("writeFormat", enumToString(cfg.writeFormat, "binary"));
    dict->setValue("writeControl", enumToString(cfg.writeControl, "timeStep"));
    dict->setValue("writeInterval", QString::number(cfg.writeInterval));
    dict->setValue("purgeWrite", QString::number(cfg.purgeWrite));

    // Remove the functions block
    dict->removeEntry("functions");

    // Extract the updated raw text
    QString updatedText = QString::fromUtf8(dict->getRawText());

    // Insert the new functions block
    if (!funcString.isEmpty()) {
        // Search for spaced footer
        int spacedFooterPos =
            updatedText.lastIndexOf("// * * * * * * * * * * * * * * * * * * * "
                                    "* * * * * * * * * * * * * * * * * * //");
        int solidFooterPos = updatedText.lastIndexOf("// *********");

        // Select whichever string appears closest to the bottom of the file
        int footerPos = std::max(spacedFooterPos, solidFooterPos);

        // Verify the selected string is actually a footer and not the header
        int appPos = updatedText.indexOf("application");
        if (footerPos != -1 && appPos != -1 && footerPos < appPos) {
            footerPos = -1;
        }

        if (footerPos != -1) {
            // Ensure there is a newline
            if (footerPos > 0 && updatedText.at(footerPos - 1) != '\n') {
                updatedText.insert(footerPos, "\n");
                footerPos++;
            }
            // Insert the functions block before the footer
            updatedText.insert(footerPos, funcString + "\n\n");
        } else {
            // Append to the very end if no footer is found
            if (!updatedText.endsWith('\n')) {
                updatedText += "\n";
            }
            updatedText += "\n" + funcString + "\n";
        }
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

    // Helper lambda for precise double conversion
    auto toPreciseString =
        [](double val) { return QString::number(val, 'g', 10); };

    // Application
    out << "// Simulation executable\n";
    writeEntry("application", cfg.application);
    if (!cfg.solver.isEmpty()) {
        writeEntry("solver", cfg.solver);
    }
    out << "\n";

    // Time control
    out << "// Time control\n";
    writeEntry("startFrom", enumToString(cfg.startFrom, "startTime"));
    writeEntry("startTime", toPreciseString(cfg.startTime));
    writeEntry("stopAt", enumToString(cfg.stopAt, "endTime"));
    writeEntry("endTime", toPreciseString(cfg.endTime));
    writeEntry("deltaT", toPreciseString(cfg.deltaT));
    out << "\n";

    // Transient fields
    out << "// Transient configuration\n";
    writeEntry("adjustTimeStep", toFoamBool(cfg.adjustTimeStep));
    if (cfg.adjustTimeStep) {
        writeEntry("maxCo", toPreciseString(cfg.maxCo));
    }
    out << "\n";

    // Data writing
    out << "// Data writing\n";
    writeEntry("writeControl", enumToString(cfg.writeControl, "timeStep"));
    writeEntry("writeInterval", toPreciseString(cfg.writeInterval));
    writeEntry("purgeWrite", QString::number(cfg.purgeWrite));
    out << "\n";

    // Output format
    out << "// Output format\n";
    writeEntry("writeFormat", enumToString(cfg.writeFormat, "binary"));
    writeEntry("writePrecision", "6");
    writeEntry("writeCompression", toFoamBool(cfg.writeCompression));
    writeEntry("timeFormat", "general");
    writeEntry("timePrecision", "6");
    out << "\n";

    // Runtime configuration
    out << "// Runtime configuration\n";
    writeEntry("runTimeModifiable", toFoamBool(cfg.runTimeModifiable));

    // Function objects
    out << "\n\n" << functionObjects;

    // Closing separator
    out << "\n\n" << createFoamFooter();
    return dictStr;
}