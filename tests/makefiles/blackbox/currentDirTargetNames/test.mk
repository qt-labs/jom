# Like nmake, ".\foo" and "foo" denote the same target, no matter whether the name
# shows up as target, as dependent or on the command line.

all: a.d .\b.d
	@echo ALL

.\a.d:
	@echo BUILT $@

b.d:
	@echo BUILT $@
