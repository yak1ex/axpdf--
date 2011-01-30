#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_container.hpp>

#include <iostream>
#include <string>

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
			using qi::char_;
			using qi::eps;
			using boost::phoenix::push_back;
			using boost::phoenix::assign;
			using boost::phoenix::insert;
			using boost::phoenix::begin;
			using boost::phoenix::end;
			using namespace qi::labels;

			root %= string_rule;
			string_rule = char2string_rule[append(_val,_1)] >> string_rule[append(_val,_1)] | char2string_rule;
			char2string_rule = qi::skip(skip_rule.alias())[char_rule[push_back(_val,_1)] >> -char_rule[push_back(_val,_1)]];
			char_rule = char_("a-zA-Z0-9");
			skip_rule = char_("\x20\x09\x0d\x0a\x0c");

			// Name setting
			root.name("root");
			string_rule.name("string_rule");
			char2string_rule.name("char2string_rule");
			skip_rule.name("skip_rule");
		}
		qi::rule<Iterator,std::string()> root;
		qi::rule<Iterator,std::string()> string_rule;
		qi::rule<Iterator,std::string()> char2string_rule;
		qi::rule<Iterator,char()> char_rule;
		qi::rule<Iterator,char()> skip_rule;
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
