#include "utils.h"

QString Utils::createFoamHeader(const QString& objectName, const QString& foamPath) {
    QString headerStr;
    QTextStream out(&headerStr);

    // Default fallback values
    bool isESI = false;
    QString verText = "unknown";
    QRegularExpression re("openfoam-?v?(\\d+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(foamPath);

    if (match.hasMatch()) {
        QString digits = match.captured(1);
        int verNumber = digits.toInt();

        // ESI/Keysight releases use YYMM (e.g., 2312, 2412), so the number is always > 100
        if (verNumber > 100) {
            isESI = true;
            verText = "v" + digits;
        } else {
            // Foundation releases use sequential major versions (e.g., 11, 12, 13)
            isESI = false;
            verText = digits;
        }
    } else {
        qWarning() << "Warning: Could not parse OpenFOAM version from path:" << foamPath;
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
    out << "/*--------------------------------*- C++ -*----------------------------------*\\\n";
    out << "| =========                 |                                                 |\n";
    out << "| \\\\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox           |\n";
    out << "|  \\\\    /   O peration     | " << line3_right.leftJustified(47, ' ') << " |\n";
    out << "|   \\\\  /    A nd           | " << line4_right.leftJustified(47, ' ') << " |\n";
    out << "|    \\\\/     M anipulation  |                                                 |\n";
    out << "\\*---------------------------------------------------------------------------*/\n";

    // Write the required FoamFile dictionary
    out << "FoamFile\n";
    out << "{\n";
    out << "    version     2.0;\n";
    out << "    format      ascii;\n";
    out << "    class       dictionary;\n";
    out << "    object      " << objectName << ";\n";
    out << "}\n";
    out << "// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n\n";

    return headerStr;
}

QString Utils::createFoamFooter() {
    return QString("// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n");
}

QString Utils::createSurfacePatchDict(const QString& openFoamPath, const QString& fileName,
                                      double featureAngle) {

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

QString Utils::createDecomposeParDict(const QString& openFoamPath, int numCores) {

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

QString Utils::createFieldFile(const QString& openFoamPath, QString fieldName, FlowCompute::FieldData data) {
    QString fileContent = QString(R"(
dimensions      %1;

internalField   %2;

boundaryField
{
)").arg(data.dimension, data.internalField);

    for (const auto& [patchName, bc] : data.bcs) {
        fileContent += QString(R"(
    %1
    {
        type            %2;
)").arg(patchName, bc.type);
        for (const auto& [paramName, paramValue] : bc.parameters) {
            fileContent += QString(R"(
        %1              %2;
)").arg(paramName, paramValue);
        }
        fileContent += QString(R"(
    }
)");
    }
    fileContent += QString(R"(
})");

    QString header = createFoamHeader(fieldName, openFoamPath).replace("dictionary", data.fieldClass);

    return header + fileContent + createFoamFooter();
}

/*
bool Utils::showParsingErrorMessage(QString fileName) {

    QMessageBox errorDialog(this);
    errorDialog.setWindowTitle("Parse Error");
    errorDialog.setText(QString("<b>Failed to parse %1.</b>").arg(fileName));
    errorDialog.setInformativeText("The file may contain syntax errors or unsupported keywords.");
    errorDialog.setIcon(QMessageBox::Warning);

    // Add the custom choices and assign them roles
    QPushButton *editBtn = errorDialog.addButton("Edit File", QMessageBox::ActionRole);
    QPushButton *overwriteBtn = errorDialog.addButton("Overwrite with Defaults", QMessageBox::DestructiveRole);
    QPushButton *cancelBtn = errorDialog.addButton("Cancel", QMessageBox::RejectRole);
    errorDialog.setDefaultButton(editBtn);

    // Execute the dialog modally
    errorDialog.exec();

    // Determine which choice the user made
    if (errorDialog.clickedButton() == editBtn) {
        mainWin->createEditor(EditorType::TEXT, fileName.split('/').last(), m_caseName + "/system");
        reject();
        return false;
    } else if (errorDialog.clickedButton() == overwriteBtn) {
        return true;
    } else if (errorDialog.clickedButton() == cancelBtn) {
        return false;
    }
    return false;
}
*/