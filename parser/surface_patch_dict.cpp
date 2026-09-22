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

#include "surface_patch_dict.h"

#include "common.h"

// Create a new surfacePatchDict file
QString CaseIO::createSurfacePatchDict(const QString& openFoamPath,
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

