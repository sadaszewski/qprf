
CONFIG += c++11 console

QT += core xml

TARGET = qprf
TEMPLATE = app

SOURCES += \
    qmatfilemodel/qmatfilemodel.cpp \
    qmatfilemodel/qmatarraymodel.cpp \
    qmatfilemodel/qmatcellmodel.cpp \
    qmatfilemodel/qmatstructmodel.cpp \
    qmatfilemodel/qmatcharmodel.cpp \
    qmatfilemodel/qmatfieldmodel.cpp \
    qmatfilemodel/qmatnavigable.cpp \
    prfstage2job.cpp \
    prfsamplingjob.cpp \
    prfcommon.cpp \
    prfsamplingjob_canonical.cpp \
    prfsamplingjob_3param.cpp \

HEADERS  += \
    qmatfilemodel/qmatfilemodel.h \
    qmatfilemodel/qmatarraymodel.h \
    qmatfilemodel/qmatcellmodel.h \
    qmatfilemodel/qmatstructmodel.h \
    qmatfilemodel/qmatcharmodel.h \
    qmatfilemodel/qmatfieldmodel.h \
    qmatfilemodel/qmatnavigable.h \
    prfstage2job.h \
    prftypecastarray.h \
    prfjob.h \
    prfsamplingjob.h \
    prfcommon.h \
    prfsamplingjob_canonical.h \
    prfsamplingjob_3param.h

