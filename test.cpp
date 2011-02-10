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
	struct test_parser : qi::grammar<Iterator, std::string()>
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
		struct append_impl
		{
			template<typename A1, typename A2>
			struct result { typedef void type; };
			void operator()(std::string &s1, const std::string &s2) const
			{
				s1.append(s2);
			}
		};
		boost::phoenix::function<append_impl> append;
		struct nop_impl
		{
			typedef void result_type;
			void operator()() const {}
		};
		boost::phoenix::function<nop_impl> nop;
		test_parser() : test_parser::base_type(root, "test")
		{
			using namespace qi::labels;

			root = qi::lit("stream") >> yak::spirit::delimited(std::string("endstream"))[qi::char_];

			// Name setting
			root.name("root");
		}
		qi::rule<Iterator,std::string()> root;
	};
	template <typename Iterator>
	bool parse_test(Iterator first, Iterator last, std::string &s)
	{
		bool r = parse(first, last, test_parser<Iterator>(), s);

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
		std::string s;
		if (str.empty() || str[0] == 'q' || str[0] == 'Q')
			break;

		if (client::parse_test(str.begin(), str.end(), s))
		{
			std::cout << "-------------------------\n";
			std::cout << "Parsing succeeded\n";
			std::cout << s << '\n';
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
