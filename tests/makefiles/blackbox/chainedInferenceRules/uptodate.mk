# A rule that loses the priority contest infers the declared target ud.d, which is
# never built. That must not keep ud.e permanently out of date.

.SUFFIXES: .c .d

.c.e:
	@echo $@ from $<
	@copy $< $@ > NUL
.d.e:
	@echo $@ from $<
	@copy $< $@ > NUL

ud.d:
ud.e:

clean:
	@del ud.e > NUL 2>&1
