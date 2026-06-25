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