
CONFIG += c++11 console

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qprf
TEMPLATE = app


INCLUDEPATH += /opt/local/include \
    /opt/local/include/eigen3


LIBS += -framework OpenCL -L/opt/local/lib -lmatio -lz -lhdf5


SOURCES += main.cpp\
        prfmainwindow.cpp \
    prfjobsdialog.cpp \
    prfstage2dialog.cpp \
    prfsamplingdialog.cpp \
    qmatfileitemmodel.cpp \
    prfmatnavdialog.cpp \
    qmatfileitemmodel2.cpp \
    qmatfilemodel/qmatfilemodel.cpp \
    qmatfilemodel/qmatarraymodel.cpp \
    qmatfilemodel/qmatcellmodel.cpp \
    qmatfilemodel/qmatstructmodel.cpp \
    qmatfilemodel/qmatcharmodel.cpp \
    qmatfilemodel/qmatfieldmodel.cpp \
    qmatfilemodel/qmatnavigable.cpp \
    prfopencldialog.cpp \
    prfstage2job.cpp \
    prfjoblistmodel.cpp \
    prfsamplingjob.cpp \
    prfcommon.cpp \
    prfsamplingjob_canonical.cpp \
    prfsamplingdialog_canonical.cpp \
    prfsamplingdialog_3param.cpp \
    prfsamplingjob_3param.cpp \
    prfmatconvdialog.cpp # \
    # prfpodfileformat.cpp

HEADERS  += prfmainwindow.h \
    prfjobsdialog.h \
    prfstage2dialog.h \
    prfsamplingdialog.h \
    qmatfileitemmodel.h \
    prfmatnavdialog.h \
    qmatfileitemmodel2.h \
    qmatfilemodel/qmatfilemodel.h \
    qmatfilemodel/qmatarraymodel.h \
    qmatfilemodel/qmatcellmodel.h \
    qmatfilemodel/qmatstructmodel.h \
    qmatfilemodel/qmatcharmodel.h \
    qmatfilemodel/qmatfieldmodel.h \
    qmatfilemodel/qmatnavigable.h \
    prfopencldialog.h \
    prfstage2job.h \
    prftypecastarray.h \
    prfjob.h \
    prfjoblistmodel.h \
    prfsamplingjob.h \
    prfcommon.h \
    prfsamplingjob_canonical.h \
    prfsamplingdialog_canonical.h \
    prfsamplingdialog_3param.h \
    prfsamplingjob_3param.h \
    prfmatconvdialog.h#  \
    # prfpodfileformat.h

FORMS    += prfmainwindow.ui \
    prfjobsdialog.ui \
    prfstage2dialog.ui \
    prfsamplingdialog.ui \
    prfmatnavdialog.ui \
    prfopencldialog.ui \
    prfsamplingdialog_canonical.ui \
    prfsamplingdialog_3param.ui \
    prfmatconvdialog.ui
