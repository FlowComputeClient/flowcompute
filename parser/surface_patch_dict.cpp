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

