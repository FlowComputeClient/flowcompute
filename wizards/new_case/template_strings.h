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

#ifndef WIZARDS_NEW_CASE_TEMPLATE_STRINGS_H_
#define WIZARDS_NEW_CASE_TEMPLATE_STRINGS_H_

#include <QString>

const QString algoTextSimple = R"FOAM(
SIMPLE
{
    nNonOrthogonalCorrectors     0;
    residualControl
    {
        p                     1e-4;
        U                     1e-4;
        "(e|h|T)"             1e-4;
        "(k|omega|epsilon)"   1e-4;
    }
}
)FOAM";

const QString algoTextPimple = R"FOAM(
PIMPLE
{
    momentumPredictor          yes;
    nOuterCorrectors             2;
    nCorrectors                  2;
    nNonOrthogonalCorrectors     1;
    residualControl
    {
        p                     1e-4;
        U                     1e-4;
        "(e|h|T)"             1e-4;
        "(k|omega|epsilon)"   1e-4;
    }
}
)FOAM";

const QString alphaTextCompressible = R"FOAM(
    rho
    {
        solver          diagonal;
    }

    "(e|h)"
    {
        solver          GAMG;
        smoother        DICGaussSeidel;
        tolerance       1e-6;
        relTol          0.1;
    }
)FOAM";

const QString alphaTextBoussinesq = R"(
    T
    {
        solver          smoothSolver;
        smoother        symGaussSeidel;
        tolerance       1e-5;
        relTol          0.1;
    }
)";

const QString alphaTextMultiphase = R"(
    alpha.water
    {
        solver          smoothSolver;
        smoother        symGaussSeidel;
        tolerance       1e-8;
        relTol          0;
    }
)";

const QString relaxTextSimple = R"FOAM(
// Under-relaxation factors used to improve stability
relaxationFactors
{
    fields
    {
        p                   0.3;
    }
    equations
    {
        U                   0.7;
        "(e|h|T)"           0.7;
        "(k|omega|epsilon)" 0.7;
    }
}
)FOAM";

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
        tolerance       1e-5;
        relTol          0.01;
        smoother        GaussSeidel;
    }
)";

const QString pTextCompressible = R"(
    p
    {
        solver          GAMG;
        tolerance       1e-5;
        relTol          0.05;
        smoother        GaussSeidel;
    }
)";

const QString pTextMultiphase = R"(
    p_rgh
    {
        solver          GAMG;
        tolerance       1e-7;
        relTol          0.01;
        smoother        GaussSeidel;
    }

    p_rghFinal
    {
        solver          PCG;
        preconditioner  DIC;
        tolerance       1e-8;
        relTol          0;
    }
)";

const QString schemeTextMultiphase =
    R"(div(phi,alpha)  Gauss vanLeer;
    div(phirb,alpha) Gauss interfaceCompression;)";

const QString schemeTextCompressible =
    R"(div(phi,K)       bounded Gauss upwind;)";

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

#endif  // WIZARDS_NEW_CASE_TEMPLATE_STRINGS_H_
