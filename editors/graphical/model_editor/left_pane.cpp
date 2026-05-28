#include "left_pane.h"

LeftPane::LeftPane(QWidget* parent): QWidget(parent) {

    // Vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    setLayout(layout);

    // Set stylesheet
    setStyleSheet(R"(
        QPushButton { color: black; }
        QLabel      { color: black; }
        QCheckBox   { color: black; }
        QDoubleSpinBox   { color: black; }
    )");

    // Button to check model
    m_checkButton = new QPushButton(tr("Run Check"));
    layout->addWidget(m_checkButton);
    connect(m_checkButton, &QPushButton::clicked,
            this, &LeftPane::onCheckButtonClicked);

    // Horizontal line
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // Label for feature angle
    QLabel* angleLabel = new QLabel(tr("Feature Angle"));
    layout->addWidget(angleLabel);

    // Spin box for feature angle
    m_angleSpin = new QDoubleSpinBox();
    m_angleSpin->setValue(45);
    m_angleSpin->setSingleStep(5.0);
    m_angleSpin->setRange(0.0, 180.0);
    layout->addWidget(m_angleSpin);

    // Check box to overwrite
    m_overwriteCheck = new QCheckBox(tr("Overwrite File"));
    m_overwriteCheck->setChecked(true);
    layout->addWidget(m_overwriteCheck);

    // Button to launch surfaceAutoPatch
    m_patchButton = new QPushButton(tr("Generate Patches"));
    layout->addWidget(m_patchButton);
    connect(m_patchButton, &QPushButton::clicked,
            this, &LeftPane::onPatchButtonClicked);

    // Horizontal line
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    layout->addStretch();
}

// Respond when the check button is pressed
void LeftPane::onCheckButtonClicked() {
    emit surfaceCheckRequested();
}

// Respond when the check button is pressed
void LeftPane::onPatchButtonClicked() {
    double angle = m_angleSpin->value();
    emit autoPatchRequested(angle);
}

