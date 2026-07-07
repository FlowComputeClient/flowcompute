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

#ifndef SURFACE_WIDGET_H
#define SURFACE_WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QFormLayout>

struct VisualizationSurface;

// --- The Base Class ---
class AbstractSurfaceWidget : public QWidget {
    Q_OBJECT

public:
    explicit AbstractSurfaceWidget(QWidget* parent = nullptr) : QWidget(parent) {}
    virtual ~AbstractSurfaceWidget() = default;

    virtual bool validateParameters() const = 0;
    virtual void saveToConfig(VisualizationSurface& config) const = 0;
};

// --- The Subclasses ---

class IsoSurfaceWidget : public AbstractSurfaceWidget {
    Q_OBJECT

public:
    explicit IsoSurfaceWidget(QWidget* parent = nullptr);
    bool validateParameters() const override;
    void saveToConfig(VisualizationSurface& config) const override;

private:
    QComboBox* m_fieldCombo;
    QDoubleSpinBox* m_thresholdSpinBox;
};

class CuttingPlaneWidget : public AbstractSurfaceWidget {
    Q_OBJECT

public:
    explicit CuttingPlaneWidget(QWidget* parent = nullptr);
    bool validateParameters() const override;
    void saveToConfig(VisualizationSurface& config) const override;

private:
    // E.g., Arrays for the 3D vectors
    QDoubleSpinBox* m_basePoint[3];
    QDoubleSpinBox* m_normal[3];
};

#endif // SURFACE_WIDGET_H
