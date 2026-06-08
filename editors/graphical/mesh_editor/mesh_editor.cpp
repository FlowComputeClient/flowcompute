#include "mesh_editor.h"

#include <algorithm>

#include <QMetaEnum>

#include "../../../dialogs/solver/solver_io.h"
#include "../../../main_window.h"
#include "../../../parser/open_foam_dictionary.h"
#include "../../../utils.h"

MeshEditor::MeshEditor(std::shared_ptr<RenderData> renderData,
                       const QString& casePath, int targetId,
                       const std::vector<FlowCompute::SolverFamily>& families,
                       const FlowCompute::TurbulenceDatabase& turbModels,
                       const QHash<QString, FlowCompute::FieldDef>& fieldData,
                       const std::vector<FlowCompute::BoundaryConditionDef>& boundaryConditions,
                       QVulkanInstance* instance, QWidget* parent):
    QWidget(parent), m_renderData(renderData), m_casePath(casePath), m_targetId(targetId),
    m_families(families), m_turbModels(turbModels), m_fieldData(fieldData), m_vulkanInstance(instance) {

    // Get the current solver
    QString solver = "simpleFoam";
    m_mainWin = qobject_cast<MainWindow*>(this->parent());
    QByteArray fileData = m_mainWin->targetSystems[targetId]->getFileContent(casePath + "/system/controlDict");
    if (!fileData.isEmpty()) {
        QRegularExpression regex("^[ \\t]*application\\s+([^;\\s]+)\\s*;",
                                 QRegularExpression::MultilineOption);
        QRegularExpressionMatch match = regex.match(QString::fromUtf8(fileData));
        if (match.hasMatch()) {
            solver = match.captured(1);
        }
    }

    // Get the turbulence model
    QString simulationType = "laminar";
    QString modelType = "none";
    fileData = m_mainWin->targetSystems[targetId]->getFileContent(casePath + "/constant/turbulenceProperties");
    if (!fileData.isEmpty()) {
        QRegularExpression simTypeRegex("^[ \\t]*simulationType\\s+([^;\\s]+)\\s*;",
                                        QRegularExpression::MultilineOption);
        QString content = QString::fromUtf8(fileData);
        QRegularExpressionMatch match = simTypeRegex.match(content);
        if (match.hasMatch()) {
            simulationType = match.captured(1);
            if (simulationType == "RAS" || simulationType == "LES") {
                QString modelKeyword = simulationType + "Model";
                QRegularExpression modelRegex("^[ \\t]*" + modelKeyword + "\\s+([^;\\s]+)\\s*;",
                    QRegularExpression::MultilineOption);
                QRegularExpressionMatch modelMatch = modelRegex.match(content);
                if (modelMatch.hasMatch()) {
                    modelType = modelMatch.captured(1);
                }
            }
        }
    }

    // Create the list of fields
    QStringList fieldList = getSolverFields(solver) + getTurbulenceFields(simulationType, modelType);
    fieldList.removeDuplicates();

    // Create left pane
    m_leftPane = new MeshLeftPane(fieldList, fieldData, boundaryConditions, this);
    updatePatches();
    m_leftPane->setFixedWidth(230);

    // Create the mesh editor
    QWidget* rightPane;
    if(!renderData->data.empty()) {
        m_vulkanWindow = new VulkanWindow(m_renderData);
        m_vulkanWindow->setVulkanInstance(m_vulkanInstance);

        // Create widget for the window (right pane)
        rightPane = QWidget::createWindowContainer(m_vulkanWindow);
        rightPane->setAttribute(Qt::WA_OpaquePaintEvent, true);
        rightPane->setAttribute(Qt::WA_NoSystemBackground, true);
        rightPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        rightPane->setFocusPolicy(Qt::StrongFocus);
        rightPane->setAttribute(Qt::WA_DeleteOnClose);
    } else {
        rightPane = new QWidget(this);
    }

    // Layout for the main widget
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_leftPane);
    layout->addWidget(rightPane, 1);

    setLayout(layout);

    // Handle events from the left pane
    connect(m_leftPane, &MeshLeftPane::meshPatchRequested,
        this, &MeshEditor::onMeshPatchRequest);
    connect(m_leftPane, &MeshLeftPane::meshCheckRequested,
        this, &MeshEditor::onMeshCheckRequest);
    connect(m_leftPane, &MeshLeftPane::meshRenumberRequested,
        this, &MeshEditor::onMeshRenumberRequest);
    connect(m_leftPane, &MeshLeftPane::patchApplyRequested,
        this, &MeshEditor::onPatchApply);
}

QStringList MeshEditor::getSolverFields(const QString& solver) {

    QStringList solverFields;

    // Iterate through families
    for (const auto& family : std::as_const(m_families)) {
        for (const auto& solverDetail : family.solvers) {
            if (solverDetail.name == solver) {
                return solverDetail.fields;
            }
        }
    }
    return solverFields;
}

QStringList MeshEditor::getTurbulenceFields(const QString& simulationType, const QString& model) {

    // Iterate through the outer map (simulation types)
    for (auto outerIt = m_turbModels.cbegin(); outerIt != m_turbModels.cend(); ++outerIt) {

        // Check if the outer key contains the target simulationType
        if (outerIt.key().contains(simulationType)) {

            // outerIt.value() is the inner QMap<QString, QList<TurbulenceModel>>
            for (const auto& turbList : outerIt.value()) {

                // Iterate through the actual TurbulenceModel structs in the list
                for (const auto& turb : turbList) {

                    // If the name matches, we found our target. Return the fields immediately.
                    if (turb.name == model) {
                        return turb.fields;
                    }
                }
            }
        }
    }
    return QStringList();
}

// Update surface data
void MeshEditor::updateModel(std::shared_ptr<RenderData> newMesh) {

    // Update data
    m_renderData = newMesh;

    // Render new mesh data
    m_vulkanWindow->setRenderData(m_renderData);

    // Update patches in the left pane
    updatePatches();

    // Set dirty state
    m_isSurfaceChanged = true;
    emit dirtyStateChanged(true);
}

void MeshEditor::updatePatches() {

    // Get list of boundaries
    QByteArray fileData = m_mainWin->targetSystems[m_targetId]->getFileContent(m_casePath + "/constant/polyMesh/boundary");
    if (!fileData.isEmpty()) {
        m_boundaries = SolverIO::parseBoundaryPatches(fileData);
    }

    // Get list of patch names
    for(const auto& patch: m_renderData->patches) {
        m_patchNames.push_back(std::string(patch.name));
    }

    // Make sure the lists of boundaries match
    m_boundaries.erase(
        std::remove_if(m_boundaries.begin(), m_boundaries.end(),
           [this](const FlowCompute::MeshPatch& boundary) {
               std::string nameToCheck = boundary.name.toStdString();
               return std::find(m_patchNames.begin(), m_patchNames.end(), nameToCheck) == m_patchNames.end();
           }), m_boundaries.end());

    m_leftPane->setPatches(m_boundaries);
}

void MeshEditor::onPatchApply(std::vector<FlowCompute::MeshPatch>& patches) {

    // Select only patches whose names or types have changed
    std::vector<FlowCompute::MeshPatch> filtered;
    std::copy_if(patches.begin(), patches.end(),
        std::back_inserter(filtered), [](const FlowCompute::MeshPatch& bp) {
        return bp.nameChanged || bp.typeChanged;
    });

    // Update boundary file
    QByteArray fileData = m_mainWin->targetSystems[m_targetId]->getFileContent(m_casePath + "/constant/polyMesh/boundary");
    SolverIO::BoundaryFileParts parts = SolverIO::splitBoundaryFile(fileData);
    auto dict = std::make_shared<OpenFoamDictionary>(parts.payload);
    if(!dict->hasSyntaxErrors()) {

        // Create updated text
        QString updatedPayload = SolverIO::updateBoundaryFile(dict, filtered);

        // Rebuild data
        QByteArray finalFileData;
        finalFileData.append(parts.header);
        finalFileData.append("(\n");
        finalFileData.append(updatedPayload.toUtf8());
        finalFileData.append("\n)\n");

        // Update data
        m_mainWin->targetSystems[m_targetId]->writeData(finalFileData, m_casePath + "/constant/polyMesh/boundary");
    } else {
        QMessageBox errorDialog(this);
        errorDialog.setWindowTitle("Parse Error");
        errorDialog.setText(QString("<b>Failed to parse boundary file</b>"));
        errorDialog.setInformativeText("The file may contain syntax errors or unsupported keywords.");
        errorDialog.setIcon(QMessageBox::Warning);
        errorDialog.exec();
    }

    // Create field map
    QMetaEnum metaEnum = QMetaEnum::fromType<FlowCompute::FieldClass>();
    std::unordered_map<QString, FlowCompute::FieldData> fieldMap;
    for(auto const& patch: patches) {
        for (const auto& [fieldName, bcData] : patch.bcs) {

            if (!fieldMap.contains(fieldName)) {

                // Create FieldData for field
                FlowCompute::FieldData fieldData;
                fieldData.fieldClass = metaEnum.valueToKey(static_cast<int>(
                    m_fieldData[fieldName].fieldClass));
                fieldData.dimension = m_fieldData[fieldName].dimensions;
                fieldData.internalField = m_fieldData[fieldName].defaultValue;
                fieldMap[fieldName] = fieldData;
            }
            fieldMap[fieldName].bcs[patch.name] = bcData;
        }
    }

    // Get OpenFoamPath
    QString caseName = m_casePath.split("/").last();
    QString openFoamPath = m_mainWin->m_caseMap[caseName].openFoamPath;

    // Create field files
    QString path = m_casePath + "/0.orig/";
    for (const auto& [fieldName, fieldData] : fieldMap) {
        QString fieldFile = Utils::createFieldFile(openFoamPath, fieldName, fieldData);
        m_mainWin->targetSystems[m_targetId]->writeData(fieldFile.toUtf8(), path + fieldName);
    }
}

void MeshEditor::onMeshCheckRequest() {
    emit meshCheckRequested(m_casePath, m_targetId);
}

void MeshEditor::onMeshPatchRequest(double featureAngle) {
    emit meshPatchRequested(featureAngle, m_casePath, m_targetId);
}

void MeshEditor::onMeshRenumberRequest() {
    emit meshRenumberRequested(m_casePath, m_targetId);
}