# Test for QTCREATORBUG-34175.
# An inference rule must also be applied when the dependent it infers isn't an
# existing file but a target that another inference rule builds.

.SUFFIXES: .b .d

.b.d:
	@echo building $@ from $<

.d.e:
	@echo building $@ from $<

a.d:
a.e:
