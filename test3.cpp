#include "parser.hpp"

#include <boost/foreach.hpp>
#include <set>

void traverse(std::set<yak::pdf::indirect_ref> &set, const yak::pdf::pdf_file_reader &pr, const yak::pdf::dictionary &dic)
{
	std::cout << dic << std::endl;
	if(yak::pdf::has_value(dic, yak::pdf::name("Type"), yak::pdf::name("Pages"))) {
		const yak::pdf::array & arr = pr.resolve<yak::pdf::array>(dic, yak::pdf::name("Kids"));
		BOOST_FOREACH(const yak::pdf::object &obj, arr) {
			traverse(set, pr, pr.resolve<yak::pdf::dictionary>(obj));
		}
	} else if(yak::pdf::has_value(dic, yak::pdf::name("Type"), yak::pdf::name("Page"))) {
		if(yak::pdf::has_key(dic, yak::pdf::name("Resources"))) {
			std::cout << pr.resolve(dic, yak::pdf::name("Resources")) << std::endl;
			const yak::pdf::dictionary &res = pr.resolve<yak::pdf::dictionary>(dic, yak::pdf::name("Resources"));
			if(yak::pdf::has_key(res, yak::pdf::name("XObject"))) {
				const yak::pdf::dictionary & xobj = pr.resolve<yak::pdf::dictionary>(res, yak::pdf::name("XObject"));
				BOOST_FOREACH(const yak::pdf::dictionary::value_type &v, xobj) {
					const yak::pdf::indirect_ref &ref = boost::get<yak::pdf::indirect_ref>(v.second);
					if(!set.count(ref)) {
						const yak::pdf::stream &s = pr.get<yak::pdf::stream>(ref);
						if(yak::pdf::has_value(s.dic, yak::pdf::name("Type"), yak::pdf::name("XObject")) && 
						   yak::pdf::has_value(s.dic, yak::pdf::name("Subtype"), yak::pdf::name("Image"))) {
							std::cout << pr.get(ref) << std::endl;
						}
						set.insert(ref);
					}
				}
			}
		}
	} else {
		throw yak::pdf::invalid_pdf("Unknown type in page tree");
	}
}

int main(int argc, char **argv)
{
	try {
		std::set<yak::pdf::indirect_ref> set;
		yak::pdf::pdf_file_reader pr(argv[1]);
		std::cout << pr.get_xref();
		std::cout << pr.get_trailer();
		if(yak::pdf::has_key(pr.get_trailer(), yak::pdf::name("Encrypt"))) {
			std::cout << pr.resolve(pr.get_trailer(), yak::pdf::name("Encrypt"));
		}
		std::cout << pr.get_root() << std::endl;
		traverse(set, pr, pr.resolve<yak::pdf::dictionary>(pr.get_root(), yak::pdf::name("Pages")));
	} catch(std::exception &e) {
		std::cerr << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Unknown exception thrown" << std::endl;
	}

	return 0;
}
