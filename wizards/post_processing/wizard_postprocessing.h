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

#ifndef WIZARDS_POST_PROCESSING_WIZARD_POSTPROCESSING_H_
#define WIZARDS_POST_PROCESSING_WIZARD_POSTPROCESSING_H_

#include <QWizard>

#include "systems/system_manager.h"

enum {
    Page_Tasks = 0,
    Page_Time_Region
};

class PostprocessingWizard : public QWizard {
    Q_OBJECT

 public:
    PostprocessingWizard(const QString& caseName, const QStringList& patches,
        const QStringList& fields, const SystemManager& systemMgr,
        QWidget *parent);

 protected:
    void accept() override;

 private:
    const QString& m_caseName;
    const SystemManager& m_systemMgr;
};

#endif  // WIZARDS_POST_PROCESSING_WIZARD_POSTPROCESSING_H_
