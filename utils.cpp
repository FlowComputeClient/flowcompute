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

#include "./utils.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>

QString Utils::createFoamHeader(const QString& objectName,
                                const QString& foamPath) {
    QString headerStr;
    QTextStream out(&headerStr);

    // Default fallback values
    bool isESI = false;
    QString verText = "unknown";
    QRegularExpression re("openfoam-?v?(\\d+)",
                          QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(foamPath);

    if (match.hasMatch()) {
        QString digits = match.captured(1);
        int verNumber = digits.toInt();

        // ESI/Keysight releases use YYMM (e.g., 2312, 2412)
        if (verNumber > 100) {
            isESI = true;
            verText = "v" + digits;
        } else {
            // Foundation releases use major versions (e.g., 11, 12, 13)
            isESI = false;
            verText = digits;
        }
    } else {
        qWarning() <<
            "Warning: Could not parse OpenFOAM version from path:" << foamPath;
    }

    QString line3_right, line4_right;

    if (!isESI) {
        // Foundation (.org) format
        line3_right = "Website:  https://openfoam.org";
        line4_right = "Version:  " + verText;
    } else {
        // Keysight / ESI (.com) format
        line3_right = "Version:  " + verText;
        line4_right = "Website:  www.openfoam.com";
    }

    // Write the banner, padding the right side to exactly 47 characters
    out << "/*--------------------------------*- C++ -*-----------------------"
           "-----------*\\\n";
    out << "| =========                 "
           "|                                                 |\n";
    out << "| \\\\      /  F ield         | "
           "OpenFOAM: The Open Source CFD Toolbox           |\n";
    out << "|  \\\\    /   O peration     | "
        << line3_right.leftJustified(47, ' ') << " |\n";
    out << "|   \\\\  /    A nd           | "
        << line4_right.leftJustified(47, ' ') << " |\n";
    out << "|    \\\\/     M anipulation  "
           "|                                                 |\n";
    out << "\\*--------------------------------------------------------------"
           "-------------*/\n";

    // Write the required FoamFile dictionary
    out << "FoamFile\n";
    out << "{\n";
    out << "    version     2.0;\n";
    out << "    format      ascii;\n";
    out << "    class       dictionary;\n";
    out << "    object      " << objectName << ";\n";
    out << "}\n";
    out << "// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * "
           "* * * * * * //\n\n";

    return headerStr;
}

QString Utils::createFoamFooter() {
    return QString("// * * * * * * * * * * * * * * * * * * * * * * * * * * * "
                   "* * * * * * * * * * //\n");
}

QString Utils::createSurfacePatchDict(const QString& openFoamPath,
                                      const QString& fileName,
                                      double featureAngle) {
    // Set dictionary content
    QString dictContent = QString(R"(geometry
{
    "%1"
    {
        type triSurfaceMesh;
    }
}

surfaces
{
    "%1"
    {
        regions
        {
            ".*"
            {
                type            autoPatch;
                featureAngle    %2;
            }
        }
    }
}
)").arg(fileName, QString::number(featureAngle));

    // Combine everything
    return createFoamHeader("surfacePatchDict", openFoamPath) +
           dictContent + createFoamFooter();
}

QString Utils::createDecomposeParDict(const QString& openFoamPath,
                                      int numCores) {
    // Set dictionary content
    QString dictContent = QString(R"(
// Number of cores used for processing
numberOfSubdomains %1;

// Decomposition method
method scotch;
)").arg(QString::number(numCores));

    // Combine everything
    return createFoamHeader("decomposeParDict", openFoamPath) +
           dictContent + createFoamFooter();
}

Utils::ParseErrorAction Utils::showParsingErrorMessage(QString fileName,
                                                       QWidget* parent) {
    // Create dialog
    QMessageBox errorDialog(parent);
    errorDialog.setWindowTitle(
        QCoreApplication::translate("Utils", "Parse Error"));
    errorDialog.setText(
        QString(QCoreApplication::translate(
                "Utils", "<b>Failed to parse %1.</b>")).arg(fileName));
    errorDialog.setInformativeText(
        QCoreApplication::translate("Utils",
            "The file may contain syntax errors or unsupported keywords."));
    errorDialog.setIcon(QMessageBox::Warning);

    // Add the custom choices and assign them roles
    QPushButton *editBtn =
        errorDialog.addButton(QCoreApplication::translate("Utils", "Edit File"),
            QMessageBox::ActionRole);
    QPushButton *overwriteBtn =
        errorDialog.addButton(
            QCoreApplication::translate("Utils", "Overwrite File"),
                QMessageBox::DestructiveRole);
    errorDialog.addButton(QCoreApplication::translate("Utils", "Cancel"),
        QMessageBox::RejectRole);
    errorDialog.setDefaultButton(editBtn);

    // Execute the dialog modally
    errorDialog.exec();

    // Return value depending on button
    if (errorDialog.clickedButton() == editBtn) {
        return ParseErrorAction::EditFile;
    }
    if (errorDialog.clickedButton() == overwriteBtn) {
        return ParseErrorAction::Overwrite;
    }
    return ParseErrorAction::Cancel;
}
