/***********************************************************************/
/*                                                                     */
/* parser.hpp: Header file for PDF parser                              */
/*     Written by Yak!                                                 */
/*                                                                     */
/*     $Id$                  */
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

#include <stdexcept>
#include <algorithm>

#include "spirit_helper.hpp"
#include "util.hpp"
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
	struct xref_parser : qi::grammar<Iterator, xref_section(), skip_normal_expr_type>
	{
		struct convert2xref_entry_impl
		{
			template<typename A1, typename A2, typename A3, typename A4>
			struct result { typedef std::pair<int, yak::pdf::xref_entry> type; };

			std::pair<int, yak::pdf::xref_entry> operator()(int n1, int n2, char c, int start) const
			{
				static int prev_start = -1;
				static int cur = -1;
				if(prev_start != start) {
					cur = prev_start = start;
				}
				return std::pair<int, yak::pdf::xref_entry>(cur++, yak::pdf::xref_entry(
					c == 'f' ? XREF_FREE : XREF_USED,
					n1,
					n2
				));
			}
		};
		boost::phoenix::function<convert2xref_entry_impl> convert2xref_entry;

		xref_parser() : xref_parser::base_type(xref, "xref")
		{
			using qi::lit;
			using qi::int_;
			using qi::char_;
			using namespace qi::labels;

			xref = xref_stream | xref_section;

			xref_stream = indirect_obj;
			indirect_obj = int_ >> int_ >> lit("obj") >> object >> lit("endobj");

			xref_section %= lit("xref") >> *xref_subsection[yak::spirit::append(boost::phoenix::at_c<0>(_val),_1)] >> trailer_dic;
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

	class invalid_pdf : public std::runtime_error
	{
	public:
		explicit invalid_pdf (const std::string& what_arg) : std::runtime_error(what_arg)
		{
		}
	};
	class unsupported_pdf : public std::runtime_error
	{
	public:
		explicit unsupported_pdf (const std::string& what_arg) : std::runtime_error(what_arg)
		{
		}
	};

	template<typename Iterator>
	class pdf_reader
	{
	protected:
		pdf_reader()
		{
		}
		void init(Iterator first_, Iterator last_)
		{
			first = first_;
			last = last_;
			int offset = get_xref_offset();
			read_xref(offset);
		}
	public:
		pdf_reader(Iterator first, Iterator last)
		{
			init(first, last);
		}
		const object& resolve(const object& obj) const {
			if(const indirect_ref *p = boost::get<indirect_ref>(&obj)) {
				return get(*p);
			} else {
				return obj;
			}
		}
		template<typename T>
		const T& resolve(const object& obj) const {
			return boost::get<T>(resolve(obj));
		}
		const object& resolve(const dictionary& dic, const name &n) const {
			if(dic.count(n)) {
				return resolve(dic.find(n)->second);
			} else throw std::runtime_error("Internal error: Can't get value from object.");
		}
		template<typename T>
		const T& resolve(const dictionary& dic, const name &n) const {
			return boost::get<T>(resolve(dic, n));
		}
		const object& get(const indirect_ref& ref) const {
			if(!objects.count(ref)) {
				xref_table::const_iterator i = xref.entries.find(ref.number);
				if(i == xref.entries.end()) {
					throw invalid_pdf("Non-existent object requested.");
				}
				const xref_entry& ent = i->second;
				if(ent.type == XREF_COMPRESSED) {
					return get_compressed(ref, ent);
				}
				if(ent.type == XREF_FREE) {
					throw invalid_pdf("Free object requested.");
				}
				Iterator first_(first + ent.offset);
				object_parser<Iterator> g;
				bool r = phrase_parse(first_, last, qi::omit[qi::int_ >> qi::int_] >> qi::lit("obj") >> g >> qi::lit("endobj"), skip_normal, objects[ref]);

				if (!r) throw invalid_pdf("Can't read object.");
			}
			return objects[ref];
		}
		const object& get(int number, int generation = 0) const {
			return get(indirect_ref(number, generation));
		}
		template<typename T>
		const T& get(const indirect_ref& ref) const {
			return boost::get<T>(get(ref));
		}
		template<typename T>
		const T& get(int number, int generation = 0) const {
			return boost::get<T>(get(number, generation));
		}
		const dictionary& get_root() const {
			return get<dictionary>(get_value<indirect_ref>(xref.trailer_dic, name("Root")));
		}
		const dictionary& get_trailer() const {
			return xref.trailer_dic;
		}
	private:
		void read_xref(int offset) // TODO: consider exception safety
		{
			xref_parser<Iterator> g;

			{
				Iterator first_(first + offset);
				bool r = phrase_parse(first_, last, g, skip_normal, xref);

				if (!r) throw invalid_pdf("Can't read xref section.");
			}

			if(has_key(xref.trailer_dic, name("Prev"))) {
				while(1) {
					xref_section xref_temp;
					Iterator first_(first + offset);

					bool r = phrase_parse(first_, last, g, skip_normal, xref_temp);

					if (!r) throw invalid_pdf("Can't read xref section.");

					xref.entries.insert(xref_temp.entries.begin(), xref_temp.entries.end());
					if(has_key(xref_temp.trailer_dic, name("Prev"))) {
						offset = resolve<int>(xref_temp.trailer_dic, name("Prev"));
					} else {
						break;
					}
				}
			}
		}
		typedef std::reverse_iterator<Iterator> RIterator;
		RIterator skip_ws(RIterator first, RIterator last)
		{
			while(first != last && 
				(*first == ' ' || *first == '\t' || *first == '\x0d' || *first == '\x0a')) {
				++first;
			}
			return first;
		}
		RIterator skip_nl(RIterator first, RIterator last)
		{
			while(first != last && 
				(*first == '\x0d' || *first == '\x0a')) {
				++first;
			}
			return first;
		}
		RIterator get_offset(RIterator first, RIterator last, int &index)
		{
			int unit = 1, result = 0;
			while(first != last && '0' <= *first && *first <= '9') {
				result += (*first - '0') * unit;
				++first;
				unit *= 10;
			}
			if(unit > 1) index = result;
			else throw invalid_pdf("No xref offset found");
			return first;
		}
		int get_xref_offset()
		{
			const std::string trailer_marker("%%EOF");
			const std::string startxref_marker("startxref");

			RIterator rfirst(last);
			RIterator rlast(first);

			int index;
			rfirst = skip_nl(rfirst, rlast);
			std::pair<RIterator, std::string::const_reverse_iterator> res
				= yak::util::safe_mismatch(rfirst, rlast, trailer_marker.rbegin(), trailer_marker.rend());
			if(res.first == rlast || res.second != trailer_marker.rend())
				throw invalid_pdf("No EOF marker found");
			rfirst = res.first;
			rfirst = skip_ws(rfirst, rlast);
			rfirst = get_offset(rfirst, rlast, index);
			rfirst = skip_ws(rfirst, rlast);
			res = yak::util::safe_mismatch(rfirst, rlast, startxref_marker.rbegin(), startxref_marker.rend());
			if(res.first == rlast || res.second != startxref_marker.rend())
				throw invalid_pdf("No startxref marker found");
			rfirst = res.first;
			return index;
		}
		bool is_set_offsets(const indirect_ref& ref) const {
			return offsets.count(ref) != 0;
		}
		void set_offsets(const indirect_ref& ref) const {
			const stream &s = get<stream>(ref);
			if(!has_value(s.dic, name("Type"), name("ObjStm")))
				throw invalid_pdf("Object stream reading requested for not object stream.");
			std::auto_ptr<std::istream> pis = yak::pdf::decoder::create_decoder(s);
			int n = get_value<int>(s.dic, name("N"));
			offsets[ref].resize(n);
			int dummy;
			for(int i = 0; i < n; ++i) {
				*pis >> dummy >> offsets[ref][i];
				offsets[ref][i] += get_value<int>(s.dic, name("First"));
			}
		}
		const object& get_compressed(const indirect_ref& ref, const xref_entry &ent) const {
			indirect_ref ref_comp(ent.offset, 0);
			if(!is_set_offsets(ref_comp)) set_offsets(ref_comp);

			const stream &s = get<stream>(ent.offset);
			std::string str;
			yak::pdf::decoder::get_decoded_result(s, str);
			typedef std::string::iterator Iterator2;
			object_parser<Iterator2> g;
			Iterator2 first_ = str.begin() + offsets.find(ref_comp)->second[ent.generation];
			Iterator2 last_ = str.end();
			bool r = phrase_parse(first_, last_, g, skip_normal, objects[ref]);

			if (!r) throw invalid_pdf("Can't read compressed object.");

			return objects[ref];
		}

		mutable std::map<indirect_ref, object> objects;
		mutable std::map<indirect_ref, std::vector<int> > offsets;
		xref_section xref;
		Iterator first, last;
	};

	template<typename Iterator>
	pdf_reader<Iterator> make_pdf_reader(Iterator first, Iterator last)
	{
		return pdf_reader<Iterator>(first, last);
	}

	class pdf_file_reader : public pdf_reader<const char*>
	{
	public:
		pdf_file_reader(const std::string& filename) : 
			fm(filename.c_str(), boost::interprocess::read_only),
			region(fm, boost::interprocess::read_only)
		{
			const char* first_ = static_cast<char*>(region.get_address());
			std::size_t size = region.get_size();
			const char* last_ = first_ + size;

			init(first_, last_);
		}
	private:
		boost::interprocess::file_mapping fm;
		boost::interprocess::mapped_region region;
	};

}}

#endif // YAK_PDF_PDFPARSER_HPP
