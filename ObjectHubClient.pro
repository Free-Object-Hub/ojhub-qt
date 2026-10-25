# Object Hub Native Client 1.0.0
# Core + domains (see docs/ARCHITECTURE.md). Qt Widgets.
# Checked: Qt 5.15. Targets: Qt 5.6+ (incl. XP/MinGW) and Qt 6.x. Android: Qt 6.x.

QT       += core gui network widgets

TARGET   = ObjectHubClient
TEMPLATE = app
CONFIG  += c++11
DEFINES += QT_DEPRECATED_WARNINGS

VERSION  = 1.0.0

INCLUDEPATH += src src/core src/api src/domain/account src/domain/catalog src/domain/detail src/domain/profile src/domain/social src/domain/projects src/domain/vacancies src/domain/news src/domain/portfolio src/domain/authoring

SOURCES += \
    src/main.cpp \
    src/settings.cpp \
    src/mainwindow.cpp \
    src/resultsmodel.cpp \
    src/profileview.cpp \
    src/logindialog.cpp \
    src/registerdialog.cpp \
    src/lang.cpp \
    src/core/session.cpp \
    src/core/network.cpp \
    src/core/protocol.cpp \
    src/domain/account/accountdomain.cpp \
    src/domain/catalog/catalogdomain.cpp \
    src/domain/catalog/catalogparse.cpp \
    src/domain/detail/detaildomain.cpp \
    src/domain/profile/profiledomain.cpp \
    src/domain/social/socialdomain.cpp \
    src/domain/projects/projectsdomain.cpp \
    src/domain/vacancies/vacanciesdomain.cpp \
    src/domain/news/newsdomain.cpp \
    src/domain/portfolio/portfoliodomain.cpp \
    src/domain/authoring/formparse.cpp \
    src/api/apiclient.cpp \
    src/projectdialog.cpp \
    src/vacancydialog.cpp \
    src/newsdialog.cpp \
    src/socialdialogs.cpp \
    src/tagpicker.cpp \
    src/tagnames.cpp \
    src/workdialog.cpp \
    src/altcha/sha256.c \
    src/altcha/altcha.c

HEADERS += \
    src/version.h \
    src/settings.h \
    src/mainwindow.h \
    src/resultsmodel.h \
    src/profileview.h \
    src/logindialog.h \
    src/registerdialog.h \
    src/lang.h \
    src/core/types.h \
    src/core/session.h \
    src/core/network.h \
    src/core/protocol.h \
    src/domain/account/accountdomain.h \
    src/domain/catalog/catalogdomain.h \
    src/domain/catalog/catalogparse.h \
    src/domain/detail/detaildomain.h \
    src/domain/profile/profiledomain.h \
    src/domain/social/socialdomain.h \
    src/domain/projects/projectsdomain.h \
    src/domain/vacancies/vacanciesdomain.h \
    src/domain/news/newsdomain.h \
    src/domain/portfolio/portfoliodomain.h \
    src/domain/authoring/formparse.h \
    src/api/apiclient.h \
    src/api/searchresult.h \
    src/api/forms.h \
    src/projectdialog.h \
    src/vacancydialog.h \
    src/newsdialog.h \
    src/socialdialogs.h \
    src/tagpicker.h \
    src/tagnames.h \
    src/workdialog.h \
    src/altcha/sha256.h \
    src/altcha/altcha.h

RESOURCES += resources/resources.qrc

android {
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}

unix:!android {
    target.path = /usr/local/bin
    INSTALLS += target
}
