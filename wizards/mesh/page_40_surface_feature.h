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

#ifndef WIZARDS_MESH_PAGE_40_SURFACE_FEATURE_H_
#define WIZARDS_MESH_PAGE_40_SURFACE_FEATURE_H_

#include <QWizardPage>

class MeshWizard;
class QTableWidget;

class SurfaceFeaturePage : public QWizardPage {
    Q_OBJECT

 public:
    explicit SurfaceFeaturePage(QWidget *parent);
    int nextId() const override;

 protected:
    void initializePage() override;
    bool validatePage() override;

 private:
    MeshWizard* meshWizard;
    QTableWidget* m_featureTable;
};

#endif  // WIZARDS_MESH_PAGE_40_SURFACE_FEATURE_H_
