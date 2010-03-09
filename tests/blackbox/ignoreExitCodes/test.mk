# test ignore exit codes in make files
#
# http://msdn.microsoft.com/en-us/library/1whxt45w.aspx

all:
	@echo Please specify a target.

tests: test_ignoreExitCode

init:

post_check:

test_ignoreExitCode:
	-nonexistent_command
	@echo Failing command was properly ignored

