#ifndef RESULT_LEFT_PANE_H
#define RESULT_LEFT_PANE_H

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "../../../core_types.h"

class ResultLeftPane : public QWidget {
    Q_OBJECT

public:
    explicit ResultLeftPane(const QStringList& timeFolders, QWidget* parent = nullptr);

signals:
    void dirtyStateChanged(bool isDirty);
    void meshCheckRequested();
    void meshPatchRequested(double angle);
    void meshRenumberRequested();
    void patchApplyRequested(std::vector<FlowCompute::MeshPatch>& patches);

private:
    QDoubleSpinBox* m_angleSpin;
    QHash<QString, FlowCompute::FieldData> m_fieldEditorMap;
    QPushButton *m_checkButton, *m_renumberButton, *m_patchButton, *m_applyButton;
    QString m_currentField;
    QStringList m_timeFolders;
    QTableWidget *m_patchTable;

    QHash<QString, FlowCompute::FieldDef> m_fieldData;
    std::vector<FlowCompute::BoundaryConditionDef> m_boundaryConditions;
    std::vector<FlowCompute::MeshPatch> m_boundaryPatches;
};

#endif // RESULT_LEFT_PANE_H
