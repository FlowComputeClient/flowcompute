#include "model_left_pane.h"

#include "../../../geometry/mesh_data.h"

ModelLeftPane::ModelLeftPane(QWidget* parent): QWidget(parent) {

    // Vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    setLayout(layout);

    // Set stylesheet
    setStyleSheet(R"(
        QPushButton { color: black; }
        QLabel      { color: black; }
        QCheckBox   { color: black; }
        QDoubleSpinBox { color: black; }
        QTableWidget { color: black; }
        QHeaderView { color: black; }
    )");

    // Label
    layout->addSpacing(5);
    QLabel* paneTitle = new QLabel(tr("<b>Surface Utilities</b>"));
    layout->addWidget(paneTitle, 0, Qt::AlignHCenter);
    layout->addSpacing(5);

    // Button to check model
    int buttonWidth = 130;
    m_checkButton = new QPushButton(tr("Run Check"));
    m_checkButton->setFixedWidth(buttonWidth);
    layout->addWidget(m_checkButton, 0, Qt::AlignHCenter);
    connect(m_checkButton, &QPushButton::clicked,
            this, &ModelLeftPane::onCheckButtonClicked);

    // Horizontal line
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // Create horizontal layout
    QHBoxLayout* hLayout = new QHBoxLayout();

    // Label for feature angle
    QLabel* angleLabel = new QLabel(tr("Feature Angle:"));
    hLayout->addWidget(angleLabel);

    // Spin box for feature angle
    m_angleSpin = new QDoubleSpinBox();
    m_angleSpin->setValue(45);
    m_angleSpin->setSingleStep(5.0);
    m_angleSpin->setRange(0.0, 180.0);
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
    m_patchTable->setColumnCount(3);
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

void ModelLeftPane::setPatchNames(const std::vector<std::string>& patchNames) {

    // Configure the table
    m_patchTable->clearContents();
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
void ModelLeftPane::onPatchButtonClicked() {
    double angle = m_angleSpin->value();
    emit surfacePatchRequested(angle);
}

