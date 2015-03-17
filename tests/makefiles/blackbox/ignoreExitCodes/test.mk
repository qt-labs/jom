# test ignore exit codes in make files

test1: testNonexistentCommand testExitCode1 testExitCode2 testExitCode3
    @echo ---SUCCESS---

testNonexistentCommand:
    -nonexistent_command 2>NUL

testExitCode1:
    -cmd /k exit 1

testExitCode2:
    -1cmd /k exit 1

testExitCode3:
    ------1234cmd /k exit 1234

# target test2 is supposed to fail
test2:
    -6cmd /k exit 7
