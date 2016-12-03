TARGET = shfolder

TEMPLATE = lib

CONFIG -= qt
CONFIG(release, debug|release):DEFINES += NDEBUG

QMAKE_LFLAGS += -static
QMAKE_LFLAGS += -nostdlib
contains(QMAKE_HOST.arch, x86_64) {
QMAKE_LFLAGS += -eDllEntryPoint
} else {
QMAKE_LFLAGS += -e_DllEntryPoint

LIBS += -lkernel32

DEF_FILE = def.def
}
QMAKE_CXXFLAGS += -Wpedantic
QMAKE_CXXFLAGS += -Wzero-as-null-pointer-constant

SOURCES += main.cpp
