/***********************************************************************/
/*                                                                     */
/* types_output.hpp: Header file for stream output for PDF parser      */
/*     Written by Yak!                                                 */
/*                                                                     */
/*     $Id$                  */
/*                                                                     */
/***********************************************************************/
#ifndef YAK_PDF_TYPES_OUTPUT_HPP
#define YAK_PDF_TYPES_OUTPUT_HPP

#include <boost/variant/apply_visitor.hpp>

#include <iostream>
#include <utility>

#include "types.hpp"
#include "decoder.hpp"

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
			std::string str;
			decoder::get_decoded_result(s, str);
			bool flag = true;
			for(std::size_t i=0;i<(std::min)(20U, str.size());++i) {
				if(str[i] < 0x20U || str[i] > 0x7EU) {
					flag = false;
					break;
				}
			}
			if(flag) os << str;
			os << "endstream" << std::endl;
		}
	};
	void out(std::ostream &os, const object& obj, int level = 0);
	std::ostream& operator<<(std::ostream &os, const object& obj);
	std::ostream& operator<<(std::ostream &os, const std::vector<indirect_obj> &objs);
	std::ostream& operator<<(std::ostream &os, const pdf_data &pd);
	std::ostream& operator<<(std::ostream &os, const xref_section &xs);
	std::ostream& operator<<(std::ostream &os, const dictionary &dic);

}}

#endif // YAK_PDF_TYPES_OUTPUT_HPP
