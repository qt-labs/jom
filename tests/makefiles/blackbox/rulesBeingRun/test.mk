# Test that inference rules are applied to dependents
# even when the parent target has explicit commands.
# This tests the fix for QTCREATORBUG-33978.

# Inference rule for .cpp -> .obj
.cpp.obj:
	echo Building $@ from $<

# Target with explicit commands that depends on foo.obj
# Before the fix, foo.obj wouldn't get its inference rule applied
# because "all" already had commands and preselectInferenceRules
# would return early without processing dependents.
all: foo.obj
	echo All target executed

# This file exists in the test directory
# foo.cpp exists and foo.obj should be built from it using the inference rule
