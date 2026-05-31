#ifndef MESH_LEFT_PANE_H
#define MESH_LEFT_PANE_H

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

class MeshLeftPane : public QWidget {
    Q_OBJECT

public:
    explicit MeshLeftPane(QWidget* parent = nullptr);
};

#endif // MESH_LEFT_PANE_H
