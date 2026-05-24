#ifndef TEMPLATE_STRINGS_H
#define TEMPLATE_STRINGS_H

#include <QString>

const QString algoTextPimple = R"(
PIMPLE
{
    momentumPredictor        yes;
    nOuterCorrectors         2;
    nCorrectors              2;
)";

const QString algoTextSimple = R"(
SIMPLE
{)";

const QString alphaTextMultiphase = R"(
    alpha.water
    {
        solver          smoothSolver;
        smoother        symGaussSeidel;
        tolerance       1e-08;
        relTol          0;
    }
)";

const QString cleanCommandGeneral = R"(
# Clean standard case files and mesh
cleanCase

# Restore initial conditions
if [ -d 0.orig ]; then
    cp -r 0.orig 0
fi

# Clean up snappyHexMesh specific files
rm -rf constant/extendedFeatureEdgeMesh 2>/dev/null\n
rm -f constant/triSurface/*.eMesh 2>/dev/null
)";

const QString fieldsTextMultiphase = R"(
# Initialize phase fields
runApplication setFields
)";

const QString meshTextGeometry = R"(runApplication surfaceFeatureExtract
runApplication snappyHexMesh -overwrite
runApplication checkMesh)";

const QString pTextIncompressible = R"(
    p
    {
        solver          GAMG;
        tolerance       1e-7;
        relTol          0.01;
        smoother        GaussSeidel;
    }

    pFinal
    {
        $p;
        relTol          0;
    }
)";

const QString pTextCompressible = R"(
    p
    {
        solver          GAMG;
        tolerance       1e-8;
        relTol          0.05;
        smoother        DICGaussSeidel;
    }

    pFinal
    {
        $p;
        relTol          0;
    }
)";

const QString pTextMultiphase = R"(
    p_rgh
    {
        solver          GAMG;
        tolerance       1e-7;
        relTol          0.01;
        smoother        DICGaussSeidel;
    }

    p_rghFinal
    {
        solver          PCG;
        preconditioner  DIC;
        tolerance       1e-8;
        relTol          0;
    }
)";

const QString schemeTextMultiphase = R"(
    div(phi,alpha)  Gauss vanLeer;
    div(phirb,alpha) Gauss interfaceCompression;
)";

const QString turbDictLES = R"(
LES
{
    LESModel      WALE;
    turbulence    on;
    printCoeffs   on;
    delta         cubeRootVol;
    WALECoeffs
    {
        Ck        0.094;
        Ce        1.048;
    }
}
)";

const QString turbDictRAS = R"(
RAS
{
    RASModel    kOmegaSST;
    turbulence  on;
    printCoeffs on;
}
)";

#endif // TEMPLATE_STRINGS_H
