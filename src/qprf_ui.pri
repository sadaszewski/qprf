qprf_cmdline {
} else {

QT += gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SOURCES += main.cpp\
        prfmainwindow.cpp \
    prfjobsdialog.cpp \
    prfstage2dialog.cpp \
    prfsamplingdialog.cpp \
    prfmatnavdialog.cpp \
    prfopencldialog.cpp \
    prfjoblistmodel.cpp \
    prfmatconvdialog.cpp \
    prfsamplingdialog_canonical.cpp \
    prfsamplingdialog_3param.cpp

HEADERS  += prfmainwindow.h \
    prfjobsdialog.h \
    prfstage2dialog.h \
    prfsamplingdialog.h \
    prfmatnavdialog.h \
    prfopencldialog.h \
    prfjoblistmodel.h \
    prfmatconvdialog.h \
    prfsamplingdialog_canonical.h \
    prfsamplingdialog_3param.h

FORMS    += prfmainwindow.ui \
    prfjobsdialog.ui \
    prfstage2dialog.ui \
    prfsamplingdialog.ui \
    prfmatnavdialog.ui \
    prfopencldialog.ui \
    prfsamplingdialog_canonical.ui \
    prfsamplingdialog_3param.ui \
    prfmatconvdialog.ui

}
