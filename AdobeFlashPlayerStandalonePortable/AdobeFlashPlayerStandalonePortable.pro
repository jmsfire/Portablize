TARGET = dinput8

TEMPLATE = lib

QMAKE_LFLAGS += -static
QMAKE_CXXFLAGS += -Wpedantic

SOURCES += main.cpp

DEF_FILE = AdobeFlashPlayerStandalonePortable.def
