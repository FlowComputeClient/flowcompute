#ifndef LEFT_PANE_H
#define LEFT_PANE_H

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class LeftPane : public QWidget {
    Q_OBJECT

public:
    explicit LeftPane(QWidget* parent = nullptr);

signals:
    void autoPatchRequested(double featureAngle);
    void surfaceCheckRequested();

private:
    bool m_isBinary;
    QCheckBox* m_overwriteCheck;
    QDoubleSpinBox* m_angleSpin;
    QPushButton *m_checkButton, *m_patchButton;

private slots:
    void onCheckButtonClicked();
    void onPatchButtonClicked();
};

#endif // LEFT_PANE_H
