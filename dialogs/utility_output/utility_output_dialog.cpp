#include "utility_output_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QStyle>
#include <QApplication>
#include <QFont>

UtilityOutputDialog::UtilityOutputDialog(const QString& title,
                                         const QString& description,
                                         const std::vector<QString>& summaryItems,
                                         const QString& rawLog,
                                         QWidget *parent)
    : QDialog(parent), m_rawLogTextEdit(nullptr), m_toggleButton(nullptr) {

    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    mainLayout->setSpacing(15);

    // Set appearance
    setWindowTitle(title);
    setStyleSheet(R"(
        QLabel      { color: black; font-size: 14px; }
        QPushButton { color: black; }
        QTextEdit   { color: black; }
    )");

    // Set description
    if (!description.isEmpty()) {
        mainLayout->addSpacing(10);
        QLabel* descLabel = new QLabel("<b>" + description + "</b>", this);
        descLabel->setWordWrap(true);
        QFont f = descLabel->font();
        f.setPointSize(f.pointSize() + 2);
        descLabel->setFont(f);
        mainLayout->addWidget(descLabel);
    }

    // Display Summary Items
    for (const auto& item : summaryItems) {
        QHBoxLayout* itemLayout = new QHBoxLayout();
        QLabel* textLabel = new QLabel(item, this);
        textLabel->setIndent(15);
        itemLayout->addWidget(textLabel);
        itemLayout->addStretch();
        mainLayout->addLayout(itemLayout);
    }

    mainLayout->addSpacing(15);

    // Log button
    m_toggleButton = new QPushButton(tr("Show Log"), this);
    m_toggleButton->setCheckable(true);
    connect(m_toggleButton, &QPushButton::toggled, this,
            &UtilityOutputDialog::toggleLogViewer);
    mainLayout->addWidget(m_toggleButton);

    // Show/hide log text
    m_rawLogTextEdit = new QTextEdit(this);
    m_rawLogTextEdit->setReadOnly(true);
    m_rawLogTextEdit->setPlainText(rawLog);
    m_rawLogTextEdit->setMinimumSize(400, 300);

    // Use a monospace font for terminal output
    QFont monoFont("Courier");
    monoFont.setStyleHint(QFont::Monospace);
    m_rawLogTextEdit->setFont(monoFont);
    m_rawLogTextEdit->setVisible(false);
    mainLayout->addWidget(m_rawLogTextEdit);

    // Close Button
    QPushButton* closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton, Qt::AlignHCenter);
    mainLayout->addLayout(buttonLayout);
}

void UtilityOutputDialog::toggleLogViewer(bool checked) {
    m_rawLogTextEdit->setVisible(checked);

    if (checked) {
        m_toggleButton->setText(tr("Hide Log"));
    } else {
        m_toggleButton->setText(tr("Show Log"));
    }

    // Resize the dialog
    this->adjustSize();
}