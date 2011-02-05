#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_container.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "spirit_helper.hpp"

BOOST_FUSION_DEFINE_STRUCT(
	(client),
	indirect_ref,
	(int, number)
	(int, generation)
)

namespace client
{
	struct name
	{
		std::string value;
		name() {}
		name(const std::string &s) : value(s) {}
		name& operator=(const std::vector<char> &v) {
			value.assign(v.begin(), v.end());
			return *this;
		}
		operator std::string() const { return value; }
	};
	bool operator<(const name& n1, const name &n2)
	{
		return n1.value < n2.value;
	}
	struct null {};
	struct stream;
	typedef boost::make_recursive_variant<
		bool, int, double, std::string, std::vector<char>, name,
		std::map<name, boost::recursive_variant_>, // dictionary
		std::vector<boost::recursive_variant_>, // array
		indirect_ref, null, stream
	>::type object;
	typedef std::map<name, object> dictionary;
	typedef std::vector<object> array;
}

BOOST_FUSION_DEFINE_STRUCT(
	(client),
	stream,
	(client::dictionary, dic)
	(std::vector<char>, data)
)
BOOST_FUSION_DEFINE_STRUCT(
	(client),
	indirect_obj,
	(int, number)
	(int, generation)
	(client::object, value)
)
BOOST_FUSION_DEFINE_STRUCT(
	(client),
	pdf_data,
	(int, major_ver)
	(int, minor_ver)
	(std::vector<client::indirect_obj>, objects)
	(client::dictionary, trailer_dic)
)

namespace client
{
	const int indent = 2;
	struct output_visitor : public boost::static_visitor<>
	{
		std::ostream &os;
		int level;
		void make_indent()
		{
			for(int i = 0 ; i < level*indent; ++i) { os << ' '; }
		}
		void output_nibble(int n)
		{
			os << "0123456789ABCDEF"[n&0xF];
		}
		output_visitor(std::ostream &os, int level = 0) : os(os), level(level) {}
		template<typename T>
		void operator()(const T& t)
		{
			make_indent(); os << t << std::endl;
		}
		void operator()(const std::string &s)
		{
			make_indent(); os << '(' << s << ')' << std::endl;
		}
		void operator()(const std::vector<char>& v)
		{
			make_indent(); os << '<'; 
			for(std::size_t i = 0; i < v.size(); ++i) {
				output_nibble(v[i]); output_nibble(v[i]>>4);
			}
			os << '>' << std::endl;
		}
		void operator()(const name &n)
		{
			make_indent(); os << '/' << n.value << std::endl;
		}
		void operator()(const dictionary& m)
		{
			make_indent(); os << "<<" << std::endl; ++level; 
			for(dictionary::const_iterator mi = m.begin(); mi != m.end(); ++mi)
			{
				make_indent(); os << '/' << mi->first.value << std::endl;
				++level; boost::apply_visitor((*this), mi->second); --level;
			}
			--level; make_indent(); os << ">>" << std::endl;
		}
		void operator()(const array& v)
		{
			make_indent(); os << '[' << std::endl; ++level; 
			for(std::size_t i = 0; i < v.size(); ++i) {
				boost::apply_visitor((*this), v[i]);
			}
			--level; make_indent(); os << ']' << std::endl;
		}
		void operator()(const indirect_ref& r)
		{
			make_indent(); os << r.number << ' ' << r.generation << " R" << std::endl;
		}
		void operator()(const null& n)
		{
			make_indent(); os << "null" << std::endl;
		}
		void operator()(const stream& s)
		{
			(*this)(s.dic);
			os << "stream" << std::endl;
			os << "endstream" << std::endl;
		}
	};
	void out(std::ostream &os, const object& obj, int level = 0)
	{
		output_visitor visitor(os, level);
		boost::apply_visitor(visitor, obj);
	}
	std::ostream& operator<<(std::ostream &os, const object& obj)
	{
		out(os, obj); return os;
	}
	std::ostream& operator<<(std::ostream &os, const std::vector<indirect_obj> &objs)
	{
		for(std::size_t i = 0; i < objs.size(); ++i) {
			os << objs[i].number << ' ' << objs[i].generation << " obj" << std::endl;
			out(os, objs[i].value, 1);
			os << "endobj" << std::endl;
		}
		return os;
	}
	std::ostream& operator<<(std::ostream &os, const pdf_data &pd)
	{
		os << "%PDF-" << pd.major_ver << '.' << pd.minor_ver << std::endl;
		os << pd.objects;
		os << "trailer" << std::endl;
		output_visitor(os, 0)(pd.trailer_dic);
		return os;
	}

	namespace qi = boost::spirit::qi;

	BOOST_SPIRIT_AUTO(qi, white_space, qi::char_("\x09\x0a\x0c\x0d\x20") | qi::char_('\0'));
	BOOST_SPIRIT_AUTO(qi, comment, qi::lit('%') >> *(qi::char_ - qi::lit("\r\n") - qi::lit('\r') - qi::lit('\n')) >> (qi::lit("\r\n") | qi::lit('\r') | qi::lit('\n')));
	BOOST_SPIRIT_AUTO(qi, skip_normal, comment | white_space);

	template <typename Iterator>
	struct pdf_parser : qi::grammar<Iterator, pdf_data(), skip_normal_expr_type>
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
		struct get_length_impl
		{
			template<typename A1>
			struct result { typedef int type; };
			int operator()(const dictionary& dic) const
			{
				return boost::get<int>(dic.find(name("Length"))->second);
			}
		};
		boost::phoenix::function<get_length_impl> get_length;
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
			using qi::int_;
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
			pdf = qi::no_skip[lit("%PDF-") >> int_ >> lit('.') >> int_ >> *qi::skip[indirect_obj] >> -qi::skip[xref_section] >> qi::skip[-trailer_dic >> trailer]];
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
			name_obj = lit('/') >> (*((regular_char - lit('#')) | hex_char_name))[_val = _1];
			hex_char_name = lit('#') >> hex_digit[_val=_1*16] >> hex_digit[_val+=_1];
			array_obj = lit('[') >> *object >> lit(']');
			object = indirect_ref | qi::bool_ | qi::real_parser<double, qi::strict_real_policies<double> >() | qi::int_ | literal_string | hex_string | name_obj | array_obj | stream | dic_obj | null_obj;
			dic_obj = lit("<<") >> *(name_obj >> object) >> lit(">>");
			indirect_obj = int_ >> int_ >> lit("obj") >> object >> lit("endobj");
			indirect_ref = int_ >> int_ >> lit('R');
			null_obj = lit("null")[_val=null()];
			stream %= dic_obj[_a=_1] >> lit("stream") >> qi::no_skip[-lit('\r') >> lit('\n')] >> stream_data(_a) >> lit("endstream");
			stream_data = qi::no_skip[repeat(get_length(_r1))[qi::byte_]];
			xref_section = lit("xref") >> *xref_subsection;
			xref_subsection = int_ >> int_[_a = _1] >> repeat(_a)[xref_entry];
			xref_entry = int_ >> int_ >> char_;
			trailer_dic = lit("trailer") >> dic_obj;
			trailer = lit("startxref") >> int_ >> qi::skip(white_space)[lit("%%EOF")];

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
			indirect_obj.name("indirect_obj");
			indirect_ref.name("indirect_ref");
			null_obj.name("null_obj");
			stream.name("stream");
			stream_data.name("stream_data");
			xref_section.name("xref_section");
			xref_subsection.name("xref_subsection");
			xref_entry.name("xref_entry");
			trailer_dic.name("trailer_dic");
			trailer.name("trailer");
		}
		qi::symbols<char const, char const> unesc_char;
		qi::symbols<char const, char const> eol_char;
		qi::rule<Iterator,pdf_data(), skip_normal_expr_type> pdf;
		qi::rule<Iterator,std::string()> literal_string;
		qi::rule<Iterator,std::string()> literal_string_;
		qi::rule<Iterator,char()> literal_string_char;
		qi::rule<Iterator> literal_string_skip;
		qi::rule<Iterator,char()> octal_char;
		qi::rule<Iterator,char()> octal_digit;
		qi::rule<Iterator,std::vector<char>()> hex_string;
		qi::rule<Iterator,char()> hex_char;
		qi::rule<Iterator,char()> hex_digit;
		qi::rule<Iterator,char()> regular_char;
		qi::rule<Iterator,name()> name_obj;
		qi::rule<Iterator,char()> hex_char_name;
		qi::rule<Iterator,array(), skip_normal_expr_type> array_obj;
		qi::rule<Iterator,client::object(), skip_normal_expr_type> object;
		qi::rule<Iterator,dictionary(), skip_normal_expr_type> dic_obj;
		qi::rule<Iterator,client::indirect_obj(), skip_normal_expr_type> indirect_obj;
		qi::rule<Iterator,client::indirect_ref(), skip_normal_expr_type> indirect_ref;
		qi::rule<Iterator,client::null(), skip_normal_expr_type> null_obj;
		qi::rule<Iterator,client::stream(), skip_normal_expr_type, qi::locals<dictionary> > stream;
		qi::rule<Iterator,std::vector<char>(const dictionary&)> stream_data;
		qi::rule<Iterator, void(), skip_normal_expr_type> xref_section;
		qi::rule<Iterator, skip_normal_expr_type, qi::locals<int> > xref_subsection;
		qi::rule<Iterator, skip_normal_expr_type> xref_entry;
		qi::rule<Iterator, client::dictionary(), skip_normal_expr_type> trailer_dic;
		qi::rule<Iterator, skip_normal_expr_type> trailer;
	};
	template <typename Iterator>
	bool parse_pdf(Iterator first, Iterator last, pdf_data &pd)
	{
		bool r = phrase_parse(first, last, pdf_parser<Iterator>(), skip_normal, pd);

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

		client::pdf_data pd;
		if (client::parse_pdf(str.begin(), str.end(), pd))
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
	return 0;
}
