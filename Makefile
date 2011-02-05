CXXFLAGS = -Wall -O2 -I /var/tmp/boost_1_45_0

all: pdf.exe test.exe test2.exe

pdf.exe: pdf.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test.exe: test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test2.exe: test2.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^
