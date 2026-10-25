#include "logindialog.h"
#include "lang.h"
#include <QHBoxLayout>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(L("WINDOW-logonWindow"));
    setMinimumWidth(320);

    QVBoxLayout *main = new QVBoxLayout(this);
    QFormLayout *form = new QFormLayout();

    m_userEdit = new QLineEdit(this);
    m_passEdit = new QLineEdit(this);
    m_passEdit->setEchoMode(QLineEdit::Password);

    form->addRow(L("login01"), m_userEdit);
    form->addRow(L("login02"), m_passEdit);
    main->addLayout(form);

    QHBoxLayout *buttons = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton(L("login"), this);
    QPushButton *cancelBtn = new QPushButton(L("otmena"), this);
    connect(okBtn, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));
    buttons->addStretch();
    buttons->addWidget(okBtn);
    buttons->addWidget(cancelBtn);
    main->addLayout(buttons);

    m_userEdit->setFocus();
}

QString LoginDialog::username() const
{
    return m_userEdit->text().trimmed();
}

QString LoginDialog::password() const
{
    return m_passEdit->text();
}
