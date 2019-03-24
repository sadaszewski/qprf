include(qprf_base.pri)
include(qprf_cmd.pri)
include(qprf_ui.pri)

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
