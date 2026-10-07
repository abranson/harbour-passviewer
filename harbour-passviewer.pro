# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

# The name of your application
TARGET = harbour-passviewer

CONFIG += sailfishapp
CONFIG += link_pkgconfig
PKGCONFIG += sailfishapp zxing
QMAKE_CXXFLAGS += -std=c++17

SOURCES += src/harbour-passviewer.cpp \
    src/passsharer.cpp \
    src/barcodescanner.cpp \
    src/barcodecodec.cpp \
    src/savedcards.cpp \
    src/cardicons.cpp \
    src/passimporter.cpp \
    src/barcodeimageprovider.cpp \
    src/homewatcher.cpp \
    src/settingsstore.cpp \
    src/zipfile.cpp \
    src/zipfileimageprovider.cpp \
    src/datetimeformat.cpp \
    src/currencyformat.cpp \
    src/passhandler.cpp \
    src/homescanner.cpp \
    src/passdb.cpp \
    src/passinfo.cpp \
    src/notificator.cpp

OTHER_FILES += qml/harbour-passviewer.qml \
    qml/pages/ScanCard.qml \
    qml/pages/SaveCard.qml \
    qml/pages/SaveCardForm.qml \
    qml/pages/ShowCard.qml \
    qml/pages/CardIcon.qml \
    qml/pages/ImportPass.qml \
    qml/cover/CoverPage.qml \
    qml/pages/FirstPage.qml \
    qml/pages/Archive.qml \
    rpm/harbour-passviewer.spec \
    rpm/harbour-passviewer.yaml \
    translations/*.ts \
    harbour-passviewer.desktop \
    qml/pages/ShowBack.qml \
    qml/pages/ShowSimple.qml \
    qml/pages/ShowCodeFullscreen.qml \
    qml/pages/Copyright.qml \
    qml/pages/Settings.qml \
    rpm/harbour-passviewer.changes \
    qml/pages/ShowPass.qml \
    qml/lib/Pass.qml \
    qml/lib/utils.js \
    qml/lib/timeline.js \
    qml/lib/Back.qml

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# to disable building translations every time, comment out the
# following CONFIG line
CONFIG += sailfishapp_i18n

# German translation is enabled as an example. If you aren't
# planning to localize your app, remember to comment out the
# following TRANSLATIONS line. And also do not forget to
# modify the localized app name in the the .desktop file.
TRANSLATIONS += translations/harbour-passviewer-de.ts
TRANSLATIONS += translations/harbour-passviewer-sv.ts
TRANSLATIONS += translations/harbour-passviewer-ru.ts
TRANSLATIONS += translations/harbour-passviewer-hu.ts
TRANSLATIONS += translations/harbour-passviewer-fr.ts
TRANSLATIONS += translations/harbour-passviewer-es.ts

HEADERS += \
    src/passsharer.h \
    src/barcodescanner.h \
    src/barcodecodec.h \
    src/savedcards.h \
    src/cardicons.h \
    src/passimporter.h \
    src/barcodeimageprovider.h \
    src/homewatcher.h \
    src/settingsstore.h \
    src/zipfile.h \
    src/zipfileimageprovider.h \
    src/datetimeformat.h \
    src/currencyformat.h \
    src/passhandler.h \
    src/homescanner.h \
    src/passdb.h \
    src/passinfo.h \
    src/notificator.h

QT += multimedia concurrent
QT += network
QT += sql
QT += positioning
QT += dbus

LIBS += -lz
LIBS += -lbz2
LIBS += -llzma
LIBS += -lnemonotifications-qt5

DISTFILES += \
    qml/lib/currencies.json

INCLUDEPATH += /usr/include/nemonotifications-qt5

pass_mime.files = data/harbour-passviewer.xml
pass_mime.path = /usr/share/mime/packages
INSTALLS += pass_mime
