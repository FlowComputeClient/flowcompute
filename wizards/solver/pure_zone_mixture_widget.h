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

#ifndef WIZARDS_SOLVER_PURE_ZONE_MIXTURE_WIDGET_H_
#define WIZARDS_SOLVER_PURE_ZONE_MIXTURE_WIDGET_H_

#include "wizards/solver/mixture_widget.h"

class QListWidget;
class QStackedWidget;

class PureZoneMixtureWidget: public MixtureWidget {
    Q_OBJECT

 public:
    PureZoneMixtureWidget(const QString& thermo, const QString& transport,
                         const QString& eos, QWidget* parent = nullptr);

    // Access data
    CaseIO::MixtureVariant getData() const;

 private:
    QString m_thermo, m_transport, m_eos;
    QListWidget* m_zoneList;
    QStackedWidget* m_stackedWidget;

 private slots:
    void addZone();
    void removeZone();
};

#endif // WIZARDS_SOLVER_PURE_ZONE_MIXTURE_WIDGET_H_