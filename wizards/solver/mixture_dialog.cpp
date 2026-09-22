#include "wizards/solver/mixture_dialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "wizards/solver/pure_mixture_widget.h"
#include "wizards/solver/multi_component_widget.h"
#include "wizards/solver/reacting_mixture_widget.h"
#include "wizards/solver/homogeneous_mixture_widget.h"
#include "wizards/solver/inhomogeneous_mixture_widget.h"
#include "wizards/solver/pure_zone_mixture_widget.h"
#include "wizards/solver/egr_mixture_widget.h"
#include "wizards/solver/custom_mixture_widget.h"

MixtureDialog::MixtureDialog(const QString& mixture, const QString& thermo,
    const QString& transport, const QString& eos, QWidget* parent):
        QDialog(parent) {
    // Set title and style
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // Create widget for mixture type
    if (mixture == "pureMixture") {
        setWindowTitle(tr("Pure Mixture Definition"));
        m_mixtureWidget = new PureMixtureWidget(thermo, transport, eos, this);
    } else if (mixture == "multiComponentMixture") {
        setWindowTitle(tr("Multi-Component Mixture Definition"));
        m_mixtureWidget =
            new MultiComponentWidget(thermo, transport, eos, this);
    } else if (mixture == "reactingMixture") {
        setWindowTitle(tr("Reacting Mixture Definition"));
        m_mixtureWidget = new ReactingMixtureWidget(this);
    } else if (mixture == "homogeneousMixture") {
        setWindowTitle(tr("Homogeneous Mixture Definition"));
        m_mixtureWidget =
            new HomogeneousMixtureWidget(thermo, transport, eos, this);
    } else if (mixture == "inhomogeneousMixture") {
        setWindowTitle(tr("Inhomogeneous Mixture Definition"));
        m_mixtureWidget =
            new InhomogeneousMixtureWidget(thermo, transport, eos, this);
    } else if (mixture == "egrMixture") {
        setWindowTitle(
            tr("Exhaust Gas Recirculation (EGR) Mixture Definition"));
        m_mixtureWidget =
            new EgrMixtureWidget(thermo, transport, eos, this);
    } else if (mixture == "pureZoneMixture") {
        setWindowTitle(tr("Pure Zone Mixture Definition"));
        m_mixtureWidget =
            new PureZoneMixtureWidget(thermo, transport, eos, this);
    } else {
        // Fallback for custom text entry
        setWindowTitle(tr("Custom Mixture Definition"));
        m_mixtureWidget = new CustomMixtureWidget(this);
    }
    mainLayout->addWidget(m_mixtureWidget);

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
