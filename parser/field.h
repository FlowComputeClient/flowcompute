#ifndef PARSER_FIELD_H_
#define PARSER_FIELD_H_

#include <QObject>
#include <QString>

#include "open_foam_dictionary.h"
#include "common.h"
#include "core_types.h"

namespace CaseIO {

// Store data from a field file
struct FieldData {
    QString dimension = "[0 0 0 0 0 0 0]";
    FlowCompute::FieldClass fieldClass =
        FlowCompute::FieldClass::volScalarField;
    QString internalField = "uniform 0";
    std::vector<std::pair<QString, BoundaryCondition>> bcs;
};

// Parse a field file
void parseFieldFile(std::shared_ptr<OpenFoamDictionary> dict,
                    CaseIO::FieldData& fieldData);

// Create a new field file
QString createFieldFile(const QString& fieldName,
                        const CaseIO::FieldData& data,
                        const QString& openFoamPath);
};

#endif  // PARSER_FIELD_H_
