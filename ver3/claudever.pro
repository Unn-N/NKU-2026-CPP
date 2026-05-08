QT += widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    rectitem.cpp \
    widget.cpp \
    demowidget.cpp \
    racewidget.cpp

HEADERS += \
    rectitem.h \
    widget.h \
    demowidget.h \
    racewidget.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
