# Chained rules for targets that spell out the current directory.

.SUFFIXES: .b .d

.b.d:
	@echo $@
.d.e:
	@echo $@

.\a.d:
.\a.e:
