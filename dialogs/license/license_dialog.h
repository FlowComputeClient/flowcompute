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

#ifndef DIALOGS_LICENSE_LICENSE_DIALOG_H_
#define DIALOGS_LICENSE_LICENSE_DIALOG_H_

#include <QDialog>
#include <QListWidget>
#include <vector>

struct LicenseInfo {
    QString name;
    QString version;
    QString url;
    QString copyright;
    QString licenseType;
    QString resourcePath;
};

class QListWidget;
class QString;
class QTextBrowser;

class LicenseDialog : public QDialog {
    Q_OBJECT

 public:
    explicit LicenseDialog(QWidget *parent = nullptr);

 private:
    void setupUi();
    void loadLicenses();
    QString readResourceText(const QString &path);

    QListWidget *m_libList;
    QTextBrowser *m_licenseBrowser;
    std::vector<LicenseInfo> m_licenses;

 private slots:
    void onLicenseSelected(QListWidgetItem *current);
};

#endif  // DIALOGS_LICENSE_LICENSE_DIALOG_H_