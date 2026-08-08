#ifndef PARSER_TURBULENCE_PROPERTIES_H_
#define PARSER_TURBULENCE_PROPERTIES_H_

#include <memory>

#include "open_foam_dictionary.h"
#include "common.h"

namespace CaseIO {

// Parse turbulence properties file
PhysicsConfig parseTurbulenceProperties(
    std::shared_ptr<OpenFoamDictionary> dict);

// Create new turbulence properties file
QString createTurbulenceProperties(
    const PhysicsConfig& config, const QString& openFoamPath);
};

#endif  // PARSER_TURBULENCE_PROPERTIES_H_
