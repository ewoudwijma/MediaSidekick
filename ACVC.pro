#-------------------------------------------------
#
# Project created by QtCreator 2019-08-20T08:56:32
#
#-------------------------------------------------

QT += core gui
QT += multimedia multimediawidgets quickwidgets

QT += qml network quick positioning location


win32: QT += winextras

# Workaround for QTBUG-38735
#QT_FOR_CONFIG += location-private
#qtConfig(geoservices_mapboxgl): QT += sql opengl
#qtConfig(geoservices_osm): QT += concurrent


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
        aclipsitemdelegate.cpp \
        aclipsitemmodel.cpp \
        aclipssortfilterproxymodel.cpp \
        aclipstableview.cpp \
        aderperviewmain.cpp \
        aderperviewprocess.cpp \
        aderperviewvideo.cpp \
        adragdroplineedit.cpp \
        aexport.cpp \
        afilessortfilterproxymodel.cpp \
        afilestreeview.cpp \
        afoldertreeview.cpp \
        agcliprectangleitem.cpp \
        ageocoding.cpp \
        agfilesystem.cpp \
        aglobal.cpp \
        agmediarectangleitem.cpp \
        agview.cpp \
        ajobitemdelegate.cpp \
        ajobthread.cpp \
        ajobtreeview.cpp \
        apropertyeditordialog.cpp \
        apropertyitemdelegate.cpp \
        apropertysortfilterproxymodel.cpp \
        apropertytreeview.cpp \
        aspinnerlabel.cpp \
        astareditor.cpp \
        astarrating.cpp \
        atagslistview.cpp \
        atimeline.cpp \
        avideowidget.cpp \
        main.cpp \
        mainwindow.cpp \
        qedge.cpp \
        qgraphwidget.cpp \
        qnode.cpp \
        sscrubbar.cpp \
        stimespinbox.cpp

HEADERS += \
        aclipsitemdelegate.h \
        aclipsitemmodel.h \
        aclipssortfilterproxymodel.h \
        aclipstableview.h \
        aderperviewmain.h \
        aderperviewprocess.h \
        aderperviewvideo.h \
        adragdroplineedit.h \
        aexport.h \
        afilessortfilterproxymodel.h \
        afilestreeview.h \
        afoldertreeview.h \
        agcliprectangleitem.h \
        ageocoding.h \
        agfilesystem.h \
        aglobal.h \
        agmediarectangleitem.h \
        agview.h \
        ajobitemdelegate.h \
        ajobthread.h \
        ajobtreeview.h \
        apropertyeditordialog.h \
        apropertyitemdelegate.h \
        apropertysortfilterproxymodel.h \
        apropertytreeview.h \
        aspinnerlabel.h \
        astareditor.h \
        astarrating.h \
        atagslistview.h \
        atimeline.h \
        avideowidget.h \
        mainwindow.h \
        qedge.h \
        qgraphwidget.h \
        qnode.h \
        sscrubbar.h \
        stimespinbox.h

FORMS += \
        mainwindow.ui \
        propertyeditordialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:RC_ICONS += acvc.ico
unix:ICON = acvc.icns

RESOURCES += \
    images.qrc \
    qml.qrc

win32: LIBS += -LD:\ACVC\windows\ffmpeg-latest-win64-dev\lib
win32: LIBS += -lavcodec -lavformat -lavutil -lswscale #-lavdevice -lavfilter -lpostproc -lswresample
win32: INCLUDEPATH +=D:\ACVC\windows\ffmpeg-latest-win64-dev\include

unix:LIBS += -L/Users/ewoudwijma/Downloads/ffmpeg-latest-macos64-shared/bin
unix:LIBS += -lavcodec.58 -lavformat.58 -lavutil.56 -lswscale.5 #-lavdevice -lavfilter -lpostproc -lswresample
unix:INCLUDEPATH += /Users/ewoudwijma/Downloads/ffmpeg-latest-macos64-dev/include
