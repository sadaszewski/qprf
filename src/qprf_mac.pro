include(qprf_base.pri)
include(qprf_cmd.pri)
include(qprf_ui.pri)

INCLUDEPATH += /opt/local/include \
    /opt/local/include/eigen3

LIBS += -framework OpenCL -L/opt/local/lib -lmatio -lz -lhdf5

QMAKE_MAC_SDK = macosx10.14
