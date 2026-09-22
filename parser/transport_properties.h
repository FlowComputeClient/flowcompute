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

#ifndef PARSER_TRANSPORT_PROPERTIES_H_
#define PARSER_TRANSPORT_PROPERTIES_H_

#include <memory>

#include "open_foam_dictionary.h"
#include "common.h"

namespace CaseIO {

// Parse transport properties file
void parseTransportProperties(std::shared_ptr<OpenFoamDictionary> dict,
                              TransportConfig& cfg);

// Create transport properties file
QString createTransportProperties(const TransportConfig& cfg,
                    const QString& openFoamPath, bool isOpenCFD);
};

#endif  // PARSER_TRANSPORT_PROPERTIES_H_
