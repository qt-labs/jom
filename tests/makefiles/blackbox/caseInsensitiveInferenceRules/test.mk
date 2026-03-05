# QTCREATORBUG-34174: inference rule extensions must match case-insensitively.
# The .SUFFIXES uses lowercase .b, but the rule uses uppercase .C for the target extension.
# The target result.c (lowercase) must still match the .b.C rule.
.SUFFIXES:
.SUFFIXES: .b

.b.C:
	@echo from=$< to=$@

all: result.c

result.c: result.b
