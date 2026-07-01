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

#ifndef SURFACE_LEFT_PANE_H_
#define SURFACE_LEFT_PANE_H_

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

class SurfaceLeftPane : public QWidget {
    Q_OBJECT

public:
    explicit SurfaceLeftPane(QWidget* parent = nullptr);

    // Get and set patch names
    std::vector<std::string> getPatchNames() const;
    void setPatchNames(const std::vector<std::string>& patchNames);
    void setBounds(std::array<float, 3> bounds);
    void changeBounds(double scaleFactor);

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void surfaceCheckRequested();
    void surfaceScaleRequested(double scaleFactor);
    void surfacePatchRequested(double featureAngle);
    void dirtyStateChanged(bool isDirty);

private:
    bool m_isBinary;
    std::array<float, 3> m_bounds;
    QDoubleSpinBox *m_angleSpin, *m_scaleSpin;
    QLabel* m_boundsLabel;
    QPushButton *m_checkButton, *m_patchButton, *m_scaleButton;
    QTableWidget *m_patchTable;

private slots:
    void onCheckButtonClicked();
    void onScaleButtonClicked();
    void onPatchButtonClicked();
};

#endif // SURFACE_LEFT_PANE_H_
