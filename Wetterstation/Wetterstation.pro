QT = core sql
HEADERS += serielle.h \
    buildinformation.h \
    dbconnection.h
SOURCES += serielle.cpp \
    Wetterstation.cpp \
    buildinformation.cpp \
    dbconnection.cpp
target.path = /usr/bin/ # hierhin wird die ausfuehrbare Datei kopiert
INSTALLS += target
