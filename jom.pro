TEMPLATE = subdirs
sub_jomlib.subdir = src/jomlib
sub_app.subdir = src/app
sub_app.depends = sub_jomlib
sub_tests.subdir = tests
sub_tests.depends = sub_jomlib sub_app
SUBDIRS = sub_app sub_jomlib sub_tests

OTHER_FILES = \
    changelog.txt

defineTest(minQtVersion) {
    maj = $$1
    min = $$2
    patch = $$3
    isEqual(QT_MAJOR_VERSION, $$maj) {
        isEqual(QT_MINOR_VERSION, $$min) {
            isEqual(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
            greaterThan(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
        }
        greaterThan(QT_MINOR_VERSION, $$min) {
            return(true)
        }
    }
    greaterThan(QT_MAJOR_VERSION, $$maj) {
        return(true)
    }
    return(false)
}

!minQtVersion(5, 2, 0) {
    message("Cannot build jom with Qt version $${QT_VERSION}.")
    error("Use at least Qt 5.2.0.")
}
