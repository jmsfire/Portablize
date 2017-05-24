TARGET = advapi33

TEMPLATE = lib

CONFIG -= qt
CONFIG(release, debug|release):DEFINES += NDEBUG

QMAKE_LFLAGS += -static
QMAKE_LFLAGS += -nostdlib
contains(QMAKE_HOST.arch, x86_64) {
QMAKE_LFLAGS += -eDllEntryPoint
} else {
QMAKE_LFLAGS += -e_DllEntryPoint
}
QMAKE_CXXFLAGS += -Wpedantic
QMAKE_CXXFLAGS += -Wzero-as-null-pointer-constant

LIBS += -ladvapi32

SOURCES += main.cpp

DEF_FILE = def.def
