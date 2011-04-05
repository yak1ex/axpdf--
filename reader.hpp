/***********************************************************************/
/*                                                                     */
/* reader.hpp: Header file for PDF reader                              */
/*                                                                     */
/*     Copyright (C) 2011 Yak! / Yasutaka ATARASHI                     */
/*                                                                     */
/*     This software is distributed under the terms of a zlib/libpng   */
/*     License.                                                        */
/*                                                                     */
/*     $Id$                 */
/*                                                                     */
/***********************************************************************/
#ifndef YAK_PDF_READER_HPP
#define YAK_PDF_READER_HPP

#include <vector>
#include <map>
#include <stdexcept>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "types.hpp"
#include "util.hpp"

namespace yak { namespace pdf {

// Exception classes

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

// Reader class

	class pdf_reader
	{
		typedef const char* Iterator;
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
		const object& resolve(const object& obj) const
		{
			if(const indirect_ref *p = boost::get<indirect_ref>(&obj)) {
				return get(*p);
			} else {
				return obj;
			}
		}
		template<typename T>
		const T& resolve(const object& obj) const
		{
			return boost::get<T>(resolve(obj));
		}
		template<typename T>
		const T& resolve(const object& obj, const T& def) const
		{
			return boost::get<T>(&resolve(obj)) ? boost::get<T>(resolve(obj)) : def;
		}
		const object& resolve(const dictionary& dic, const name &n) const
		{
			if(dic.count(n)) {
				return resolve(dic.find(n)->second);
			} else throw std::runtime_error("Internal error: Can't get value from object.");
		}
		template<typename T>
		const T& resolve(const dictionary& dic, const name &n) const
		{
			return boost::get<T>(resolve(dic, n));
		}
		template<typename T>
		const T& resolve(const dictionary& dic, const name &n, const T& def) const
		{
			return boost::get<T>(&resolve(dic, n)) ? boost::get<T>(resolve(dic, n)) : def;
		}
		const object& get(const indirect_ref& ref) const;
		const object& get(int number, int generation = 0) const
		{
			return get(indirect_ref(number, generation));
		}
		template<typename T>
		const T& get(const indirect_ref& ref) const
		{
			return boost::get<T>(get(ref));
		}
		template<typename T>
		const T& get(int number, int generation = 0) const
		{
			return boost::get<T>(get(number, generation));
		}
		const dictionary& get_root() const
		{
			return get<dictionary>(get_value<indirect_ref>(xref.trailer_dic, name("Root")));
		}
		const dictionary& get_trailer() const
		{
			return xref.trailer_dic;
		}
		const xref_section& get_xref() const
		{
			return xref;
		}
	private:
		void read_xref(int offset);
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
		bool is_set_offsets(const indirect_ref& ref) const
		{
			return offsets.count(ref) != 0;
		}
		void set_offsets(const indirect_ref& ref) const;
		const object& get_compressed(const indirect_ref& ref, const xref_entry &ent) const;

		mutable std::map<indirect_ref, object> objects;
		mutable std::map<indirect_ref, std::vector<int> > offsets;
		xref_section xref;
		Iterator first, last;
	};

	class pdf_file_reader : public pdf_reader
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

#endif // YAK_PDF_READER_HPP
