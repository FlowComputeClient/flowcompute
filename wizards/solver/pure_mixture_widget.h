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

#ifndef WIZARDS_SOLVER_PURE_MIXTURE_WIDGET_H_
#define WIZARDS_SOLVER_PURE_MIXTURE_WIDGET_H_

#include "wizards/solver/mixture_widget.h"

class PureMixtureWidget: public MixtureWidget {
    Q_OBJECT

 public:
    PureMixtureWidget(const QString& thermo, const QString& transport,
        const QString& eos, QWidget* parent = nullptr);

    // Access data
    CaseIO::MixtureVariant getData() const;
};

#endif // WIZARDS_SOLVER_PURE_MIXTURE_WIDGET_H_

