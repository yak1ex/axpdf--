/***********************************************************************/
/*                                                                     */
/* decoder.cpp: Header file for stream decoder of PDF parser           */
/*     Written by Yak!                                                 */
/*                                                                     */
/*     $Id$                  */
/*                                                                     */
/***********************************************************************/
#ifndef YAK_PDF_DECODER_HPP
#define YAK_PDF_DECODER_HPP

#include <iosfwd>
#include <vector>
#include <utility>
#include <boost/iostreams/concepts.hpp> // input_filter
#include <boost/iostreams/operations.hpp> // get, EOF, WOULD_BLOCK

#include "types.hpp"

namespace yak { namespace pdf { namespace decoder {

	class predicator_png : public boost::iostreams::input_filter
	{
	public:
		predicator_png(std::size_t column) : mode(0), column_pos(column), buf(column), prev_val(0)
		{
		}
		template<typename Source>
		int get(Source &src)
		{
			int c = boost::iostreams::get(src);
			if(c == EOF || c == boost::iostreams::WOULD_BLOCK) return c;
			if(column_pos == buf.size()) {
				set_mode(c);
				return get(src);
			} else {
				switch(mode) {
				case 0: /* None */
					return set(c);
				case 1: /* Sub */
					return set(c + prev());
				case 2: /* Up */
					return set(c + up());
				case 3: /* Average */
					return set(c + (prev() + up()) / 2);
				case 4: /* Paeth */
					return set(c + paeth());
				default:
					return set(c);
				}
			}
		}
		template<typename Source>
		void close(Source &src)
		{
			column_pos = 0;
			buf.assign(buf.size(), 0);
		}
	private:
		void set_mode(unsigned char c)
		{
			mode = c;
			buf[column_pos-1] = prev_val;
			prev_val = 0;
			column_pos = 0;
		}
		unsigned char set(unsigned char c)
		{
			if(column_pos>0) buf[column_pos-1] = prev_val;
			++column_pos;
			prev_val = c;
			return c;
		}
		unsigned char prev() const
		{
			if(column_pos == 0) return 0;
			return prev_val;
		}
		unsigned char up() const
		{
			return buf[column_pos];
		}
		unsigned char prevup() const
		{
			if(column_pos < 1) return 0;
			return buf[column_pos-1];
		}
		int abs(int c1, int c2) const
		{
			return c1 < c2 ? c2 - c1 : c1 - c2;
		}
		unsigned char paeth()
		{
			int a = prev(), b = up(), c = prevup();
			int p = static_cast<int>(a) + b - c;
			int pa = abs(p, a);
			int pb = abs(p, b);
			int pc = abs(p, c);
			if(pa <= pb && pa <= pc) return a;
			else if(pb <= pc) return b;
			return c;
		}

		int mode;
		std::size_t column_pos;
		std::vector<unsigned char> buf;
		unsigned char prev_val;
	};

	std::auto_ptr<std::istream> create_decoder(const yak::pdf::stream &s);
	void get_decoded_result(const yak::pdf::stream &st, std::string &s);

}}}

#endif
