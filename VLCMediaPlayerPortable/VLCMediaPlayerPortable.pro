TARGET = user33

TEMPLATE = lib

QMAKE_LFLAGS += -static
QMAKE_CXXFLAGS += -Wpedantic

SOURCES += main.cpp

contains(QMAKE_HOST.arch, x86_64) {
SOURCES += \
    $$PWD/MinHook/src/*.c \
    $$PWD/MinHook/src/HDE/*.c

HEADERS += \
    $$PWD/MinHook/src/*.h
    $$PWD/MinHook/src/HDE/*.h
}

DEF_FILE = VLCMediaPlayerPortable.def
