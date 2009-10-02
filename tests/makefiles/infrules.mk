DEPNAME=($<)

.cpp.obj:
	@echo .cpp.obj $(DEPNAME)

{subdir}.cpp.obj:
	@echo {subdir}.cpp.obj ($<)

all: init\
     foo1.obj foo2.obj foo3.obj foo4.obj\
     foo5.obj foo6.obj\
     cleanup

init:
	@echo I am temporary. Delete me. > foo6.cpp
	@echo I am temporary. Delete me. > foo5.cpp
	@echo I am temporary. Delete me. > subdir\foo5.cpp

cleanup:
	@del foo6.cpp
	@del foo5.cpp
	@del subdir\foo5.cpp

foo4.obj: foo4.cpp

foo5.obj: foo5.cpp

foo6.obj: foo6.cpp

