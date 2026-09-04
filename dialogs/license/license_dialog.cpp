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

#include "dialogs/license/license_dialog.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QHBoxLayout>
#include <QSplitter>
#include <QString>
#include <QTextBrowser>
#include <QTextStream>
#include <QtGlobal>
#include <QVBoxLayout>

#include <libssh/libssh.h>
#include <zlib.h>

// Display a dialog that shows the different licenses
LicenseDialog::LicenseDialog(QWidget *parent): QDialog(parent) {
    // Set the dialog's appearance
    setWindowTitle(tr("Third-Party Licenses"));
    resize(750, 450);

    // Create the main layout and splitter
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // Define the licenses
    m_licenses = {
        {
            .name = "Qt",
            .version = qVersion(),
            .url = "https://www.qt.io",
            .copyright = "Copyright (c) The Qt Company Ltd.",
            .licenseType = "LGPL v3",
            .resourcePath = ":/licenses/qt.txt"
        },
        {
            .name = "Asio",
            .version = "1.36.0",
            .url = "https://think-async.com/Asio",
            .copyright = "Copyright (c) 2003-2026 Christopher M. Kohlhoff",
            .licenseType = "Boost Software License 1.0",
            .resourcePath = ":/licenses/asio.txt"
        },
        {
            .name = "libssh",
            .version = QString::fromUtf8(ssh_version(0)),
            .url = "https://www.libssh.org",
            .copyright = "Copyright (c) 2003-2018 Aris Adamantiadis and others",
            .licenseType = "LGPL v2.1",
            .resourcePath = ":/licenses/libssh.txt"
        },
        {
            .name = "Tree-sitter",
            .version = "0.20.x",
            .url = "https://tree-sitter.github.io",
            .copyright = "Copyright (c) 2018 Max Brunsfeld",
            .licenseType = "The MIT License (MIT)",
            .resourcePath = ":/licenses/tree_sitter.txt"
        },
        {
            .name = "zlib",
            .version = QString::fromUtf8(ZLIB_VERSION),
            .url = "https://zlib.net",
            .copyright = "Copyright (c) 1995-2026 Jean-loup Gailly "
                      "and Mark Adler",
            .licenseType = "zlib License",
            .resourcePath = ":/licenses/zlib.txt"
        },
    };

    // Create list widget
    m_libList = new QListWidget(splitter);
    splitter->addWidget(m_libList);
    for (const auto &lic : m_licenses) {
        m_libList->addItem(lic.name);
    }

    // Create text browser
    m_licenseBrowser = new QTextBrowser(splitter);
    splitter->addWidget(m_licenseBrowser);

    // Configure the splitter
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setCollapsible(0, false);
    mainLayout->addWidget(splitter);

    // Respond to library selection
    connect(m_libList, &QListWidget::currentItemChanged,
            this, &LicenseDialog::onLicenseSelected);
    m_libList->setCurrentRow(0);

    // Add OK button
    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

QString LicenseDialog::readResourceText(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return tr("License text could not be loaded.");
    }
    QTextStream in(&file);
    return in.readAll();
}

void LicenseDialog::onLicenseSelected(QListWidgetItem *current) {
    if (!current) return;

    int index = m_libList->row(current);
    if (index < 0 || index >= static_cast<int>(m_licenses.size()))
        return;

    // Convert selection to LicenseInfo
    const LicenseInfo &lic = m_licenses[index];
    QString rawText = readResourceText(lic.resourcePath);

    // Build the formatted HTML
    QString html;
    html += "<h2>" + lic.name + " " + lic.version + "</h2>";
    html += "<p><b>Project:</b> "
            "<a href=\"" + lic.url + "\">" + lic.url + "</a><br/>";
    html += "<b>Copyright:</b> " + lic.copyright + "<br/>";
    html += "<b>License Type:</b> " + lic.licenseType + "</p>";
    html += "<hr/>";

    // Format license text
    html += "<pre style=\"font-family: monospace; white-space: pre-wrap;\">"
            + rawText.toHtmlEscaped() + "</pre>";

    // Display formatted text
    m_licenseBrowser->setHtml(html);
}
