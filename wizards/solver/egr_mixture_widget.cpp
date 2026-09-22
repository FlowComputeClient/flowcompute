#include "wizards/solver/egr_mixture_widget.h"

#include <QTabWidget>
#include <QVBoxLayout>

#include "wizards/solver/pure_mixture_widget.h"

EgrMixtureWidget::EgrMixtureWidget(const QString& thermo,
    const QString& transport, const QString& eos, QWidget* parent)
    : MixtureWidget(parent) {

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QTabWidget* tabWidget = new QTabWidget(this);

    // Instantiate the four child widgets using the passed-in context strings
    m_fuelWidget = new PureMixtureWidget(thermo, transport, eos, this);
    m_oxidantWidget = new PureMixtureWidget(thermo, transport, eos, this);
    m_productWidget = new PureMixtureWidget(thermo, transport, eos, this);
    m_egrWidget = new PureMixtureWidget(thermo, transport, eos, this);

    // Add them to the tab widget
    tabWidget->addTab(m_fuelWidget, tr("Fuel"));
    tabWidget->addTab(m_oxidantWidget, tr("Oxidant"));
    tabWidget->addTab(m_productWidget, tr("Products"));
    tabWidget->addTab(m_egrWidget, tr("EGR"));

    mainLayout->addWidget(tabWidget);
}

CaseIO::MixtureVariant EgrMixtureWidget::getData() const {
    CaseIO::EgrMixtureConfig config;

    // Retrieve the MixtureVariant from the child widgets
    CaseIO::MixtureVariant fuelVariant = m_fuelWidget->getData();
    CaseIO::MixtureVariant oxidantVariant = m_oxidantWidget->getData();
    CaseIO::MixtureVariant productsVariant = m_productWidget->getData();
    CaseIO::MixtureVariant egrVariant = m_egrWidget->getData();

    // Safely extract the PureMixtureConfig from the returned variants
    if (std::holds_alternative<CaseIO::PureMixtureConfig>(fuelVariant)) {
        config.fuel = std::get<CaseIO::PureMixtureConfig>(fuelVariant);
    }

    if (std::holds_alternative<CaseIO::PureMixtureConfig>(oxidantVariant)) {
        config.oxidant = std::get<CaseIO::PureMixtureConfig>(oxidantVariant);
    }

    if (std::holds_alternative<CaseIO::PureMixtureConfig>(productsVariant)) {
        config.products = std::get<CaseIO::PureMixtureConfig>(productsVariant);
    }

    if (std::holds_alternative<CaseIO::PureMixtureConfig>(egrVariant)) {
        config.egr = std::get<CaseIO::PureMixtureConfig>(egrVariant);
    }

    return config;
}
