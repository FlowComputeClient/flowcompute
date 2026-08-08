#ifndef PARSER_SURFACE_FEATURE_H_
#define PARSER_SURFACE_FEATURE_H_

#include <memory>

#include "open_foam_dictionary.h"
#include "common.h"

namespace CaseIO {

// Parse transport properties file
void parseTransportProperties(std::shared_ptr<OpenFoamDictionary> dict,
                              PhysicsConfig& cfg);


// Create transport properties file
QString createTransportProperties(const PhysicsConfig& cfg,
                                  const QString& openFoamPath);
};

#endif  // PARSER_SURFACE_FEATURE_H_
