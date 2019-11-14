#-------------------------------------------------
#
# Project created by QtCreator 2019-08-20T08:56:32
#
#-------------------------------------------------

QT       += core gui
QT += multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ACVC
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        fdragdroplineedit.cpp \
        fedititemdelegate.cpp \
        fedititemmodel.cpp \
        feditsortfilterproxymodel.cpp \
        fedittableview.cpp \
        ffilestreeview.cpp \
        ffoldertreeview.cpp \
        fgenerate.cpp \
        fglobal.cpp \
        flogitemdelegate.cpp \
        flogtableview.cpp \
        fprocessmanager.cpp \
        fpropertyitemdelegate.cpp \
        fpropertysortfilterproxymodel.cpp \
        fpropertytreeview.cpp \
        fstareditor.cpp \
        fstarrating.cpp \
        ftagslistview.cpp \
        ftimeline.cpp \
        fvideowidget.cpp \
        main.cpp \
        mainwindow.cpp \
        qedge.cpp \
        qgraphwidget.cpp \
        qnode.cpp \
        sscrubbar.cpp \
        stimespinbox.cpp

HEADERS += \
        fdragdroplineedit.h \
        fedititemdelegate.h \
        fedititemmodel.h \
        feditsortfilterproxymodel.h \
        fedittableview.h \
        ffilestreeview.h \
        ffoldertreeview.h \
        fgenerate.h \
        fglobal.h \
        flogitemdelegate.h \
        flogtableview.h \
        fprocessmanager.h \
        fpropertyitemdelegate.h \
        fpropertysortfilterproxymodel.h \
        fpropertytreeview.h \
        fstareditor.h \
        fstarrating.h \
        ftagslistview.h \
        ftimeline.h \
        fvideowidget.h \
        mainwindow.h \
        qedge.h \
        qgraphwidget.h \
        qnode.h \
        sscrubbar.h \
        stimespinbox.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:RC_ICONS += acvclogo.ico

RESOURCES += \
    images.qrc
