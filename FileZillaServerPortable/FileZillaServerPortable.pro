TARGET = winmm

TEMPLATE = lib

QMAKE_LFLAGS += -static
QMAKE_CXXFLAGS += -Wpedantic

LIBS += -lole32

SOURCES += main.cpp

DEF_FILE = FileZillaServerPortable.def
