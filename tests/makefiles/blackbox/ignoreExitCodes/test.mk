# test ignore exit codes in make files

test1: testNonexistentCommand testExitCode1 testExitCode2
    @echo ---SUCCESS---

testNonexistentCommand:
    -nonexistent_command 2>NUL

testExitCode1:
    -cmd /k exit 1

testExitCode2:
    -1cmd /k exit 1

# target test2 is supposed to fail
test2:
    -6cmd /k exit 7
