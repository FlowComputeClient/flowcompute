#ifndef UTILITY_OUTPUT_DIALOG_H
#define UTILITY_OUTPUT_DIALOG_H

#include <QDialog>
#include <QString>
#include <vector>

class QLabel;
class QTextEdit;
class QPushButton;
class QVBoxLayout;

// A clean struct to pass data into the dialog
struct SummaryItem {
    QString text;
    bool isSuccess;
};

class UtilityOutputDialog : public QDialog {
    Q_OBJECT

public:
    explicit UtilityOutputDialog(const QString& title,
                                 const QString& description,
                                 const std::vector<QString>& summaryItems,
                                 const QString& rawLog,
                                 QWidget *parent = nullptr);

private slots:
    void toggleLogViewer(bool checked);

private:
    QTextEdit* m_rawLogTextEdit;
    QPushButton* m_toggleButton;
};

#endif // UTILITY_OUTPUT_DIALOG_H