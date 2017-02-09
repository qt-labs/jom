isEmpty(PROJECT_BUILD_ROOT):error(PROJECT_BUILD_ROOT must be set)

win32-g++ {
    JOMLIB_PREFIX = lib
    JOMLIB_SUFFIX = a
} else {
    JOMLIB_PREFIX =
    JOMLIB_SUFFIX = lib
}

build_pass:CONFIG(debug, debug|release) {
    JOMLIB = $$PROJECT_BUILD_ROOT/lib/$${JOMLIB_PREFIX}jomlibd.$$JOMLIB_SUFFIX
}
build_pass:CONFIG(release, debug|release) {
    JOMLIB = $$PROJECT_BUILD_ROOT/lib/$${JOMLIB_PREFIX}jomlib.$$JOMLIB_SUFFIX
}

LIBS += $$JOMLIB
POST_TARGETDEPS += $$JOMLIB
unset(JOMLIB)
