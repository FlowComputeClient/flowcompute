#include "model_left_pane.h"

#include "../../../geometry/graphic_data.h"

ModelLeftPane::ModelLeftPane(QWidget* parent): QWidget(parent) {

    // Vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    setLayout(layout);

    // Set stylesheet
    setStyleSheet(R"(
        QPushButton    { color: black; }
        QLabel         { color: black; }
        QCheckBox      { color: black; }
        QDoubleSpinBox { color: black; }
        QTableWidget   { color: black; }
        QHeaderView    { color: black; }
    )");

    // Set dimensions
    int buttonWidth = 130;
    int spinBoxWidth = 80;

    // Label
    layout->addSpacing(5);
    QLabel* paneTitle = new QLabel(tr("<b>Surface Utilities</b>"));
    layout->addWidget(paneTitle, 0, Qt::AlignHCenter);
    layout->addSpacing(5);

    // Button to check model
    m_checkButton = new QPushButton(tr("Check Surface"));
    m_checkButton->setFixedWidth(buttonWidth);
    layout->addWidget(m_checkButton, 0, Qt::AlignHCenter);
    connect(m_checkButton, &QPushButton::clicked,
            this, &ModelLeftPane::onCheckButtonClicked);

    // Horizontal line
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // Label that displays surface size
    m_boundsLabel = new QLabel();
    layout->addWidget(m_boundsLabel, 0, Qt::AlignHCenter);

    // Create horizontal layout
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->setSpacing(5);

    // Label for scale factor
    QLabel* scaleLabel = new QLabel(tr("Scale Factor:"));
    hLayout->addWidget(scaleLabel);

    // Spin box for scale factor
    m_scaleSpin = new QDoubleSpinBox();
    m_scaleSpin->setValue(1.0);
    m_scaleSpin->setSingleStep(0.1);
    m_scaleSpin->setDecimals(5);
    m_scaleSpin->setRange(0.00001, 100000.0);
    m_scaleSpin->setFixedWidth(spinBoxWidth);
    hLayout->addWidget(m_scaleSpin);
    layout->addLayout(hLayout);

    // Button to launch surfaceTransformPoints
    m_scaleButton = new QPushButton(tr("Scale Surface"));
    m_scaleButton->setFixedWidth(buttonWidth);
    layout->addWidget(m_scaleButton, 0, Qt::AlignHCenter);
    connect(m_scaleButton, &QPushButton::clicked,
            this, &ModelLeftPane::onScaleButtonClicked);

    // Horizontal line
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // Create horizontal layout
    hLayout = new QHBoxLayout();
    hLayout->setSpacing(5);

    // Label for feature angle
    QLabel* angleLabel = new QLabel(tr("Feature Angle:"));
    hLayout->addWidget(angleLabel);

    // Spin box for feature angle
    m_angleSpin = new QDoubleSpinBox();
    m_angleSpin->setValue(45);
    m_angleSpin->setSingleStep(5.0);
    m_angleSpin->setRange(0.0, 180.0);
    m_angleSpin->setFixedWidth(spinBoxWidth);
    hLayout->addWidget(m_angleSpin);
    layout->addLayout(hLayout);

    // Button to launch surfaceAutoPatch
    m_patchButton = new QPushButton(tr("Generate Patches"));
    m_patchButton->setFixedWidth(buttonWidth);
    layout->addWidget(m_patchButton, 0, Qt::AlignHCenter);
    connect(m_patchButton, &QPushButton::clicked,
            this, &ModelLeftPane::onPatchButtonClicked);

    // Horizontal line
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // Set table title
    QLabel* tableTitle = new QLabel(tr("<b>Surface Patches</b>"));
    layout->addWidget(tableTitle, 0, Qt::AlignHCenter);

    // Patch table
    m_patchTable = new QTableWidget(this);
    m_patchTable->setColumnCount(2);
    m_patchTable->horizontalHeader()->setVisible(false);
    m_patchTable->setColumnWidth(0, 30);
    m_patchTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_patchTable->verticalHeader()->setVisible(false);
    m_patchTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    layout->addWidget(m_patchTable, 0, Qt::AlignHCenter);

    // Emit signal when a patch name changes
    connect(m_patchTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (item->column() == 1) {
            emit dirtyStateChanged(true);
        }
    });

    layout->addStretch();
}

void ModelLeftPane::setBounds(std::array<float, 3> bounds) {

    m_bounds = bounds;
    QString boundsStr = QString("<b>( %1, %2, %3 )</b>")
        .arg(m_bounds[0], 0, 'g', 5)
        .arg(m_bounds[1], 0, 'g', 5)
        .arg(m_bounds[2], 0, 'g', 5);
    m_boundsLabel->setText(boundsStr);
}

void ModelLeftPane::changeBounds(double scaleFactor) {
    m_bounds[0] *= scaleFactor;
    m_bounds[1] *= scaleFactor;
    m_bounds[2] *= scaleFactor;
    QString boundsStr = QString("<b>( %1, %2, %3 )</b>")
        .arg(m_bounds[0], 0, 'g', 5)
        .arg(m_bounds[1], 0, 'g', 5)
        .arg(m_bounds[2], 0, 'g', 5);
    m_boundsLabel->setText(boundsStr);
}

void ModelLeftPane::setPatchNames(const std::vector<std::string>& patchNames) {

    // Configure the table
    m_patchTable->setRowCount(0);
    m_patchTable->setRowCount(static_cast<int>(patchNames.size()));

    // Add rows to the table
    for(int i = 0; i < static_cast<int>(patchNames.size()); i++) {

        // Get the patch color
        QColor color(
            static_cast<int>(patchColors[i][0] * 255.0f),
            static_cast<int>(patchColors[i][1] * 255.0f),
            static_cast<int>(patchColors[i][2] * 255.0f),
            static_cast<int>(patchColors[i][3] * 255.0f));

        // Create item for patch color
        QTableWidgetItem *colorItem = new QTableWidgetItem();
        colorItem->setFlags(Qt::ItemIsEnabled);
        colorItem->setBackground(color);
        m_patchTable->setItem(i, 0, colorItem);

        // Create item for patch name
        QTableWidgetItem *nameItem = new QTableWidgetItem(QString::fromStdString(patchNames[i]));
        nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        m_patchTable->setItem(i, 1, nameItem);
    }

    // Set the table height
    int height = m_patchTable->frameWidth() * 2;
    for (int row = 0; row < m_patchTable->rowCount(); ++row) {
        height += m_patchTable->rowHeight(row);
    }
    m_patchTable->setFixedHeight(height);

    update();
}

std::vector<std::string> ModelLeftPane::getPatchNames() const {

    // Reserve memory
    std::vector<std::string> patchNames;
    int rowCount = m_patchTable->rowCount();
    patchNames.reserve(rowCount);

    // Access patch names in table
    for (int row = 0; row < rowCount; ++row) {
        QTableWidgetItem* item = m_patchTable->item(row, 1);
        if (item) {
            patchNames.push_back(item->text().toStdString());
        } else {
            patchNames.push_back("");
        }
    }
    return patchNames;
}

// Respond when the check button is pressed
void ModelLeftPane::onCheckButtonClicked() {
    emit surfaceCheckRequested();
}

// Respond when the check button is pressed
void ModelLeftPane::onScaleButtonClicked() {
    double scale = m_scaleSpin->value();
    emit surfaceScaleRequested(scale);
}

// Respond when the check button is pressed
void ModelLeftPane::onPatchButtonClicked() {
    double angle = m_angleSpin->value();
    emit surfacePatchRequested(angle);
}

