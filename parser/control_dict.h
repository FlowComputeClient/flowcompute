#ifndef PARSER_CONTROL_DICT_H_
#define PARSER_CONTROL_DICT_H_

#include <memory>

#include "open_foam_dictionary.h"
#include "common.h"

namespace CaseIO {

struct ControlConfig {
    QString solverCategory;
    QString solver;

    // Run control
    StartSolverType startFrom = StartSolverType::startTime;
    double startTime = 0.0;
    EndSolverType stopAt = EndSolverType::endTime;
    double endTime = 0.5;
    double deltaT = 1.0;

    // Time step adjustment
    bool adjustTimeStep = false;
    double maxCo = 1.0;

    // Data writing
    bool writeCompression = false;
    bool runTimeModifiable = true;
    WriteFormatType writeFormat = WriteFormatType::binary;
    WriteControlType writeControl = WriteControlType::timeStep;
    double writeInterval = 20.0;
    int purgeWrite = 0;
};

// Parse controlDict
ControlConfig parseControlDict(std::shared_ptr<OpenFoamDictionary> dict);

// Update existing controlDict
QString updateControlDict(std::shared_ptr<OpenFoamDictionary> dict,
                          ControlConfig& cfg, QString funcString);

// Create new controlDict file
QString createControlDict(const ControlConfig& cfg, const QString& openFoamPath,
                          const QString& functionObjects);
};

#endif  // PARSER_CONTROL_DICT_H_
