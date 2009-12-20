all : one two three

one: a b c
    @echo one
two:; @echo two
three : "a;b" ;@echo three; @echo end of three

a :
b :
c :
"a;b":
    @echo X

