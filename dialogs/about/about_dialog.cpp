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

#include "dialogs/about/about_dialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>

AboutDialog::AboutDialog(QWidget *parent): QDialog(parent) {
    // Set the dialog's appearance
    setWindowTitle(tr("About FlowCompute"));
    resize(550, 220);

    // Create main layout
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(20);

    // Display icon
    auto* iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/images/flowcompute.png").pixmap(64, 64));
    iconLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    // Right layout
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(15);

    // Title (Dynamically pulls from QCoreApplication)
    QLabel* titleLabel = new QLabel(QString("<b>FlowCompute %1</b>").
            arg(QCoreApplication::applicationVersion()), this);
    rightLayout->addWidget(titleLabel);
    rightLayout->addSpacing(10);

    // Build (Dynamically generated at compile time)
    QLabel* buildLabel = new QLabel(QString("Built on %1, based on Qt %2").
            arg(__DATE__, qVersion()), this);
    rightLayout->addWidget(buildLabel);

    // Copyright
    QLabel* copyrightLabel =
        new QLabel("Copyright ©2026 FlowCompute LLC", this);
    rightLayout->addWidget(copyrightLabel);

    // Acknowledgements (Fixed HTML tags)
    QLabel* ackLabel = new QLabel(
        "<b>Acknowledgements:</b>"
        "<ul>"
        "<li>OpenFOAM® is a trademark of OpenCFD Ltd, producer and distributor "
        "of <a href=\"https://www.openfoam.com\">OpenFOAM software</a>.</li>"
        "<li>The graphical user interface and application framework are built "
        "with <a href=\"https://www.qt.io/\">Qt</a>.</li>"
        "<li>Dictionary parsing is powered by the "
        "<a href=\"https://tree-sitter.github.io/tree-sitter/\">TreeSitter</a> "
        "library.</li>"
        "<li>Asynchronous network operations are handled by the "
        "<a href=\"https://think-async.com/Asio/\">Asio</a> C++ library.</li>"
        "<li>Remote server connectivity is provided by "
        "<a href=\"https://www.libssh.org/\">libssh</a>.</li>"
        "<li>High-performance 3D graphics are rendered via the "
        "<a href=\"https://www.vulkan.org/\">Vulkan</a> API.</li>"
        "</ul>", this);

    // Enable link clicking and prevent excessive dialog width
    ackLabel->setOpenExternalLinks(true);
    ackLabel->setWordWrap(true);

    rightLayout->addWidget(ackLabel);
    rightLayout->addStretch();

    // Add OK button
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok,
                                           Qt::Horizontal, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    rightLayout->addWidget(buttonBox);

    // Assemble the dialog
    mainLayout->addWidget(iconLabel);
    mainLayout->addLayout(rightLayout);
}
