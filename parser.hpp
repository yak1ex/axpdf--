/***********************************************************************/
/*                                                                     */
/* parser.hpp: Header file for PDF parser                              */
/*                                                                     */
/*     Copyright (C) 2011 Yak! / Yasutaka ATARASHI                     */
/*                                                                     */
/*     This software is distributed under the terms of a zlib/libpng   */
/*     License.                                                        */
/*                                                                     */
/*     $Id$                 */
/*                                                                     */
/***********************************************************************/
#ifndef YAK_PDF_PDFPARSER_HPP
#define YAK_PDF_PDFPARSER_HPP

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_match.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/home/qi/nonterminal/debug_handler.hpp>
#include <boost/fusion/include/at_c.hpp>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <vector>
#include <string>
#include <iostream>

#include "spirit_helper.hpp"
#include "types.hpp"
#include "types_output.hpp"
#include "decoder.hpp"

namespace boost { namespace spirit { namespace traits {

	template <>
	struct is_container<yak::pdf::dictionary> : mpl::false_ {};

	// Conversion from yak::pdf::object to yak::pdf::dictionary
	template <>
	// Attrib, T, Enable
	struct assign_to_attribute_from_value<yak::pdf::dictionary, yak::pdf::object, void>
	{
		static void call(yak::pdf::object const& val, yak::pdf::dictionary& attr)
		{
			attr = boost::get<yak::pdf::dictionary>(val);
		}
	};

	// Conversion from yak::pdf::indirect_obj to yak::pdf::xref_section
	template <>
	// Attrib, T, Enable
	struct assign_to_attribute_from_value<yak::pdf::xref_section, yak::pdf::indirect_obj, void>
	{
		static int read_bin(std::istream& is, int size)
		{
			int value = 0;
			while(size-->0) {
				value = value * 256 + is.get();
			}
			return value;
		}
		static void call(yak::pdf::indirect_obj const& val, yak::pdf::xref_section& attr)
		{
			const yak::pdf::stream &st = boost::get<yak::pdf::stream>(val.value);
			std::auto_ptr<std::istream> pis = yak::pdf::decoder::create_decoder(st);
			yak::pdf::array index;
			if(has_key(st.dic, yak::pdf::name("Index"))) {
				index = yak::pdf::get_value<yak::pdf::array>(st.dic, yak::pdf::name("Index"));
			} else {
				index.push_back(0);
				index.push_back(yak::pdf::get_value<int>(st.dic, yak::pdf::name("Size")));
			}
			const yak::pdf::array &w = yak::pdf::get_value<yak::pdf::array>(st.dic, yak::pdf::name("W"));
			int w_type = boost::get<int>(w[0]), w_info1 = boost::get<int>(w[1]), w_info2 = boost::get<int>(w[2]);

			yak::pdf::array::iterator it = index.begin();
			while(it != index.end()) {
				int first = boost::get<int>(*it++);
				int size = boost::get<int>(*it++);
				for(int i = 0; i < size; ++i) {
					int type = read_bin(*pis, w_type);
					int offset = read_bin(*pis, w_info1);
					int generation = read_bin(*pis, w_info2);
					yak::pdf::xref_entry ent(static_cast<yak::pdf::xref_type>(type), offset, generation);
					attr.entries.insert(std::make_pair(first + i, ent));
				}
			}
			attr.trailer_dic = st.dic;
		}
	};

}}}

namespace yak { namespace pdf {

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
			stream %= dic_obj[_a=_1] >> lit("stream") >> no_skip[-lit('\r') >> lit('\n')] >> no_skip[qi::lazy(boost::phoenix::if_else(has_valid_length(_a), stream_data(_a), stream_data_wo_length(_a)))];
			// PDF specification states "There should be an end-of-line marker after the data and before endstream".
			// However, PDF includeing stream without an end-of-line marker exists and Adobe Reader can display such a file.
			stream_data = no_skip[qi::repeat(get_length(_r1))[qi::byte_]] >> -(lit('\r') || lit('\n')) >> lit("endstream");
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
	struct xref_parser : qi::grammar<Iterator, xref_section(), skip_normal_expr_type>
	{
		struct convert2xref_entry_impl
		{
			template<typename A1, typename A2, typename A3, typename A4>
			struct result { typedef std::pair<int, yak::pdf::xref_entry> type; };

			std::pair<int, yak::pdf::xref_entry> operator()(int n1, int n2, char c, int start) const
			{
				if(prev_start != start) {
					cur = prev_start = start;
				}
				return std::pair<int, yak::pdf::xref_entry>(cur++, yak::pdf::xref_entry(
					c == 'f' ? XREF_FREE : XREF_USED,
					n1,
					n2
				));
			}
			convert2xref_entry_impl(int &prev_start, int &cur) : prev_start(prev_start), cur(cur) {}
		private:
			int &prev_start;
			int &cur;
		};
		int prev_start;
		int cur;
		boost::phoenix::function<convert2xref_entry_impl> convert2xref_entry;

		xref_parser() : xref_parser::base_type(xref, "xref"), prev_start(-1), cur(-1), convert2xref_entry(convert2xref_entry_impl(prev_start, cur))
		{
			using qi::lit;
			using qi::int_;
			using qi::char_;
			using namespace qi::labels;
			namespace phx = boost::phoenix;

			xref = xref_stream | xref_section;

			xref_stream = indirect_obj;
			indirect_obj = int_ >> int_ >> lit("obj") >> object >> lit("endobj");

			xref_section %= lit("xref") >> *xref_subsection[phx::insert(phx::at_c<0>(_val), phx::begin(_1), phx::end(_1))] >> trailer_dic;
			xref_subsection = qi::omit[int_[_a = _1] >> int_[_b = _1]] >> qi::repeat(_b)[xref_entry(_a)];
			xref_entry = (int_ >> int_ >> char_)[_val=convert2xref_entry(_1, _2, _3, _r1)];
			trailer_dic = lit("trailer") >> object;

			xref.name("xref");
			xref_stream.name("xref_stream");
			indirect_obj.name("indirect_obj");
			xref_section.name("xref_section");
			xref_subsection.name("xref_subsection");
			xref_entry.name("xref_entry");
			trailer_dic.name("trailer_dic");

//			qi::debug(xref);
		}
		void reset()
		{
			prev_start = cur = -1;
		}
		qi::rule<Iterator,yak::pdf::xref_section(), skip_normal_expr_type> xref;
		qi::rule<Iterator,yak::pdf::xref_section(), skip_normal_expr_type> xref_stream;
		qi::rule<Iterator,yak::pdf::xref_section(), skip_normal_expr_type> xref_table;
		qi::rule<Iterator,yak::pdf::indirect_obj(), skip_normal_expr_type> indirect_obj;
		object_parser<Iterator> object;
		qi::rule<Iterator, yak::pdf::xref_section(), skip_normal_expr_type> xref_section;
		qi::rule<Iterator, yak::pdf::xref_table(), skip_normal_expr_type, qi::locals<int, int> > xref_subsection;
		qi::rule<Iterator, std::pair<int,yak::pdf::xref_entry>(int), skip_normal_expr_type> xref_entry;
		qi::rule<Iterator, dictionary(), skip_normal_expr_type> trailer_dic;
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
//			trailer_dic = lit("trailer") >> object[_val = get_dictionary(_1)];
			trailer_dic = lit("trailer") >> object;
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
