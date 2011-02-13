#include "parser.hpp"

void rpl()
{
	std::string str;
	while (getline(std::cin, str))
	{
		std::string str2;
		while (getline(std::cin, str2)) {
			if (str2.empty()) break;
			str += '\x0a' + str2;
		}
		if (str.empty() || str[0] == 'q' || str[0] == 'Q')
			break;

		yak::pdf::pdf_data pd;
		if (parse_pdf(str.begin(), str.end(), pd))
		{
			std::cout << "-------------------------\n";
			std::cout << "Parsing succeeded\n";
			std::cout << pd << '\n';
			std::cout << "\n-------------------------\n";
		}
		else
		{
			std::cout << "-------------------------\n";
			std::cout << "Parsing failed\n";
			std::cout << "-------------------------\n";
		}
	}

	std::cout << "Bye... :-) \n\n";
}

int main(int argc, char **argv)
{
	if(argc > 1) {
		yak::pdf::pdf_data pd;
		if(parse_pdf_file(argv[1], pd)) {
			std::cout << "-------------------------\n";
			std::cout << "Parsing succeeded\n";
			std::cout << pd << '\n';
			std::cout << "\n-------------------------\n";
		} else {
			std::cout << "-------------------------\n";
			std::cout << "Parsing failed\n";
			std::cout << "-------------------------\n";
		}
	} else {
		rpl();
	}
	return 0;
}

