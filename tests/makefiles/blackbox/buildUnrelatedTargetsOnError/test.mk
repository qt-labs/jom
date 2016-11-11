# test the /k option
# When running "jom /f test.mk /k" the target "dependsOnFailingTarget" must not be built.

first: workingTarget dependsOnFailingTarget

failingTarget:
    cmd /c exit 7

dependsOnFailingTarget: failingTarget
    @echo We should not see this.

workingTarget:
    @echo Yay! This always works!
