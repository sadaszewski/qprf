#-------------------------------------------------
#
# Project created by QtCreator 2016-07-22T12:38:54
#
#-------------------------------------------------

CONFIG += c++11 console

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qprf
TEMPLATE = app


INCLUDEPATH += ../../matio-1.5.8/src \
    ../../OpenCL-Headers-master \
    ../../eigen-eigen-dc6cfdf9bcec


win32 {
    contains(QT_ARCH, i386) {
        QMAKE_CXXFLAGS += -std=gnu++1y
        LIBS += "C:/Program Files/NVIDIA Corporation/OpenCL/OpenCL.dll"
        LIBS += F:\Users\sadaszew\Downloads\matio-1.5.8\src\matio.dll \
            F:\Users\sadaszew\Downloads\zlib-1.2.8\zlib.dll \
            F:\Progra~1\HDF_Group\HDF5\1.8.17\bin\hdf5.dll
    } else {
        LIBS += "C:/Program Files/NVIDIA Corporation/OpenCL/OpenCL64.lib"
        LIBS += F:\Users\sadaszew\Downloads\matio-1.5.8-x64\src\matio.lib \
            F:\Users\sadaszew\Downloads\zlib-1.2.8-x64\zlib.lib \
            F:\Progra~2\HDF_Group\HDF5\1.8.17\lib\hdf5.lib
    }
}


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
