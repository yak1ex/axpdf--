########################################################################
#
# Makefile: Makefile for axpdf--.spi for gcc
#
#     Copyright (C) 2011 Yak! / Yasutaka ATARASHI
#
#     This software is distributed under the terms of a zlib/libpng
#     License.
#
#     $Id$
#
########################################################################

VER=0_03

#CXX = i686-w64-mingw32-g++
CXXFLAGS = -Wall -Wno-unused-local-typedefs -O2 -I /usr/local/include
LIBS = -L/usr/local/lib -lboost_iostreams-mt
ifdef MTR
MEMTRACE = memtrace.o
endif

all: axpdf--.spi pdf.exe pdfshell.exe test.exe test2.exe

axpdf--.o: axpdf--.cpp spirit_helper.hpp parser.hpp

odstream.o: odstream.cpp odstream.hpp

axpdf--.spi: axpdf--.o parser.o reader.o decoder.o types_output.o bmp_helper.o odstream.o axpdf--.def

pdf.o: pdf.cpp spirit_helper.hpp parser.hpp

pdf.exe: pdf.o decoder.o types_output.o $(MEMTRACE)
	$(LINK.cc) $^ -o $@ $(LIBS)

test.exe: test.cpp spirit_helper.hpp
	$(LINK.cc) $< -o $@

test2.exe: test2.cpp
	$(LINK.cc) $< -o $@

test3.exe: test3.cpp
	$(LINK.cc) $< -o $@

pdfshell.exe: pdfshell.o reader.o decoder.o types_output.o $(MEMTRACE)
	$(LINK.cc) $^ -o $@ $(LIBS)

%.spi: %.o
	$(LINK.cc) -mwindows -shared $^ -o $@ $(LIBS)

%.exe: %.o
	$(LINK.cc) $^ -o $@

dist:
	-rm -rf source source.zip axpdf--$(VER).zip disttemp
	mkdir source
	git archive --worktree-attributes master | tar xf - -C source
	(cd source; zip ../source.zip *)
	-rm -rf source
	mkdir disttemp
	cp axpdf--.spi axpdf--.txt source.zip disttemp
	(cd disttemp; zip ../axpdf--$(VER).zip *)
	-rm -rf disttemp

tag:
	git tag axpdf--$(VER)
