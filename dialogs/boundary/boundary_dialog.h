#ifndef BOUNDARY_DIALOG_H
#define BOUNDARY_DIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QDialogButtonBox>

#include "../../../core_types.h"

class BoundaryDialog : public QDialog {
    Q_OBJECT

public:
    BoundaryDialog(QString patchName, QString patchType, QStringList fields,
                   const QHash<QString, FlowCompute::FieldDef>& fieldData,
                   const std::vector<FlowCompute::BoundaryConditionDef>& boundaryConditions,
                   QWidget* parent = nullptr);
    std::unordered_map<QString, FlowCompute::BoundaryCondition> getPatchStates() { return m_patchStates; }

private:
    std::unordered_map<QString, QComboBox*> m_boundaryTypeCombos;
    std::unordered_map<QString, QGroupBox*> m_paramGroups;
    std::unordered_map<QString, QFormLayout*> m_paramLayouts;
    QListWidget* m_fieldListWidget;
    QStackedWidget* m_stackedWidget;
    QString m_currentField, m_patchName, m_patchType, m_currentDataType;
    QStringList m_fields;
    QHash<QString, FlowCompute::FieldDef> m_fieldData;
    std::vector<FlowCompute::BoundaryConditionDef> m_boundaryConditions;
    QMap<QString, QStringList> m_paramMap;
    std::unordered_map<QString, FlowCompute::BoundaryCondition> m_patchStates;

    void saveCurrentState(); // Helper to extract UI data into m_patchStates

private slots:
    void bcChanged(const QString& fieldName, const QString& text);
};

#endif // BOUNDARY_DIALOG_H
