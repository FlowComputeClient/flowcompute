#include <QMetaEnum>
#include <QRegularExpression>

#include "field.h"

// Parse the block mesh file into a BlockMeshConfig structure
void CaseIO::parseFieldFile(std::shared_ptr<OpenFoamDictionary> dict,
                            CaseIO::FieldData& cfg) {
    if (!dict) { return; }

    cfg.dimension = dict->getString("dimensions");
    cfg.internalField = dict->getString("internalField");

    // Parse Field Class
    QString classStr = dict->getString("FoamFile/class");
    if (!classStr.isEmpty()) {
        bool ok = false;
        int enumValue =
            QMetaEnum::fromType<FlowCompute::FieldClass>().keyToValue(
                classStr.toUtf8().constData(), &ok);
        if (ok) {
            cfg.fieldClass = static_cast<FlowCompute::FieldClass>(enumValue);
        }
    }

    // Parse Boundary Conditions
    QStringList patchNames = dict->getDictKeys("boundaryField");
    for (const QString& patchName : std::as_const(patchNames)) {
        CaseIO::BoundaryCondition bc;
        QString patchPath = "boundaryField/" + patchName;

        // Boundary condition type
        bc.type = dict->getString(patchPath + "/type");

        // Extract all other parameters for this specific boundary condition
        QStringList paramKeys = dict->getDictKeys(patchPath);
        for (const QString& paramKey : std::as_const(paramKeys)) {
            if (paramKey == "type") { continue; }

            // Treat all parameter values as strings for the UI backend.
            QString paramValue = dict->getString(patchPath + "/" + paramKey);

            // Only insert if it actually resolved a value
            if (!paramValue.isEmpty()) {
                bc.parameters[paramKey] = paramValue;
            }
        }

        // Append the fully constructed boundary condition to the field data
        cfg.bcs.push_back({patchName, bc});
    }
}

QString CaseIO::createFieldFile(const QString& fieldName,
                                  const CaseIO::FieldData& data,
                                  const QString& openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Field class
    QString fieldClassStr = enumToString(data.fieldClass, "volScalarField");

    // Write the standard header and correctly replace the class type
    QString header = createFoamHeader(fieldName, openFoamPath);
    header.replace("dictionary", fieldClassStr);
    out << header;

    // Standard lambda with indentation support
    auto writeEntry = [&out](const QString& keyword, const QString& value,
                             int indentLevel = 0, bool addEmptyLine = false) {
        QString indent(indentLevel * 4, ' ');
        out << indent << keyword.leftJustified(20, ' ') << value << ";\n";
        if (addEmptyLine) out << "\n";
    };

    // 1. Dimensions and internal field
    writeEntry("dimensions", data.dimension, 0, true);
    writeEntry("internalField", data.internalField, 0, true);

    // 2. Boundary field sub-dictionary
    out << "boundaryField\n{\n";

    for (const auto& [patchName, bc] : data.bcs) {
        out << "    " << patchName << "\n    {\n";

        // Write the boundary condition type
        writeEntry("type", bc.type, 2);

        // Iterate through and write any additional parameters
        for (const auto& [paramName, paramValue] : bc.parameters) {
            writeEntry(paramName, paramValue, 2);
        }

        out << "    }\n";
    }

    out << "}\n\n";

    // Write closing separator
    out << "// ************************************************************************* //\n";

    return dictStr;
}