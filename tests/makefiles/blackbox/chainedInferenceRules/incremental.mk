# Chained rules that really produce files, for the out-of-date check.
# Uses its own source file, so that the makefiles above never see a built chain.

.SUFFIXES: .b .d

.b.d:
	@echo $@
	@copy $< $@ > NUL

.d.e:
	@echo $@
	@copy $< $@ > NUL

inc.d:
inc.e:

clean:
	@del inc.d inc.e > NUL 2>&1
