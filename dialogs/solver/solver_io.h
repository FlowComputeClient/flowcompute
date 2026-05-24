#ifndef SOLVER_IO_H
#define SOLVER_IO_H

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include "../../parser/open_foam_dictionary.h"
#include "solver_structs.h"

namespace SolverIO {

    // Header string
    QString createFoamHeader(const QString& objectName, const QString& openFoamPath);

    // Parse functions
    ControlConfig parseControlConfig(std::shared_ptr<OpenFoamDictionary> dict);
    PhysicsConfig parseTurbulenceProperties(std::shared_ptr<OpenFoamDictionary> dict);
    void parseTransportProperties(std::shared_ptr<OpenFoamDictionary> dict,
                                  PhysicsConfig& cfg);
}

#endif // SOLVER_IO_H
