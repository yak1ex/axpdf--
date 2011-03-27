#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_container.hpp>

#include <boost/typeof/typeof.hpp>

#include <iostream>
#include <string>

#include "spirit_helper.hpp"

namespace client
{
	namespace qi = boost::spirit::qi;

	template <typename Iterator>
	struct test_parser : qi::grammar<Iterator, std::vector<int>(), qi::space_type>
	{
		struct output_impl
		{
			template<typename A1>
			struct result { typedef char type; };
			char operator()(char c) const
			{
				std::cout << (int)c << std::endl;
				return c;
			}
		};
		boost::phoenix::function<output_impl> output;
		struct nop_impl
		{
			typedef void result_type;
			void operator()() const {}
		};
		boost::phoenix::function<nop_impl> nop;
		test_parser() : test_parser::base_type(root, "test")
		{
			using namespace qi::labels;
			std::vector<int> v;
			v.push_back(0);
			v.push_back(0);
			v.push_back(0);

			root = *int_list[yak::spirit::append(_val,_1)];
			int_list = *qi::int_ >> qi::omit[qi::char_];

			// Name setting
			root.name("root");
			int_list.name("int_list");
		}
		qi::rule<Iterator,std::vector<int>(),qi::space_type> root;
		qi::rule<Iterator,std::vector<int>(),qi::space_type> int_list;
	};
	template <typename Iterator>
	bool parse_test(Iterator first, Iterator last, std::vector<int> &s)
	{
		bool r = phrase_parse(first, last, test_parser<Iterator>(), qi::space, s);

		if (!r || first != last) // fail if we did not get a full match
			return false;
		return r;
	}
}

int main()
{
	std::string str;
	while (getline(std::cin, str))
	{
		std::vector<int> v;
		if (str.empty() || str[0] == 'q' || str[0] == 'Q')
			break;

		if (client::parse_test(str.begin(), str.end(), v))
		{
			std::cout << "-------------------------\n";
			std::cout << "Parsing succeeded\n";
			for(std::size_t i = 0; i < v.size(); ++i) {
				std::cout << v[i] << ' ';
			}
			std::cout << std::endl;
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
	return 0;
}
