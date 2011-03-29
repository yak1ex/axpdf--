#include "parser.hpp"

int main(int argc, char **argv)
{
	try {
		yak::pdf::pdf_file_reader pr(argv[1]);
		std::cout << pr.get_root() << std::endl;
	} catch(std::exception &e) {
		std::cerr << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Unknown exception thrown" << std::endl;
	}

	return 0;
}
