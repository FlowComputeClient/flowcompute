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
    void setBounds(std::array<float, 3> bounds);
    void changeBounds(double scaleFactor);

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void surfaceCheckRequested();
    void surfaceScaleRequested(double scaleFactor);
    void surfacePatchRequested(double featureAngle);
    void dirtyStateChanged(bool isDirty);

private:
    bool m_isBinary;
    std::array<float, 3> m_bounds;
    QDoubleSpinBox *m_angleSpin, *m_scaleSpin;
    QLabel* m_boundsLabel;
    QPushButton *m_checkButton, *m_patchButton, *m_scaleButton;
    QTableWidget *m_patchTable;

private slots:
    void onCheckButtonClicked();
    void onScaleButtonClicked();
    void onPatchButtonClicked();
};

#endif // MODEL_LEFT_PANE_H
