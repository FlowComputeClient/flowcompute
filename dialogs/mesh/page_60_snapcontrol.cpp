#include "page_60_snapcontrol.h"

#include "wizard_mesh.h"

// Introduction page asks for the case name and platform
SnapControlPage::SnapControlPage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Snap Control Configuration"));

    // Create a vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);

    // Set group box for global constraints
    QGroupBox* snapBox = new QGroupBox(tr("Snapping Iterations"), this);
    layout->addWidget(snapBox);
    QFormLayout* snapLayout = new QFormLayout(snapBox);

    // Number of smoothing iterations
    smoothingBox = new QSpinBox(snapBox);
    smoothingBox->setRange(0, 10);
    snapLayout->addRow(tr("Surface smoothing iterations prior to snapping:"), smoothingBox);

    // Max number of snapping iterations
    maxSnappingBox = new QSpinBox(snapBox);
    maxSnappingBox->setRange(10, 300);
    snapLayout->addRow(tr("Maximum mesh displacement iterations:"), maxSnappingBox);

    // Number of relaxation iterations after snapping
    relaxationBox = new QSpinBox(snapBox);
    relaxationBox->setRange(3, 20);
    snapLayout->addRow(tr("Maximum number of relaxation iterations:"), relaxationBox);

    // Create spin box for snapping tolerance
    toleranceBox = new QDoubleSpinBox(this);
    toleranceBox->setRange(0.1, 10.0);
    toleranceBox->setSingleStep(0.1);
    toleranceBox->setDecimals(1);
    snapLayout->addRow(tr("Snapping tolerance (relative to local cell size):"), toleranceBox);

    // Set group box for edge resolution
    QGroupBox* edgeBox = new QGroupBox(tr("Feature Edge Resolution"), this);
    layout->addWidget(edgeBox);
    QFormLayout* edgeLayout = new QFormLayout(edgeBox);

    // Whether the mesher should read edges from eMesh file
    explicitSnapBox = new QCheckBox(tr("Snap to explicit feature edges (.eMesh files)"), edgeBox);
    edgeLayout->addRow(explicitSnapBox);

    // Whether the mesher should detect edges using normal vectors
    implicitSnapBox = new QCheckBox(tr("Implicitly detect edges using surface curvature"), edgeBox);
    edgeLayout->addRow(implicitSnapBox);

    // Number of iterations for edge snapping
    snapIterationBox = new QSpinBox(edgeBox);
    snapIterationBox->setRange(3, 20);
    edgeLayout->addRow(tr("Number of iterations for edge snapping:"), snapIterationBox);
}

void SnapControlPage::initializePage() {

    // Access the mesh wizard
    meshWizard = qobject_cast<MeshWizard*>(wizard());
    if (!meshWizard) {
        qWarning() << "Failed to cast to MeshWizard!";
        return;
    }
    m_cfg = &(meshWizard->getSnapControlConfig());

    // Update widgets
    smoothingBox->setValue(m_cfg->nSmoothPatch);
    maxSnappingBox->setValue(m_cfg->nSolveIter);
    relaxationBox->setValue(m_cfg->nRelaxIter);
    toleranceBox->setValue(m_cfg->tolerance);
    explicitSnapBox->setChecked(m_cfg->explicitFeatureSnap);
    implicitSnapBox->setChecked(m_cfg->implicitFeatureSnap);
    snapIterationBox->setValue(m_cfg->nFeatureSnapIter);
}

bool SnapControlPage::validatePage() {

    // Update configuration object with widget values
    m_cfg->nSmoothPatch = smoothingBox->value();
    m_cfg->nSolveIter = maxSnappingBox->value();
    m_cfg->nRelaxIter = relaxationBox->value();
    m_cfg->tolerance = toleranceBox->value();
    m_cfg->explicitFeatureSnap = explicitSnapBox->isChecked();
    m_cfg->implicitFeatureSnap = implicitSnapBox->isChecked();
    m_cfg->nFeatureSnapIter = snapIterationBox->value();

    return true;
}

int SnapControlPage::nextId() const {
    if (meshWizard->m_runLayers) {
        return Page_LayerControl;
    }
    return Page_Execution;
}