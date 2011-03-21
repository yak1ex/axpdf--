#include <fstream>
#include <iostream>
#include <iomanip>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include "decoder.hpp"

//<<
//  /DecodeParms<<
//    /Columns 6/Predictor 12
//  >>
//  /Filter/FlateDecode
//  /ID[<0DF2E1C43D8D81A4D33AD6DA0A6FC30D><D2D2783A6BE93F40BF1765CEF17DB48A>]
//  /Info 260 0 R
//  /Length 812
//  /Root 262 0 R
//  /Size 316
//  /Type/XRef
//  /W[1 4 1]
//>>

int main(void)
{
	namespace io = boost::iostreams;

	{
		io::filtering_istream in;
		io::zlib_decompressor zd;
		in.push(zd);
		io::file_source fs("stream.dat", std::ios_base::in | std::ios_base::binary);
		in.push(fs);
		for(int i=0;i<30;++i) {
			std::cout << std::hex << std::setw(2) << static_cast<int>(in.get()) << ' ';
			if(i%16 == 15) 
				std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	{
		io::filtering_istream in;
		yak::pdf::predicator_png pp(6);
		in.push(pp);
		io::zlib_decompressor zd;
		in.push(zd);
		io::file_source fs("stream.dat", std::ios_base::in | std::ios_base::binary);
		in.push(fs);
		for(int i=0;i<30;++i) {
			std::cout << std::hex << std::setw(2) << static_cast<int>(in.get()) << ' ';
			if(i%16 == 15) 
				std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	return 0;
}
