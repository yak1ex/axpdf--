#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_container.hpp>

#include <iostream>
#include <string>

namespace client
{
	namespace qi = boost::spirit::qi;
	template <typename Iterator>
	struct pdf_parser : qi::grammar<Iterator, std::string()>
	{
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
		pdf_parser() : pdf_parser::base_type(pdf, "pdf")
		{
			using qi::lit;
			using qi::char_;
			using qi::string;
			using qi::omit;
			using qi::eoi;
			using qi::eps;
			using qi::skip;
			using boost::phoenix::if_else;
			using boost::phoenix::assign;
			using boost::phoenix::bind;
			using boost::phoenix::push_back;
			using namespace qi::labels;

			unesc_char.add
				("\\n", '\n')("\\r", '\r')("\\t", '\t')("\\b", '\b')
				("\\f", '\f')("\\(", '(')("\\)", ')')("\\\\",'\\')
			;
			eol_char.add("\r\n",'\n')("\r",'\n')("\n",'\n');
			pdf %= literal_string | hex_string;
			literal_string %= lit('(') >> -literal_string_ >> lit(')');
			literal_string_ =
				char_('(')[push_back(_val,_1)] >> -literal_string_[append(_val,_1)] >> char_(')')[push_back(_val,_1)] >> -literal_string_[append(_val,_1)] |
				literal_string_char[push_back(_val,_1)] >> -literal_string_[append(_val,_1)];
			literal_string_char %= 
				skip(literal_string_skip.alias())[octal_char | unesc_char | lit('\\') >> char_ | eol_char | (char_ - char_("()"))];
			literal_string_skip = lit("\\\r\n") | lit("\\\r") | lit("\\\n");
			octal_char = lit('\\')[_val=0] >> octal_digit[_val=_1] >> -octal_digit[_val=_val*8+_1] >> -octal_digit[_val=_val*8+_1];
			octal_digit = char_('0','7')[_val=_1-'0'];
			hex_string = lit('<') >> *hex_char >> lit('>');
			hex_char = skip(hex_skip.alias())[hex_digit[_val=_1*16] >> -hex_digit[_val+=_1]];
			hex_digit = char_('0','9')[_val=_1-'0'] | char_('A','F')[_val=_1-'A'] | char_('a','f')[_val=_1-'a'];
			hex_skip = char_("\x20\x09\x0d\x0a\x0c");

			// Name setting
			pdf.name("pdf");
			literal_string.name("literal_string");
			literal_string_.name("literal_string_");
			literal_string_char.name("literal_string_char");
			literal_string_skip.name("literal_string_skip");
			octal_char.name("octal_char");
			octal_digit.name("octal_digit");
			hex_string.name("hex_string");
			hex_char.name("hex_char");
			hex_digit.name("hex_digit");
			hex_skip.name("hex_skip");
		}
		qi::symbols<char const, char const> unesc_char;
		qi::symbols<char const, char const> eol_char;
		qi::rule<Iterator,std::string()> pdf;
		qi::rule<Iterator,std::string()> literal_string;
		qi::rule<Iterator,std::string()> literal_string_;
		qi::rule<Iterator,char()> literal_string_char;
		qi::rule<Iterator> literal_string_skip;
		qi::rule<Iterator,char()> octal_char;
		qi::rule<Iterator,char()> octal_digit;
		qi::rule<Iterator,std::string()> hex_string;
		qi::rule<Iterator,char()> hex_char;
		qi::rule<Iterator,char()> hex_digit;
		qi::rule<Iterator,char()> hex_skip;
	};
	template <typename Iterator>
	bool parse_pdf(Iterator first, Iterator last, std::string &s)
	{
		bool r = parse(first, last, pdf_parser<Iterator>(), s);

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
		std::string str2;
		while (getline(std::cin, str2)) {
			if (str2.empty()) break;
			str += '\x0a' + str2;
		}
		if (str.empty() || str[0] == 'q' || str[0] == 'Q')
			break;

		std::string s;
		if (client::parse_pdf(str.begin(), str.end(), s))
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
