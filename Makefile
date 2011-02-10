CXXFLAGS = -Wall -O2 -I /var/tmp/boost_1_45_0

all: pdf.exe test.exe test2.exe

pdf.exe: pdf.cpp spirit_helper.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

test.exe: test.cpp spirit_helper.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

test2.exe: test2.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
