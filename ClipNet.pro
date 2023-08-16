QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# choose your (pseudo-)cryptographic poison
CONFIG += cryptopp
#CONFIG += simplecrypt

CONFIG(debug, debug|release) {
    DEFINES += QT_DEBUG
    DESTDIR = deploy/debug
} else {
    DESTDIR = deploy/release
}

RESOURCES += ./ClipNet.qrc

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

mac {
    DEFINES += QT_OSX
}

unix:!mac {
    DEFINES += QT_LINUX
}

win32 {
    DEFINES += QT_WIN

    # for Registry API
    LIBS += -ladvapi32

    CONFIG(static) {
        # make sure we match the linkage for Crypto++
        CONFIG(debug, debug|release) {
            QMAKE_CFLAGS += /MTd
            QMAKE_CXXFLAGS += /MTd
        } else {
            QMAKE_CFLAGS += /MT
            QMAKE_CXXFLAGS += /MT
        }
    }
}

SOURCES += \
    Receiver.cpp \
    Secure.cpp \
    Sender.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Receiver.h \
    Secure.h \
    Sender.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

cryptopp {
    CRYPTOPP_PREFIX = $$(CRYPTOPP_INSTALL_ROOT)
    isEmpty(CRYPTOPP_PREFIX){
        win32 {
            CRYPTOPP_PREFIX = M:\Projects\cryptopp
        }
        unix:!mac {
            CRYPTOPP_PREFIX = /home/bob/projects/cryptopp
        }
    }

    # from a security standpoint, Crypt++ is preferrable to
    # SimpleCrypt (as SimpleCrypt is preferrable to nothing
    # at all)...

    DEFINES += USE_ENCRYPTION
    DEFINES += CRYPTOPP

    INCLUDEPATH += $$CRYPTOPP_PREFIX

    win32 {
        LIBS += -lcryptlib
        CONFIG(debug, debug|release) {
            LIBS += -L$$CRYPTOPP_PREFIX\x64\Output\Debug
        } else {
            LIBS += -L$$CRYPTOPP_PREFIX\x64\Output\Release
        }
    }
    unix:!mac {
        LIBS += -lcryptopp
        LIBS += -L$$CRYPTOPP_PREFIX
    }
}

simplecrypt {
    # ...however, if you're exchanging clipbaord data
    # between machines on an isolated network, or you
    # aren't paranoid enough to use a heavier crypto
    # solution like AES, then SimpleCrypt's obfuscation
    # would likely be just fine for you.

    DEFINES += USE_ENCRYPTION
    DEFINES += SIMPLECRYPT

    SOURCES += SimpleCrypt.cpp
    HEADERS += SimpleCrypt.h
}

INTERMEDIATE_NAME = intermediate
MOC_DIR = $$INTERMEDIATE_NAME/moc
OBJECTS_DIR = $$INTERMEDIATE_NAME/obj
RCC_DIR = $$INTERMEDIATE_NAME/rcc
UI_DIR = $$INTERMEDIATE_NAME/ui

win32:RC_FILE = ClipNet.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
