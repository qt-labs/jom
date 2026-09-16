# nmake does not chain through an intermediate that isn't a target of the
# makefile. Nothing must be built here, because a.d is not a target.

.SUFFIXES: .b .d

.b.d:
	@echo $@ from $<
.d.e:
	@echo $@ from $<

a.e:
