#ifndef MODEL_LEFT_PANE_H
#define MODEL_LEFT_PANE_H

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

class ModelLeftPane : public QWidget {
    Q_OBJECT

public:
    explicit ModelLeftPane(QWidget* parent = nullptr);

    // Get and set patch names
    std::vector<std::string> getPatchNames() const;
    void setPatchNames(const std::vector<std::string>& patchNames);

signals:
    void surfacePatchRequested(double featureAngle);
    void surfaceCheckRequested();
    void dirtyStateChanged(bool isDirty);

private:
    bool m_isBinary;
    QDoubleSpinBox* m_angleSpin;
    QPushButton *m_checkButton, *m_patchButton;
    QTableWidget *m_patchTable;

private slots:
    void onCheckButtonClicked();
    void onPatchButtonClicked();
};

#endif // MODEL_LEFT_PANE_H
