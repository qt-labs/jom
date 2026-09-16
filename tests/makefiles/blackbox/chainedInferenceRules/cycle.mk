# Mutually recursive inference rules over declared targets. Like nmake, jom must
# report a cycle instead of silently building nothing.

.SUFFIXES: .d .e

.d.e:
	@echo $@ from $<
.e.d:
	@echo $@ from $<

cyc.d:
cyc.e:
