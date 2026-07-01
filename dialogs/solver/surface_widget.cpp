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

#include "surface_widget.h"

#include <QMessageBox>

IsoSurfaceWidget::IsoSurfaceWidget(QWidget* parent) : AbstractSurfaceWidget(parent) {
    QFormLayout* layout = new QFormLayout(this);

    m_fieldCombo = new QComboBox(this);
    // Populate fieldCombo...
    layout->addRow(tr("Iso Field:"), m_fieldCombo);

    m_thresholdSpinBox = new QDoubleSpinBox(this);
    layout->addRow(tr("Threshold Value:"), m_thresholdSpinBox);
}

bool IsoSurfaceWidget::validateParameters() const {
    if (m_fieldCombo->currentText().isEmpty()) {
        QMessageBox::warning(nullptr, tr("Validation Error"), tr("Please select a field for the iso-surface."));
        return false;
    }
    return true;
}

void IsoSurfaceWidget::saveToConfig(VisualizationSurface& config) const {
    // Save logic here...
}

// --- CuttingPlaneWidget Implementation ---

CuttingPlaneWidget::CuttingPlaneWidget(QWidget* parent) : AbstractSurfaceWidget(parent) {
    // Layout and widget instantiation...
}

bool CuttingPlaneWidget::validateParameters() const {
    // Validation logic...
    return true;
}

void CuttingPlaneWidget::saveToConfig(VisualizationSurface& config) const {
    // Save logic...
}