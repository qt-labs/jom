# A batch mode rule that consumes an intermediate target. $< must expand to the
# name the makefile declared, like it does for a regular inference rule.

.SUFFIXES: .b .d

.b.d:
	@echo $@ from $<
.d.e::
	@echo batch $<

.\a.d:
.\a.e:
