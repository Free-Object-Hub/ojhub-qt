#include "profileview.h"

ProfileView::ProfileView(const PublicProfile &profile, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(profile.displayName().isEmpty()
                   ? QString::fromUtf8("Profile")  // LANGXXX
                   : profile.displayName());
    setMinimumSize(400, 300);
    resize(480, 360);
    setAttribute(Qt::WA_DeleteOnClose);
    setupUi(profile);
}

void ProfileView::setupUi(const PublicProfile &profile)
{
    QVBoxLayout *main = new QVBoxLayout(this);

    QLabel *name = new QLabel(this);
    QFont f = name->font();
    f.setPointSize(f.pointSize() + 3);
    f.setBold(true);
    name->setFont(f);
    name->setText(profile.displayName());
    name->setWordWrap(true);
    main->addWidget(name);

    if (!profile.username.isEmpty() && profile.username != profile.nickname) {
        QLabel *user = new QLabel(QString::fromUtf8("@") + profile.username, this);  // LANGXXX
        user->setStyleSheet(QString::fromLatin1("color: gray;"));
        main->addWidget(user);
    }

    if (profile.userId > 0) {
        QLabel *idLbl = new QLabel(QString::fromUtf8("ID: %1").arg(profile.userId), this);  // LANGXXX
        idLbl->setStyleSheet(QString::fromLatin1("color: gray;"));
        main->addWidget(idLbl);
    }

    if (!profile.socials.isEmpty()) {
        QLabel *soc = new QLabel(QString::fromUtf8("Socials: ") + profile.socials, this);  // LANGXXX
        soc->setWordWrap(true);
        main->addWidget(soc);
    }

    QTextEdit *resume = new QTextEdit(this);
    resume->setReadOnly(true);
    if (!profile.resume.isEmpty())
        resume->setPlainText(profile.resume);
    else
        resume->setPlainText(QString::fromUtf8("(no description)"));  // LANGXXX
    main->addWidget(resume, 1);

    QHBoxLayout *bottom = new QHBoxLayout();
    bottom->addStretch();
    QPushButton *closeBtn = new QPushButton(QString::fromUtf8("Close"), this);  // LANGXXX
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(accept()));
    bottom->addWidget(closeBtn);
    main->addLayout(bottom);
}
