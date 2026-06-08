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

    struct BoundaryFileParts {
        QByteArray header;
        QByteArray payload;
        QByteArray footer;
    };

    // Parse functions
    ControlConfig parseControlConfig(std::shared_ptr<OpenFoamDictionary> dict);
    PhysicsConfig parseTurbulenceProperties(std::shared_ptr<OpenFoamDictionary> dict);
    void parseTransportProperties(std::shared_ptr<OpenFoamDictionary> dict,
                                  PhysicsConfig& cfg);
    std::vector<FlowCompute::MeshPatch> parseBoundaryPatches(const QByteArray& fileData);
    QString updateBoundaryFile(std::shared_ptr<OpenFoamDictionary> dict,
                               const std::vector<FlowCompute::MeshPatch>& filtered);
    BoundaryFileParts splitBoundaryFile(const QByteArray& rawData);
}

#endif // SOLVER_IO_H
