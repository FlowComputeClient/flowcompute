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

#ifndef EDITORS_TEXT_FILE_CHANGED_BANNER_H_
#define EDITORS_TEXT_FILE_CHANGED_BANNER_H_

#include <QFrame>

class QLabel;
class QPushButton;

class FileChangedBanner : public QFrame {
    Q_OBJECT

public:
    explicit FileChangedBanner(QWidget *parent = nullptr);

signals:
    void reloadClicked();
    void ignoreClicked();

private:
    QLabel *m_iconLabel;
    QLabel *m_messageLabel;
    QPushButton *m_reloadButton;
    QPushButton *m_ignoreButton;
};

#endif // EDITORS_TEXT_FILE_CHANGED_BANNER_H_
