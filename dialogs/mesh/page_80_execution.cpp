#include "page_80_execution.h"
#include "wizard_mesh.h"
#include <QVariant>

ExecutionPage::ExecutionPage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Mesh Execution Configuration"));

    // Use a standard vertical layout for stacking the group boxes
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // blockMesh group
    m_blockMeshGroup = new QGroupBox(tr("Base Mesh Generation"), this);
    QVBoxLayout* blockMeshLayout = new QVBoxLayout(m_blockMeshGroup);
    m_runBlockMeshBox = new QCheckBox(tr("Run blockMesh"), m_blockMeshGroup);
    m_runBlockMeshBox->setChecked(true);
    blockMeshLayout->addWidget(m_runBlockMeshBox);
    mainLayout->addWidget(m_blockMeshGroup);
    registerField("execRunBlockMesh", m_runBlockMeshBox);

    // surfaceFeatureExtract group
    m_extractGroup = new QGroupBox(tr("Feature Extraction"), this);
    QVBoxLayout* extractLayout = new QVBoxLayout(m_extractGroup);
    m_runFeatureExtractBox = new QCheckBox(tr("Run surfaceFeatureExtract"), m_extractGroup);
    m_runFeatureExtractBox->setChecked(true);
    extractLayout->addWidget(m_runFeatureExtractBox);
    mainLayout->addWidget(m_extractGroup);
    registerField("execRunFeatureExtract", m_runFeatureExtractBox);

    // snappyHexMesh group
    m_snappyGroup = new QGroupBox(tr("snappyHexMesh Generation"), this);
    QVBoxLayout* snappyLayout = new QVBoxLayout(m_snappyGroup);

    // Master snappy toggle
    m_runSnappyHexMeshBox = new QCheckBox(tr("Run snappyHexMesh"), m_snappyGroup);
    m_runSnappyHexMeshBox->setChecked(true);
    snappyLayout->addWidget(m_runSnappyHexMeshBox);
    registerField("execRunSnappyHexMesh", m_runSnappyHexMeshBox);

    // Sub-options container (indented slightly for visual hierarchy)
    m_snappyOptionsWidget = new QWidget(m_snappyGroup);
    QFormLayout* snappyOptionsLayout = new QFormLayout(m_snappyOptionsWidget);
    snappyOptionsLayout->setContentsMargins(20, 5, 0, 0);

    m_snappyHexMeshModeBox = new QComboBox(m_snappyOptionsWidget);
    m_snappyHexMeshModeBox->addItems({ tr("Generate Mesh"), tr("Dry Run"), tr("Check Geometry") });
    snappyOptionsLayout->addRow(tr("Execution mode:"), m_snappyHexMeshModeBox);
    registerField("execSnappyMode", m_snappyHexMeshModeBox);

    m_snappyHexMeshOverwriteBox = new QCheckBox(tr("Overwrite existing mesh"), m_snappyOptionsWidget);
    snappyOptionsLayout->addRow(m_snappyHexMeshOverwriteBox);
    registerField("execSnappyOverwrite", m_snappyHexMeshOverwriteBox);

    snappyLayout->addWidget(m_snappyOptionsWidget);
    mainLayout->addWidget(m_snappyGroup);

    // Connect snappy master toggle to its sub-options
    connect(m_runSnappyHexMeshBox, &QCheckBox::toggled,
            m_snappyOptionsWidget, &QWidget::setEnabled);

    // Connect mode dropdown to diagnostic logic
    connect(m_snappyHexMeshModeBox, &QComboBox::currentIndexChanged,
            this, &ExecutionPage::snappyHexMeshModeChanged);

    // Post-processing group
    m_postGroup = new QGroupBox(tr("Post-Generation"), this);
    QVBoxLayout* postLayout = new QVBoxLayout(m_postGroup);

    m_checkMeshBox = new QCheckBox(tr("Run checkMesh"), m_postGroup);
    m_checkMeshBox->setChecked(true);
    postLayout->addWidget(m_checkMeshBox);
    registerField("execRunCheckMesh", m_checkMeshBox);

    m_displayMeshBox = new QCheckBox(tr("Display generated mesh"), m_postGroup);
    m_displayMeshBox->setChecked(true);
    postLayout->addWidget(m_displayMeshBox);
    registerField("execDisplayMesh", m_displayMeshBox);

    mainLayout->addWidget(m_postGroup);
    mainLayout->addStretch(1);
}

void ExecutionPage::initializePage() {

    meshWizard = qobject_cast<MeshWizard*>(wizard());
    if (!meshWizard) {
        qWarning() << "Failed to cast to MeshWizard!";
        return;
    }

    bool willRunSnappy = meshWizard->m_runCastellated || meshWizard->m_runSnap || meshWizard->m_runLayers;

    // By hiding the entire group box, Qt's layout manager automatically
    // removes the spacing and collapses the UI seamlessly.
    m_blockMeshGroup->setVisible(meshWizard->m_runBlockMesh);
    m_extractGroup->setVisible(meshWizard->m_runExtract);
    m_snappyGroup->setVisible(willRunSnappy);
}

void ExecutionPage::snappyHexMeshModeChanged() {
    bool isGenerating = (m_snappyHexMeshModeBox->currentIndex() == 0);

    // Disable overwrite and post-processing if running a diagnostic mode
    m_snappyHexMeshOverwriteBox->setEnabled(isGenerating);
    m_checkMeshBox->setEnabled(isGenerating);
    m_displayMeshBox->setEnabled(isGenerating);

    if (!isGenerating) {
        m_snappyHexMeshOverwriteBox->setChecked(false);
        m_checkMeshBox->setChecked(false);
        m_displayMeshBox->setChecked(false);
    }
}