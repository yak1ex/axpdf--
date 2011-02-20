#ifndef YAK_PDF_PDFPARSER_HPP
#define YAK_PDF_PDFPARSER_HPP

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_match.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/home/qi/nonterminal/debug_handler.hpp>

#include <boost/variant/recursive_variant.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "zlib.h"

#include "spirit_helper.hpp"

namespace yak { namespace util {

	template<typename InputIterator1, typename InputIterator2>
	std::pair<InputIterator1, InputIterator2>
	safe_mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
	{
		while(first1 != last1 && first2 != last2 && *first1 == *first2) {
			++first1; ++first2;
		}
		return std::make_pair(first1, first2);
	}
	template<typename InputIterator1, typename InputIterator2, typename BinaryPredicate>
	std::pair<InputIterator1, InputIterator2>
	safe_mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, BinaryPredicate pred)
	{
		while(first1 != last1 && first2 != last2 && pred(*first1, *first2)) {
			++first1; ++first2;
		}
		return std::make_pair(first1, first2);
	}
	template<typename InputIterator1, typename InputIterator2>
	bool safe_equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
	{
		while(first1 != last1 && first2 != last2 && *first1 == *first2) {
			++first1; ++first2;
		}
		return first1 == last1 && first2 == last2;
	}
	template<typename InputIterator1, typename InputIterator2, typename BinaryPredicate>
	bool safe_equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, BinaryPredicate pred)
	{
		while(first1 != last1 && first2 != last2 && pred(*first1, *first2)) {
			++first1; ++first2;
		}
		return first1 == last1 && first2 == last2;
	}

}}

namespace boost {
	struct recursive_variant_ {};
}

BOOST_FUSION_DEFINE_STRUCT(
	(yak)(pdf),
	indirect_ref,
	(int, number)
	(int, generation)
)

namespace yak { namespace pdf {

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
	bool operator==(const name& n1, const name &n2)
	{
		return n1.value == n2.value;
	}
	bool operator<(const name& n1, const name &n2)
	{
		return n1.value < n2.value;
	}
	std::ostream& operator<<(std::ostream& os, const name &n)
	{
		os << '/' << n.value; return os;
	}
	struct null {};
	std::ostream& operator<<(std::ostream& os, const null &n)
	{
		os << "null"; return os;
	}
	struct stream;
	typedef boost::make_recursive_variant<
		bool, int, double, std::string, std::vector<char>, name,
		std::map<name, boost::recursive_variant_>, // dictionary
		std::vector<boost::recursive_variant_>, // array
		indirect_ref, null, stream
	>::type object;
	typedef std::map<name, object> dictionary;
}}

BOOST_FUSION_DEFINE_STRUCT(
	(yak)(pdf),
	stream,
	(yak::pdf::dictionary, dic)
	(std::vector<char>, data)
)
BOOST_FUSION_DEFINE_STRUCT(
	(yak)(pdf),
	indirect_obj,
	(int, number)
	(int, generation)
	(yak::pdf::object, value)
)
BOOST_FUSION_DEFINE_STRUCT(
	(yak)(pdf),
	pdf_data,
	(int, major_ver)
	(int, minor_ver)
	(std::vector<yak::pdf::indirect_obj>, objects)
	(yak::pdf::dictionary, trailer_dic)
)

namespace yak { namespace pdf {

	template<typename T>
	bool is_type(const object &obj)
	{
		return boost::get<T>(&obj) != 0;
	}
	template<typename T>
	bool has_value(const dictionary &dic, const name &n, const T& t)
	{
		return dic.count(n) &&
			boost::get<T>(&dic.find(n)->second) &&
			boost::get<T>(dic.find(n)->second) == t;
	}
	typedef std::vector<object> array;

}}

namespace yak { namespace pdf {

	struct output_visitor : public boost::static_visitor<>
	{
		static const int indent = 2;
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
			if(has_value(s.dic, name("Filter"), name("FlateDecode"))) {
				uLongf size = s.data.size();
				std::string str;
				int res;
				do {
					size *= 2;
					str.resize(size);
					res = uncompress((Bytef*)&str[0], &size, (Bytef*)&s.data[0], s.data.size());
				} while(res == Z_BUF_ERROR);
				str.resize(size);
				os << str;
			}
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
	struct object_parser : qi::grammar<Iterator, yak::pdf::object(), skip_normal_expr_type>
	{
		struct get_length_impl
		{
			template<typename A1>
			struct result { typedef int type; };
			int operator()(const dictionary& dic) const
			{
				return boost::get<int>(dic.find(yak::pdf::name("Length"))->second);
			}
		};
		boost::phoenix::function<get_length_impl> get_length;
		struct has_valid_length_impl
		{
			template<typename A1>
			struct result { typedef bool type; };
			bool operator()(const dictionary& dic) const
			{
				dictionary::const_iterator iter = dic.find(yak::pdf::name("Length"));
				return iter != dic.end() && boost::get<int>(&(iter->second));
			}
		};
		boost::phoenix::function<has_valid_length_impl> has_valid_length;

		object_parser() : object_parser::base_type(object, "object")
		{
			using qi::lit;
			using qi::char_;
			using qi::int_;
			using qi::skip;
			using qi::no_skip;
			using boost::phoenix::push_back;
			using namespace qi::labels;
			using yak::spirit::append;

			unesc_char.add
				((char*)"\\n", '\n')((char*)"\\r", '\r')((char*)"\\t", '\t')((char*)"\\b", '\b')
				((char*)"\\f", '\f')((char*)"\\(", '(')((char*)"\\)", ')')((char*)"\\\\",'\\')
			;
			eol_char.add((char*)"\r\n",'\n')((char*)"\r",'\n')((char*)"\n",'\n');
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
			object = indirect_ref | qi::bool_ | qi::real_parser<double, qi::strict_real_policies<double> >() | int_ | literal_string | hex_string | name_obj | array_obj | stream | dic_obj | null_obj;
			dic_obj = lit("<<") >> *(name_obj >> object) >> lit(">>");
			indirect_ref = int_ >> int_ >> lit('R');
			null_obj = lit("null")[_val=null()];
			stream %= dic_obj[_a=_1] >> lit("stream") >> no_skip[-lit('\r') >> lit('\n')] >> qi::lazy(boost::phoenix::if_else(has_valid_length(_a), stream_data(_a), stream_data_wo_length(_a)));
			stream_data = no_skip[qi::repeat(get_length(_r1))[qi::byte_]] >> (lit('\r') || lit('\n')) >> lit("endstream");
			stream_data_wo_length = no_skip[yak::spirit::delimited(std::string("endstream"))[qi::byte_]];

			// Name setting
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
			indirect_ref.name("indirect_ref");
			null_obj.name("null_obj");
			stream.name("stream");
			stream_data.name("stream_data");
			stream_data_wo_length.name("stream_data_wo_length");
		}
		qi::symbols<char const, char const> unesc_char;
		qi::symbols<char const, char const> eol_char;
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
		qi::rule<Iterator,yak::pdf::name()> name_obj; // qualificatoin needed by VC++10
		qi::rule<Iterator,char()> hex_char_name;
		qi::rule<Iterator,array(), skip_normal_expr_type> array_obj;
		qi::rule<Iterator,yak::pdf::object(), skip_normal_expr_type> object;
		qi::rule<Iterator,dictionary(), skip_normal_expr_type> dic_obj;
		qi::rule<Iterator,yak::pdf::indirect_ref(), skip_normal_expr_type> indirect_ref;
		qi::rule<Iterator,yak::pdf::null(), skip_normal_expr_type> null_obj;
		qi::rule<Iterator,yak::pdf::stream(), skip_normal_expr_type, qi::locals<dictionary> > stream;
		qi::rule<Iterator,std::vector<char>(const dictionary&)> stream_data;
		qi::rule<Iterator,std::vector<char>(const dictionary&)> stream_data_wo_length;
	};

	template <typename Iterator>
	struct pdf_parser : qi::grammar<Iterator, pdf_data(), skip_normal_expr_type>
	{
		struct get_dictionary_impl
		{
			template<typename A1>
			struct result { typedef const dictionary& type; };
			const dictionary& operator()(const object& obj) const
			{
				return boost::get<dictionary>(obj);
			}
		};
		boost::phoenix::function<get_dictionary_impl> get_dictionary;
		pdf_parser() : pdf_parser::base_type(pdf, "pdf")
		{
			using qi::lit;
			using qi::char_;
			using qi::int_;
			using qi::skip;
			using qi::no_skip;
			using namespace qi::labels;

			pdf = no_skip[lit("%PDF-") >> int_ >> lit('.') >> int_ >> *skip[indirect_obj] >> -skip[xref_section] >> skip[-trailer_dic >> trailer]];
			indirect_obj = int_ >> int_ >> lit("obj") >> object >> lit("endobj");
			xref_section = lit("xref") >> *xref_subsection;
			xref_subsection = int_ >> int_[_a = _1] >> qi::repeat(_a)[xref_entry];
			xref_entry = int_ >> int_ >> char_;
			trailer_dic = lit("trailer") >> object[_val = get_dictionary(_1)];
			trailer = lit("startxref") >> int_ >> skip(white_space)[lit("%%EOF")];

			// Name setting
			pdf.name("pdf");
			indirect_obj.name("indirect_obj");
			xref_section.name("xref_section");
			xref_subsection.name("xref_subsection");
			xref_entry.name("xref_entry");
			trailer_dic.name("trailer_dic");
			trailer.name("trailer");

//			debug(pdf);
		}
		qi::rule<Iterator,pdf_data(), skip_normal_expr_type> pdf;
		qi::rule<Iterator,yak::pdf::indirect_obj(), skip_normal_expr_type> indirect_obj;
		qi::rule<Iterator, void(), skip_normal_expr_type> xref_section;
		qi::rule<Iterator, skip_normal_expr_type, qi::locals<int> > xref_subsection;
		qi::rule<Iterator, skip_normal_expr_type> xref_entry;
		qi::rule<Iterator, dictionary(), skip_normal_expr_type> trailer_dic;
		qi::rule<Iterator, skip_normal_expr_type> trailer;
		object_parser<Iterator> object;
	};
	template <typename Iterator>
	bool parse_pdf(Iterator first, Iterator last, pdf_data &pd)
	{
		pdf_parser<Iterator> g;
		bool r = phrase_parse(first, last, g, skip_normal, pd);

		if (!r || first != last) // fail if we did not get a full match
			return false;
		return r;
	}
	bool parse_pdf_file(const std::string &name, pdf_data &pd)
	{
		using namespace boost::interprocess;

		file_mapping fm(name.c_str(), read_only);
		mapped_region region(fm, read_only);

		char* first = static_cast<char*>(region.get_address());
		std::size_t size = region.get_size();
		char* last = first + size;

		return parse_pdf(first, last, pd);
	}

}}

#endif // YAK_PDF_PDFPARSER_HPP
