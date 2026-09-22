#include "wizards/solver/page_30_thermo.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QLineEdit>
#include <QPushButton>

#include "wizards/solver/wizard_solver.h"
#include "wizards/solver/mixture_dialog.h"

ThermoPage::ThermoPage(QWidget *parent): QWizardPage(parent) {
    // Set title and layout
    setTitle(tr("Thermal Properties"));
    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(25);
    layout->setColumnMinimumWidth(1, 200);
    setLayout(layout);

    // Create widgets for each field
    int row = 0;
    setupField(layout, row++, tr("Base Model (type):"), m_typeCombo,
        {"heRhoThermo", "hePsiThermo", "heSolidThermo", "rhoThermo",
        "psiThermo", "Custom..."});

    setupField(layout, row++, tr("Mixture Composition (mixture):"),
        m_mixtureCombo, {"pureMixture", "reactingMixture",
        "multiComponentMixture", "homogeneousMixture", "inhomogeneousMixture",
        "egrMixture", "pureZoneMixture", "Custom..."});

    setupField(layout, row++, tr("Transport Properties (transport):"),
        m_transportCombo, {"const", "sutherland", "polynomial", "logPolynomial",
        "Custom..."});

    setupField(layout, row++, tr("Thermodynamic Properties (thermo):"),
        m_thermoCombo, {"hConst", "eConst", "janaf", "hPolynomial",
        "Custom..."});

    setupField(layout, row++, tr("State equation (equationOfState):"),
        m_eosCombo, { "perfectGas", "incompressiblePerfectGas", "rhoConst",
        "Boussinesq", "PengRobinsonGas", "adiabaticPerfectGas", "linear",
        "rPolynomial", "icoPolynomial", "perfectFluid", "Custom..." });

    setupField(layout, row++, tr("Species Formulation (specie):"),
        m_specieCombo, { "specie", "Custom..." });

    setupField(layout, row++, tr("Energy variable (energy):"),
        m_energyCombo, { "sensibleEnthalpy", "sensibleInternalEnergy",
        "absoluteEnthalpy", "absoluteInternalEnergy", "Custom..." });

    // Create button - occupy entire row
    m_mixtureButton = new QPushButton(tr("Define Mixture..."), this);
    m_mixtureButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(m_mixtureButton, row++, 0, 1, 2);
    connect(m_mixtureButton, &QPushButton::clicked, this, [this]() {
        MixtureDialog dialog(m_mixtureCombo->currentText(),
            m_thermoCombo->currentText(), m_transportCombo->currentText(),
            m_eosCombo->currentText(), this);
        if (dialog.exec() == QDialog::Accepted) {
            m_mixtureData = dialog.getData();
        }
    });

    // Inert Specie Container
    m_inertSpecieContainer = new QWidget(this);
    QHBoxLayout* inertLayout = new QHBoxLayout(m_inertSpecieContainer);
    inertLayout->setContentsMargins(0, 0, 0, 0);
    inertLayout->addWidget(new QLabel(tr("Inert Specie (inertSpecie):")));
    m_inertSpecieEdit = new QLineEdit(this);
    inertLayout->addWidget(m_inertSpecieEdit);
    layout->addWidget(m_inertSpecieContainer, row++, 0, 1, 2);

    // Chemistry Reader Container
    m_chemistryContainer = new QWidget(this);
    QHBoxLayout* chemLayout = new QHBoxLayout(m_chemistryContainer);
    chemLayout->setContentsMargins(0, 0, 0, 0);
    chemLayout->addWidget(new QLabel(tr("Chemistry Reader:")));
    m_chemistryReaderCombo = new QComboBox(this);
    m_chemistryReaderCombo->addItems({"foamChemistryReader", "chemkinReader"});
    chemLayout->addWidget(m_chemistryReaderCombo);
    layout->addWidget(m_chemistryContainer, row++, 0, 1, 2);

    // Chemistry Files Container
    m_chemistryFilesContainer = new QWidget(this);
    QGridLayout* filesLayout = new QGridLayout(m_chemistryFilesContainer);
    filesLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* chemFileLabel = new QLabel(tr("foamChemistryFile:"));
    m_chemFileEdit = new QLineEdit(this);
    filesLayout->addWidget(chemFileLabel, 0, 0);
    filesLayout->addWidget(m_chemFileEdit, 0, 1);

    QLabel* thermoFileLabel = new QLabel(tr("foamChemistryThermoFile:"));
    m_thermoFileEdit = new QLineEdit(this);
    filesLayout->addWidget(thermoFileLabel, 1, 0);
    filesLayout->addWidget(m_thermoFileEdit, 1, 1);

    layout->addWidget(m_chemistryFilesContainer, row++, 0, 1, 2);

    // Connect signals for additional fields
    connect(m_mixtureCombo, &QComboBox::currentTextChanged,
            this, &ThermoPage::updateExtraFieldsVisibility);

    connect(m_chemistryReaderCombo, &QComboBox::currentTextChanged,
            this, &ThermoPage::updateExtraFieldsVisibility);

    // Set the initial state
    updateExtraFieldsVisibility();
}

void ThermoPage::setupField(QGridLayout* layout, int row,
    const QString& labelText, QComboBox*& combo, const QStringList& options) {
    // Create label
    QLabel* label = new QLabel(labelText, this);
    layout->addWidget(label, row, 0);

    // Create combo box
    combo = new QComboBox(this);
    combo->addItems(options);
    layout->addWidget(combo, row, 1);
    connect(combo, QOverload<int>::of(&QComboBox::activated),
            this, [this, combo](int index) {
        // Check if the user selected the last item ("Custom...")
        if (index == combo->count() - 1) {

            bool ok;
            QString text = QInputDialog::getText(this,
                tr("Custom Entry"), tr("Enter custom model name:"),
                QLineEdit::Normal, "", &ok);

            if (ok && !text.trimmed().isEmpty()) {
                QString customText = text.trimmed();

                // Check if entry is already present
                int existingIndex = combo->findText(customText);
                if (existingIndex != -1) {
                    combo->setCurrentIndex(existingIndex);
                } else {
                    // Insert the new entry
                    combo->insertItem(0, customText);
                    combo->setCurrentIndex(0);
                }
            } else {
                combo->setCurrentIndex(0);
            }
        }
    });
}

void ThermoPage::updateExtraFieldsVisibility() {
    QString currentMixture = m_mixtureCombo->currentText();
    QString currentReader = m_chemistryReaderCombo->currentText();

    // inertSpecie logic
    bool needsInert = (currentMixture != "pureMixture" &&
                       currentMixture != "pureZoneMixture" &&
                       currentMixture != "Custom...");
    m_inertSpecieContainer->setVisible(needsInert);

    // chemistryReader logic
    bool needsChemistry = (currentMixture == "reactingMixture");
    m_chemistryContainer->setVisible(needsChemistry);

    // chemistry files logic (cascade dependency)
    bool needsFiles =
        (needsChemistry && currentReader == "foamChemistryReader");
    m_chemistryFilesContainer->setVisible(needsFiles);
}

bool ThermoPage::validatePage() {
    SolverWizard* wizardPtr = qobject_cast<SolverWizard*>(this->wizard());
    if (!wizardPtr)
        return false;

    // Check mixture data
    if (std::holds_alternative<std::monostate>(m_mixtureData)) {
        QMessageBox::warning(this, tr("Incomplete Configuration"),
            tr("Please click 'Define Mixture...' to configure the mixture "
                                "properties before proceeding."));
        return false;
    }

    // Access configuration object
    CaseIO::ThermoConfig& cfg = wizardPtr->getThermoConfig();
    cfg.thermoType.type = m_typeCombo->currentText();
    cfg.thermoType.mixture = m_mixtureCombo->currentText();
    cfg.thermoType.transport = m_transportCombo->currentText();
    cfg.thermoType.thermo = m_thermoCombo->currentText();
    cfg.thermoType.equationOfState = m_eosCombo->currentText();
    cfg.thermoType.specie = m_specieCombo->currentText();
    cfg.thermoType.energy = m_energyCombo->currentText();
    cfg.mixtureConfig = m_mixtureData;

    // Set other fields
    if (m_inertSpecieContainer->isVisible()) {
        QString inert = m_inertSpecieEdit->text().trimmed();
        if (inert.isEmpty()) {
            QMessageBox::warning(this, tr("Missing Data"),
                tr("Inert Specie cannot be empty."));
            return false;
        }
        cfg.inertSpecie = inert;
    } else {
        cfg.inertSpecie.clear();
    }

    if (m_chemistryContainer->isVisible()) {
        cfg.chemistryReader = m_chemistryReaderCombo->currentText();
    } else {
        cfg.chemistryReader.clear();
    }

    if (m_chemistryFilesContainer->isVisible()) {
        QString chemFile = m_chemFileEdit->text().trimmed();
        if (chemFile.isEmpty() &&
            cfg.chemistryReader == "foamChemistryReader") {
            QMessageBox::warning(this, tr("Missing Data"),
                tr("Please specify the chemistry file."));
            return false;
        }
        cfg.foamChemistryFile = chemFile;
        cfg.foamChemistryThermoFile = m_thermoFileEdit->text().trimmed();
    } else {
        cfg.foamChemistryFile.clear();
        cfg.foamChemistryThermoFile.clear();
    }

    return true;
}