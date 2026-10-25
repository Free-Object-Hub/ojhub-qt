#ifndef PROFILEVIEW_H
#define PROFILEVIEW_H

#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "api/searchresult.h"

class ProfileView : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileView(const PublicProfile &profile, QWidget *parent = 0);

private:
    void setupUi(const PublicProfile &profile);
};

#endif // PROFILEVIEW_H
