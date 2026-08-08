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

#ifndef WIZARDS_POST_PROCESSING_SURFACES_DIALOG_H_
#define WIZARDS_POST_PROCESSING_SURFACES_DIALOG_H_

#include <QDialog>

#include "parser/function_object.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QListWidget;
class QStackedWidget;

class SurfacesDialog : public QDialog {
    Q_OBJECT
 public:
    SurfacesDialog(const QStringList& fields,
        CaseIO::SurfacesConfig& surfacesConfig, QWidget* parent = nullptr);
    CaseIO::SurfacesConfig getFunctionObject() { return m_surfacesConfig; };

 private:
    QStringList m_fields;
    CaseIO::SurfacesConfig& m_surfacesConfig;
    QCheckBox *m_logCheck, *m_locationCheck;
    QComboBox *m_executeCombo, *m_writeCombo, *m_surfaceFormatCombo;
    QComboBox *m_interpolationCombo;
    QDoubleSpinBox *m_executeSpin, *m_writeSpin;
    QLineEdit *m_nameEdit;
    QListWidget *m_fieldListWidget, *m_surfaceListWidget;
    QStackedWidget *m_surfaceStack;

    const std::unordered_map<CaseIO::SurfaceDef::SurfaceType, QStringList>
        m_surfaceParameters = {
            {CaseIO::SurfaceDef::SurfaceType::patch,
                {"patches", "interpolate"}},
            {CaseIO::SurfaceDef::SurfaceType::cuttingPlane,
                {"planeType", "basePoint", "normalVector", "interpolate"}},
            {CaseIO::SurfaceDef::SurfaceType::isoSurface,
                {"isoField", "isoValue", "interpolate", "regularise"}},
            {CaseIO::SurfaceDef::SurfaceType::isoSurfaceCell,
                {"isoField", "isoValue", "interpolate", "regularise"}},
            {CaseIO::SurfaceDef::SurfaceType::meshedSurface,
                {"surface", "source", "interpolate"}},
            {CaseIO::SurfaceDef::SurfaceType::distanceSurface,
                {"distance", "signed", "surface", "interpolate"}}
        };

 private slots:
    void onOkClicked();
    void addSurface();
    void removeSurface();
};

#endif  // WIZARDS_POST_PROCESSING_SURFACES_DIALOG_H_
