# A cycle made of an inferred and an explicit dependent. The parser's cycle check
# only follows explicit dependents, so it can't see this one.

.SUFFIXES: .d .e

.d.e:
	@echo $@ from $<

mix.d: mix.e
mix.e:
