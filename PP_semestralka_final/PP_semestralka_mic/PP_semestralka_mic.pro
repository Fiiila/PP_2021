QT += widgets network

TARGET = PP_semestralka_mic

win32 {
    TEMPLATE = vcapp
    GUID = {E60050D1-7955-409A-AB4A-E330A102A510}
}

unix {
    TEMPLATE = app
}

SOURCES += main.cpp \
      renderarea.cpp \
      window.cpp \

HEADERS += renderarea.h \
        window.h\
