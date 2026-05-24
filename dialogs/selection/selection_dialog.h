#ifndef SELECTION_DIALOG_H
#define SELECTION_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

class SelectionDialog : public QDialog {
    Q_OBJECT
public:
    // Pass the title, the prompt, and the options in the constructor
    SelectionDialog(const QString& title, const QString& prompt, const QStringList& items, QWidget* parent = nullptr);

    QString getSelectedItem() const;

private:
    QListWidget* m_listWidget;
};

#endif // SELECTION_DIALOG_H
