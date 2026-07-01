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

#include "page_10_control.h"

#include "wizard_solver.h"
#include "page_60_simple.h"
#include "page_70_pimple.h"
#include "page_80_piso.h"

// Introduction page asks for the case name and platform
AlgorithmPage::AlgorithmPage(QWidget *parent): QWizardPage(parent) {

    // Set title
    setTitle(tr("Global Algorithm Configuration (fvSolution)"));

    // Create a grid layout with two columns
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);

    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    layout->addWidget(mainSplitter);

    // Left side: fields
    QWidget* listContainer = new QWidget(mainSplitter);
    QVBoxLayout* listLayout = new QVBoxLayout(listContainer);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(4);

    // Create label for list widget
    QLabel* listLabel = new QLabel(tr("Fields:"), listContainer);
    QFont labelFont = listLabel->font();
    labelFont.setBold(true);
    listLabel->setFont(labelFont);
    listLayout->addWidget(listLabel);

    // Create list widget
    m_fieldListWidget = new QListWidget(listContainer);
    m_fieldListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listLayout->addWidget(m_fieldListWidget);

    // Right side
    m_solverStack = new QStackedWidget(mainSplitter);

    // Create the primary configuration page
    QWidget* configPage = new QWidget();
    QFormLayout* solverLayout = new QFormLayout(configPage);
    solverLayout->setContentsMargins(10, 20, 10, 0);
    solverLayout->setSpacing(20);

    // Set solver
    m_solverCombo = new QComboBox(configPage);
    QMetaEnum solverEnum = QMetaEnum::fromType<FlowCompute::LinearSolver>();
    for (int i = 0; i < solverEnum.keyCount(); ++i) {
        m_solverCombo->addItem(solverEnum.key(i), solverEnum.value(i));
    }
    solverLayout->addRow(tr("Solver: "), m_solverCombo);
    connect(m_solverCombo, &QComboBox::currentIndexChanged,
            this, &AlgorithmPage::solverChanged);

    // Preconditioner/smoother
    m_preconditionerSmootherLabel = new QLabel(tr("Preconditioner: "), configPage);
    m_preconditionerSmootherCombo = new QComboBox(configPage);
    solverLayout->addRow(m_preconditionerSmootherLabel, m_preconditionerSmootherCombo);
    connect(m_preconditionerSmootherCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        QListWidgetItem* currentItem = m_fieldListWidget->currentItem();
        if (index < 0 || !currentItem || !m_cfg) return;
        auto precond = static_cast<FlowCompute::Preconditioner>(
            m_preconditionerSmootherCombo->itemData(index).toInt());
        m_cfg->fieldMathConfigs[currentItem->text()].preconditioner = precond;
    });

    // Absolute tolerance
    m_absTolEdit = new QLineEdit(configPage);
    solverLayout->addRow(tr("Absolute tolerance: "), m_absTolEdit);
    connect(m_absTolEdit, &QLineEdit::textEdited, this, [this](const QString& text) {
        QListWidgetItem* currentItem = m_fieldListWidget->currentItem();
        if (!currentItem || !m_cfg) return;
        bool ok;
        double val = text.toDouble(&ok);
        if (ok) m_cfg->fieldMathConfigs[currentItem->text()].absTolerance = val;
    });

    // Relative tolerance
    m_relTolEdit = new QLineEdit(configPage);
    solverLayout->addRow(tr("Relative tolerance: "), m_relTolEdit);
    connect(m_relTolEdit, &QLineEdit::textEdited, this, [this](const QString& text) {
        QListWidgetItem* currentItem = m_fieldListWidget->currentItem();
        if (!currentItem || !m_cfg) return;
        bool ok;
        double val = text.toDouble(&ok);
        if (ok) m_cfg->fieldMathConfigs[currentItem->text()].relTolerance = val;
    });

    // Relaxation factor
    m_relaxationSpin = new QDoubleSpinBox(configPage);
    m_relaxationSpin->setRange(0.0, 1.0);
    m_relaxationSpin->setSingleStep(0.1);
    solverLayout->addRow(tr("Relaxation factor: "), m_relaxationSpin);
    connect(m_relaxationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        QListWidgetItem* currentItem = m_fieldListWidget->currentItem();
        if (!currentItem || !m_cfg) return;
        m_cfg->fieldMathConfigs[currentItem->text()].relaxationFactor = value;
    });

    // Relaxation Type
    m_fieldsRadio = new QRadioButton(tr("Field"), configPage);
    m_equationsRadio = new QRadioButton(tr("Equation"), configPage);
    m_equationsRadio->setChecked(true);

    // Radio button group
    m_relaxGroup = new QButtonGroup(this);
    m_relaxGroup->addButton(m_fieldsRadio, 0);
    m_relaxGroup->addButton(m_equationsRadio, 1);
    connect(m_relaxGroup, &QButtonGroup::idToggled, this, [this](int id, bool checked) {
        QListWidgetItem* currentItem = m_fieldListWidget->currentItem();
        if (!checked || !currentItem || !m_cfg) return;
        m_cfg->fieldMathConfigs[currentItem->text()].isFieldsRelaxation = (id == 0);
    });

    // Radio button layout
    QHBoxLayout* radioLayout = new QHBoxLayout();
    radioLayout->setContentsMargins(0, 0, 0, 0);
    radioLayout->addWidget(m_fieldsRadio);
    radioLayout->addWidget(m_equationsRadio);
    radioLayout->addStretch();
    solverLayout->addRow(tr("Relaxation type: "), radioLayout);

    // Final iteration override
    QGroupBox* finalGroup = new QGroupBox(tr("Final Iteration Configuration"), configPage);
    QFormLayout* finalLayout = new QFormLayout(finalGroup);

    // Check for final iteration override
    m_finalIterationCheck = new QCheckBox(finalGroup);
    finalLayout->addRow(m_finalIterationCheck);
    connect(m_finalIterationCheck, &QCheckBox::toggled, this, [this](bool checked) {
        QListWidgetItem* currentItem = m_fieldListWidget->currentItem();
        if (!currentItem || !m_cfg) return;
        m_cfg->fieldMathConfigs[currentItem->text()].hasFinalOverride = checked;
        m_finalAbsTolEdit->setEnabled(checked);
        m_finalRelTolEdit->setEnabled(checked);
    });

    // Final absolute tolerance
    m_finalAbsTolEdit = new QLineEdit(finalGroup);
    finalLayout->addRow(tr("Final absolute tolerance: "), m_finalAbsTolEdit);
    connect(m_finalAbsTolEdit, &QLineEdit::textEdited, this, [this](const QString& text) {
        QListWidgetItem* currentItem = m_fieldListWidget->currentItem();
        if (!currentItem || !m_cfg) return;
        bool ok;
        double val = text.toDouble(&ok);
        if (ok) m_cfg->fieldMathConfigs[currentItem->text()].finalAbsTolerance = val;
    });

    // Final relative tolerance
    m_finalRelTolEdit = new QLineEdit(finalGroup);
    finalLayout->addRow(tr("Final relative tolerance: "), m_finalRelTolEdit);
    connect(m_finalRelTolEdit, &QLineEdit::textEdited, this, [this](const QString& text) {
        QListWidgetItem* currentItem = m_fieldListWidget->currentItem();
        if (!currentItem || !m_cfg) return;
        bool ok;
        double val = text.toDouble(&ok);
        if (ok) m_cfg->fieldMathConfigs[currentItem->text()].finalRelTolerance = val;
    });

    solverLayout->addRow(finalGroup);

    // Add the completed form to the stacked widget
    m_solverStack->addWidget(configPage);

    // Sizing hints (1:4 ratio to keep the right-hand panel spacious)
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 3);

    // Event-handling
    connect(m_fieldListWidget, &QListWidget::itemSelectionChanged,
            this, &AlgorithmPage::fieldSelectionChanged);
}

void AlgorithmPage::initializePage() {
    m_solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    if (!m_solverWizard) return;
    m_cfg = &(m_solverWizard->getMathConfig());

    // Get fields
    QStringList fields = m_solverWizard->getConfiguredFields();

    // Configure field list widget
    m_fieldListWidget->blockSignals(true);
    m_fieldListWidget->clear();
    m_fieldListWidget->addItem("< default >");
    m_fieldListWidget->addItems(fields);
    m_fieldListWidget->blockSignals(false);

    // Add default field to state
    if (!m_cfg->fieldMathConfigs.contains("< default >")) {
        m_cfg->fieldMathConfigs.insert("< default >", FieldMathConfig());
    }

    // Lambda to populate residual controls
    auto populateResiduals = [&fields](auto& config) {
        if (fields.contains("p")) {
            config.resControls.push_back({false, "p", "1e-5"});
        }
        if (fields.contains("U")) {
            config.resControls.push_back({false, "U", "1e-5"});
        }
        for (const auto& field : std::as_const(fields)) {
            if (field != "p" && field != "U") {
                config.resControls.push_back({false, field, "1e-5"});
            }
        }
    };

    // Update layout according to solver algorithm
    m_algorithm = m_solverWizard->getSolverAlgorithm();

    if (m_algorithm == FlowCompute::Algorithm::SIMPLE) {
        if (!std::holds_alternative<SimpleConfig>(m_cfg->algorithmConfig)) {
            m_cfg->algorithmConfig = SimpleConfig();
        }
        SimpleConfig& cfg = std::get<SimpleConfig>(m_cfg->algorithmConfig);
        if (cfg.resControls.empty()) {
            populateResiduals(cfg);
        }
    }
    else if (m_algorithm == FlowCompute::Algorithm::PIMPLE) {
        if (!std::holds_alternative<PimpleConfig>(m_cfg->algorithmConfig)) {
            m_cfg->algorithmConfig = PimpleConfig();
        }
        PimpleConfig& cfg = std::get<PimpleConfig>(m_cfg->algorithmConfig);
        if (cfg.resControls.empty()) {
            populateResiduals(cfg);
        }
    }
    else if (m_algorithm == FlowCompute::Algorithm::PISO) {
        if (!std::holds_alternative<PisoConfig>(m_cfg->algorithmConfig)) {
            m_cfg->algorithmConfig = PisoConfig();
        }
    }

    // Start event processing
    if (m_fieldListWidget->count() > 0) {
        m_fieldListWidget->setCurrentRow(0);
    }
}

// Set next page of the wizard
int AlgorithmPage::nextId() const {
    if (m_algorithm == FlowCompute::Algorithm::SIMPLE) {
        return Page_Simple;
    } else if (m_algorithm == FlowCompute::Algorithm::PIMPLE) {
        return Page_Pimple;
    } else if (m_algorithm == FlowCompute::Algorithm::PISO) {
        return Page_Piso;
    }
    return Page_Simple;
}

// Respond when a field is selected
void AlgorithmPage::fieldSelectionChanged() {

    QList<QListWidgetItem*> items = m_fieldListWidget->selectedItems();
    if (items.isEmpty()) { return; }

    // Use the first selected item to populate the UI baseline
    QString displayField = items.first()->text();

    // Ensure the field exists in the configuration
    if (!m_cfg->fieldMathConfigs.contains(displayField)) {
        m_cfg->fieldMathConfigs.insert(displayField, FieldMathConfig());
    }

    // Retrieve a copy of the config strictly for reading/populating the UI
    const FieldMathConfig& displayConfig = m_cfg->fieldMathConfigs[displayField];

    // Block signals
    m_absTolEdit->blockSignals(true);
    m_relTolEdit->blockSignals(true);
    m_relaxationSpin->blockSignals(true);
    m_fieldsRadio->blockSignals(true);
    m_equationsRadio->blockSignals(true);
    m_finalIterationCheck->blockSignals(true);
    m_finalAbsTolEdit->blockSignals(true);
    m_finalRelTolEdit->blockSignals(true);
    m_solverCombo->blockSignals(true);

    // Update tolerances
    m_absTolEdit->setText(QString::number(displayConfig.absTolerance));
    m_relTolEdit->setText(QString::number(displayConfig.relTolerance));

    // Update relaxation parameters
    m_relaxationSpin->setValue(displayConfig.relaxationFactor);
    if (displayConfig.isFieldsRelaxation) {
        m_fieldsRadio->setChecked(true);
    } else {
        m_equationsRadio->setChecked(true);
    }

    // Final iterations settings
    if (displayField == "< default >") {
        // Disable and explain why
        m_finalIterationCheck->setEnabled(false);
        m_finalIterationCheck->setChecked(false);
        m_finalIterationCheck->setText(tr("Enable Final Iteration (Per-field only)"));
        m_finalAbsTolEdit->setEnabled(false);
        m_finalRelTolEdit->setEnabled(false);
        m_finalAbsTolEdit->clear();
        m_finalRelTolEdit->clear();
    } else {
        // Enable and dynamically set the field name
        m_finalIterationCheck->setEnabled(true);
        m_finalIterationCheck->setText(tr("Enable %1Final").arg(displayField));

        if (displayConfig.hasFinalOverride) {
            m_finalIterationCheck->setChecked(true);
            m_finalAbsTolEdit->setEnabled(true);
            m_finalRelTolEdit->setEnabled(true);
            m_finalAbsTolEdit->setText(QString::number(displayConfig.finalAbsTolerance));
            m_finalRelTolEdit->setText(QString::number(displayConfig.finalRelTolerance));
        } else {
            m_finalIterationCheck->setChecked(false);
            m_finalAbsTolEdit->setEnabled(false);
            m_finalRelTolEdit->setEnabled(false);
            m_finalAbsTolEdit->clear();
            m_finalRelTolEdit->clear();
        }
    }

    // Unblock signals
    m_absTolEdit->blockSignals(false);
    m_relTolEdit->blockSignals(false);
    m_relaxationSpin->blockSignals(false);
    m_fieldsRadio->blockSignals(false);
    m_equationsRadio->blockSignals(false);
    m_finalIterationCheck->blockSignals(false);
    m_finalAbsTolEdit->blockSignals(false);
    m_finalRelTolEdit->blockSignals(false);
    m_solverCombo->blockSignals(false);

    // Update the combo box to match the underlying data
    int solverInt = static_cast<int>(displayConfig.solver);
    int comboIndex = m_solverCombo->findData(solverInt);
    if (comboIndex >= 0) {
        m_solverCombo->setCurrentIndex(comboIndex);
        solverChanged(comboIndex);
    }
}

// Respond when a solver is selected
void AlgorithmPage::solverChanged(int index) {

    QListWidgetItem* currentItem = m_fieldListWidget->currentItem();
    if (index < 0 || !currentItem || !m_cfg) return;

    // Get the selected solver
    QVariant data = m_solverCombo->itemData(index);
    auto solver = static_cast<FlowCompute::LinearSolver>(index);

    // Update state data
    m_cfg->fieldMathConfigs[currentItem->text()].solver = solver;

    // Block signals
    m_preconditionerSmootherCombo->blockSignals(true);

    // Populate the preconditionerSmoother combo box
    m_preconditionerSmootherCombo->clear();

    if (solver == FlowCompute::LinearSolver::PCG ||
        solver == FlowCompute::LinearSolver::PBiCG ||
        solver == FlowCompute::LinearSolver::PBiCGStab) {

        m_preconditionerSmootherLabel->setText(tr("Preconditioner: "));
        m_preconditionerSmootherLabel->show();
        m_preconditionerSmootherCombo->show();

        // Populate with preconditioners
        QMetaEnum precondEnum = QMetaEnum::fromType<FlowCompute::Preconditioner>();
        for (int i = 0; i < precondEnum.keyCount() - 1; ++i) {
            m_preconditionerSmootherCombo->addItem(precondEnum.key(i), precondEnum.value(i));
        }

        // Safely set preconditioner from state using findData
        auto savedPrecond = m_cfg->fieldMathConfigs[currentItem->text()].preconditioner;
        if (savedPrecond != FlowCompute::Preconditioner::NONE) {
            int comboIndex = m_preconditionerSmootherCombo->findData(static_cast<int>(savedPrecond));
            if (comboIndex >= 0) {
                m_preconditionerSmootherCombo->setCurrentIndex(comboIndex);
            }
        } else if (m_preconditionerSmootherCombo->count() > 0) {
            m_preconditionerSmootherCombo->setCurrentIndex(0);
            m_cfg->fieldMathConfigs[currentItem->text()].preconditioner =
                static_cast<FlowCompute::Preconditioner>(m_preconditionerSmootherCombo->itemData(0).toInt());
        }
    }
    else if (solver == FlowCompute::LinearSolver::smoothSolver ||
        solver == FlowCompute::LinearSolver::GAMG) {

        m_preconditionerSmootherLabel->setText(tr("Smoother: "));
        m_preconditionerSmootherLabel->show();
        m_preconditionerSmootherCombo->show();

        // Populate with smoothers
        QMetaEnum smootherEnum = QMetaEnum::fromType<FlowCompute::Smoother>();
        for (int i = 0; i < smootherEnum.keyCount() - 1; ++i) {
            m_preconditionerSmootherCombo->addItem(smootherEnum.key(i), smootherEnum.value(i));
        }

        // Safely set smoother from state using findData
        auto savedSmoother = m_cfg->fieldMathConfigs[currentItem->text()].smoother;
        if (savedSmoother != FlowCompute::Smoother::NONE) {
            int comboIndex = m_preconditionerSmootherCombo->findData(static_cast<int>(savedSmoother));
            if (comboIndex >= 0) {
                m_preconditionerSmootherCombo->setCurrentIndex(comboIndex);
            }
        } else if (m_preconditionerSmootherCombo->count() > 0) {
            m_preconditionerSmootherCombo->setCurrentIndex(0);
            m_cfg->fieldMathConfigs[currentItem->text()].smoother =
                static_cast<FlowCompute::Smoother>(m_preconditionerSmootherCombo->itemData(0).toInt());
        }
    }
    else {
        m_preconditionerSmootherLabel->hide();
        m_preconditionerSmootherCombo->hide();
    }

    // Unblock signals
    m_preconditionerSmootherCombo->blockSignals(false);
}