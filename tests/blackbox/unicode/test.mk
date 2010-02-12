# test unicode files

all:
	@echo Please specify a target.

tests: test_utf8 test_utf16

init:
post_check:

test_utf8:
    $(MAKE) /$(MAKEFLAGS) /f test_utf8.mk

test_utf16:
    $(MAKE) /$(MAKEFLAGS) /f test_utf16.mk
