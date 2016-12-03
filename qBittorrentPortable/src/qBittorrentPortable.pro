TARGET = shell33

TEMPLATE = lib

CONFIG -= qt
CONFIG(release, debug|release):DEFINES += NDEBUG

QMAKE_LFLAGS += -static
QMAKE_LFLAGS += -nostdlib
contains(QMAKE_HOST.arch, x86_64) {
QMAKE_LFLAGS += -eDllEntryPoint

SOURCES += \
$$PWD/MinHook/buffer.cpp \
$$PWD/MinHook/hde.cpp \
$$PWD/MinHook/hook.cpp \
$$PWD/MinHook/trampoline.cpp

HEADERS += \
$$PWD/MinHook/buffer.h \
$$PWD/MinHook/hde.h \
$$PWD/MinHook/trampoline.h
} else {
QMAKE_LFLAGS += -e_DllEntryPoint
}
QMAKE_CXXFLAGS += -Wpedantic
QMAKE_CXXFLAGS += -Wzero-as-null-pointer-constant

LIBS += -lkernel32 -ladvapi32 -lshell32

SOURCES += main.cpp

DEF_FILE = def.def
