# Three inference rules in a row.

.SUFFIXES: .b .c .d

.b.c:
	@echo $@ from $<
.c.d:
	@echo $@ from $<
.d.e:
	@echo $@ from $<

a.c:
a.d:
a.e:
