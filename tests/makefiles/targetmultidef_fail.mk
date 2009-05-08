all: bar

bar: foo1.cpp
	echo bla
bar: foo3.cpp
bar: foo4.cpp
	echo blubb

