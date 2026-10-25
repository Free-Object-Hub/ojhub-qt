#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);

    QString username() const;
    QString password() const;

private:
    QLineEdit *m_userEdit;
    QLineEdit *m_passEdit;
};

#endif // LOGINDIALOG_H
