all: Footb$$@ LolCatExtractorManager.tar.gz manyDependents \
     manyDependentsSingleExecution manyDependentsSubstitutedNames \
     manyDependentsInlineFile \
     gen_init generated.txt gen_cleanup macros.mk $(MAKEDIR)\infrules.mk

Football:
	@echo $$@
	@echo $@ $@

LolCatExtractorManager.tar.gz:
	@echo $*

manyDependents: Timmy Jimmy Kenny Eric Kyle Stan
	@echo $**
	@echo $?

manyDependentsSingleExecution: Timmy Jimmy Kenny Eric Kyle Stan
	!@echo $**
	!@echo $?

manyDependentsSubstitutedNames: Tilly Jilly
    @echo $$(**) $(**)
    @echo $$(?) $(?)
    @echo $$(**:ll=mm) $(**:ll=mm)
    @echo $$(?:ll=mm) $(?:ll=mm)

manyDependentsInlineFile: Timmy Jimmy Kenny Eric Kyle Stan
    @type <<
$$@ $@
$$** $**
<<

Timmy:
Jimmy:
Kenny:
Eric:
Kyle:
Stan:
Tilly:
Jilly:

gen_init:
	@echo x > gen1.txt
	@echo x > generated.txt
	@echo x > gen2.txt
	@echo x > gen3.txt

generated.txt: gen1.txt gen2.txt gen3.txt
	@echo $?

gen_cleanup:
	@del generated.txt gen?.txt

macros.mk:
	@echo $(@D)
	@echo $(@B)
	@echo $(@F)
	@echo $(@R)

$(MAKEDIR)\infrules.mk: force
	@echo $(@D)
	@echo $(@B)
	@echo $(@F)
	@echo $(@R)

force:

root_dir\substitutionWithColon:
    @echo $(@:root_dir=C:\somewhere)
    @echo $(@R:root_dir=C:\somewhere)
