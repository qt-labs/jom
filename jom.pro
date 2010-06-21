TEMPLATE = subdirs
SUBDIRS = src/jomlib src/app tests
CONFIG += ordered

src/app.depends = src/jomlib
tests.depends = src/jomlib
