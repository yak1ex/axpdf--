#include "parser.hpp"
#include <boost/foreach.hpp>

void traverse(const yak::pdf::pdf_file_reader &pr, const yak::pdf::dictionary &dic)
{
	std::cout << dic << std::endl;
	if(yak::pdf::has_value(dic, yak::pdf::name("Type"), yak::pdf::name("Pages"))) {
		const yak::pdf::array & arr = pr.resolve<yak::pdf::array>(dic, yak::pdf::name("Kids"));
		BOOST_FOREACH(const yak::pdf::object &obj, arr) {
			traverse(pr, pr.resolve<yak::pdf::dictionary>(obj));
		}
	} else if(yak::pdf::has_value(dic, yak::pdf::name("Type"), yak::pdf::name("Page"))) {
		if(yak::pdf::has_key(dic, yak::pdf::name("Resources"))) {
			std::cout << pr.resolve(dic, yak::pdf::name("Resources")) << std::endl;
		}
	} else {
		throw yak::pdf::invalid_pdf("Unknown type in page tree");
	}
}

int main(int argc, char **argv)
{
	try {
		yak::pdf::pdf_file_reader pr(argv[1]);
		std::cout << pr.get_root() << std::endl;
		traverse(pr, pr.resolve<yak::pdf::dictionary>(pr.get_root(), yak::pdf::name("Pages")));
	} catch(std::exception &e) {
		std::cerr << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Unknown exception thrown" << std::endl;
	}

	return 0;
}
