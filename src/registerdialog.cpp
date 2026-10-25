#include "registerdialog.h"
#include "lang.h"
#include <QHBoxLayout>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(L("WINDOW-logon2Window"));
    setMinimumWidth(320);

    QVBoxLayout *main = new QVBoxLayout(this);
    QFormLayout *form = new QFormLayout();

    m_userEdit = new QLineEdit(this);
    m_passEdit = new QLineEdit(this);
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_emailEdit = new QLineEdit(this);

    form->addRow(L("login06"), m_userEdit);
    form->addRow(L("login03"), m_emailEdit);
    form->addRow(L("login02"), m_passEdit);
    main->addLayout(form);

    QHBoxLayout *buttons = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton(L("register"), this);
    QPushButton *cancelBtn = new QPushButton(L("otmena"), this);
    connect(okBtn, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));
    buttons->addStretch();
    buttons->addWidget(okBtn);
    buttons->addWidget(cancelBtn);
    main->addLayout(buttons);

    m_userEdit->setFocus();
}

QString RegisterDialog::username() const
{
    return m_userEdit->text().trimmed();
}

QString RegisterDialog::password() const
{
    return m_passEdit->text();
}

QString RegisterDialog::email() const
{
    return m_emailEdit->text().trimmed();
}
