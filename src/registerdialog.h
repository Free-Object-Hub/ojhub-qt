#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = 0);

    QString username() const;
    QString password() const;
    QString email() const;

private:
    QLineEdit *m_userEdit;
    QLineEdit *m_passEdit;
    QLineEdit *m_emailEdit;
};

#endif // REGISTERDIALOG_H
