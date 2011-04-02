########################################################################
#
# Makefile: Makefile for axpdf--.spi for gcc, currently not working
#
#     Copyright (C) 2011 Yak! / Yasutaka ATARASHI
#
#     This software is distributed under the terms of a zlib/libpng
#     License.
#
#     $Id$
#
########################################################################

#CXX = i686-w64-mingw32-g++
#CXXFLAGS = -Wall -O2 -I /var/tmp/boost_1_45_0

CXX = g++-3
CXXFLAGS = -mno-cygwin -Wall -O2 -I /var/tmp/boost_1_45_0

#CXXFLAGS = -Wall -O2 -I /var/tmp/boost_1_45_0

all: axpdf--.spi pdf.exe test.exe test2.exe

axpdf--.o: axpdf--.cpp spirit_helper.hpp parser.hpp

odstream.o: odstream.cpp odstream.hpp

axpdf--.spi: axpdf--.o odstream.o axpdf--.def

pdf.exe: pdf.cpp spirit_helper.hpp parser.hpp
	$(LINK.cc) $< -o $@

test.exe: test.cpp spirit_helper.hpp
	$(LINK.cc) $< -o $@

test2.exe: test2.cpp
	$(LINK.cc) $< -o $@

test3.exe: test3.cpp
	$(LINK.cc) $< -o $@

%.spi: %.o
	$(LINK.cc) -mwindows -shared $^ -o $@

%.exe: %.o
	$(LINK.cc) $^ -o $@
