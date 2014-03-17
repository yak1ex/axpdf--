#include <iostream>
#include <fstream>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>

#include "reader.hpp"
#include "types_output.hpp"
#include "decoder.hpp"

int main(int argc, char **argv)
{
	if(argc < 2) {
		std::cout << "pdfshell <pdf_filename>" << std::endl;
		return 1;
	}
	yak::pdf::pdf_file_reader pfr(argv[1]);
	std::string s;
	std::cout << "> " << std::flush;
	while(getline(std::cin, s))
	{
		try {
			std::vector<std::string> token;
			boost::algorithm::split(token, s, boost::algorithm::is_space(), boost::algorithm::token_compress_on );
			if(token.size() > 0) {
				if(token[0] == "end") break;
				if(token[0] == "show") {
					if(token.size() < 2) throw std::runtime_error("show <number>: object number is required");
					std::cout << pfr.get(boost::lexical_cast<int>(token[1]));
				} else if(token[0] == "root") {
					std::cout << pfr.get_root();
				} else if(token[0] == "xref") {
					std::cout << pfr.get_xref();
				} else if(token[0] == "trailer") {
					std::cout << pfr.get_trailer();
				} else if(token[0] == "raw") {
					if(token.size() < 3) throw std::runtime_error("raw <number> <output_filename>: object number and output filename are required");
					const yak::pdf::stream &s = pfr.get<yak::pdf::stream>(boost::lexical_cast<int>(token[1]));
					std::ofstream ofs(token[2].c_str(), std::ios::out | std::ios::binary);
					ofs.write(static_cast<const char*>(static_cast<const void*>(&s.data[0])), s.data.size());
				} else if(token[0] == "decode") {
					if(token.size() < 3) throw std::runtime_error("decode <number> <output_filename>: object number and output filename are required");
					const yak::pdf::stream &s = pfr.get<yak::pdf::stream>(boost::lexical_cast<int>(token[1]));
					std::string d;
					yak::pdf::decoder::get_decoded_result(s, d);
					std::ofstream ofs(token[2].c_str(), std::ios::out | std::ios::binary);
					ofs.write(static_cast<const char*>(static_cast<const void*>(&d[0])), d.size());
				} else if(token[0] == "help") {
					std::cout << "end\nshow <number>\nroot\nxref\ntrailer\nraw <number> <outfile>\ndecode <number> <outfile>\nhelp" << std::endl;
				}
			}
		} catch(std::exception &e) {
			std::cout << e.what() << std::endl;
		}
		std::cout << "> " << std::flush;
	}
	return 0;
}
