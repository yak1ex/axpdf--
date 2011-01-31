#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_container.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/variant.hpp>

#include <iostream>
#include <string>

#include "spirit_helper.hpp"

namespace client
{
	namespace qi = boost::spirit::qi;

	BOOST_SPIRIT_AUTO(qi, white_space, qi::char_("\x09\x0a\x0c\x0d\x20") | qi::char_('\0'));
	BOOST_SPIRIT_AUTO(qi, comment, qi::lit('%') >> *(qi::char_ - qi::lit("\r\n") - qi::lit('\r') - qi::lit('\n')) >> (qi::lit("\r\n") | qi::lit('\r') | qi::lit('\n')));
	BOOST_SPIRIT_AUTO(qi, skip_normal, comment | white_space);

	template <typename Iterator>
	struct pdf_parser : qi::grammar<Iterator, std::string(), skip_normal_expr_type>
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
		struct stringize_impl
		{
			template<typename A1>
			struct result { typedef std::string type; };
			template<typename T>
			std::string operator()(const T& t) const
			{
				return boost::lexical_cast<std::string>(t);
			}
		};
		boost::phoenix::function<stringize_impl> stringize;
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
			pdf %= object;
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
			hex_char = skip(white_space)[hex_digit[_val=_1*16] >> -hex_digit[_val+=_1]];
			hex_digit = char_('0','9')[_val=_1-'0'] | char_('A','F')[_val=_1-'A'] | char_('a','f')[_val=_1-'a'];
			regular_char = char_ - white_space - char_("\x28\x29\x3c\x3e\x5b\x5d\x7b\x7d\x2f\x25");
			name_obj = lit('/') >> *((regular_char - lit('#')) | hex_char_name);
			hex_char_name = lit('#') >> hex_digit[_val=_1*16] >> hex_digit[_val+=_1];
			array_obj = lit('[')[append(_val,"[")] >> *(object[append(_val,_1)] >> eps[append(_val," ")]) >> lit(']')[append(_val,"]")];
			object = (qi::bool_ | qi::real_parser<double, qi::strict_real_policies<double> >() | qi::int_ | literal_string | hex_string | name_obj | array_obj | dic_obj)[_val=stringize(_1)];
			dic_obj = lit("<<")[append(_val,"<<")] >> *(name_obj[append(_val,_1)] >> eps[append(_val," : ")] >> object[append(_val, _1)]) >> lit(">>")[append(_val,">>")];

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
			regular_char.name("regular_char");
			name_obj.name("name_obj");
			hex_char_name.name("hex_char_name");
			array_obj.name("array_obj");
			object.name("object");
			dic_obj.name("dic_obj");
		}
		qi::symbols<char const, char const> unesc_char;
		qi::symbols<char const, char const> eol_char;
		qi::rule<Iterator,std::string(), skip_normal_expr_type> pdf;
		qi::rule<Iterator,std::string()> literal_string;
		qi::rule<Iterator,std::string()> literal_string_;
		qi::rule<Iterator,char()> literal_string_char;
		qi::rule<Iterator> literal_string_skip;
		qi::rule<Iterator,char()> octal_char;
		qi::rule<Iterator,char()> octal_digit;
		qi::rule<Iterator,std::string()> hex_string;
		qi::rule<Iterator,char()> hex_char;
		qi::rule<Iterator,char()> hex_digit;
		qi::rule<Iterator,char()> regular_char;
		qi::rule<Iterator,std::string()> name_obj;
		qi::rule<Iterator,char()> hex_char_name;
		qi::rule<Iterator,std::string(), skip_normal_expr_type> array_obj;
		qi::rule<Iterator,std::string(), skip_normal_expr_type> object;
		qi::rule<Iterator,std::string(), skip_normal_expr_type> dic_obj;
	};
	template <typename Iterator>
	bool parse_pdf(Iterator first, Iterator last, std::string &s)
	{
		bool r = phrase_parse(first, last, pdf_parser<Iterator>(), skip_normal, s);

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
