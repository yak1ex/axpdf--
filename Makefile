all: pdf.exe test.exe test2.exe

pdf.exe: pdf.cpp
	$(CXX) -Wall -O2 -I /usr/local/include/boost-1.43 -o $@ $^

test.exe: test.cpp
	$(CXX) -Wall -O2 -I /usr/local/include/boost-1.43 -o $@ $^

test2.exe: test2.cpp
	$(CXX) -Wall -O2 -I /usr/local/include/boost-1.43 -o $@ $^
