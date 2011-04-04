/***********************************************************************/
/*                                                                     */
/* reader.hpp: Source file for PDF reader                              */
/*                                                                     */
/*     Copyright (C) 2011 Yak! / Yasutaka ATARASHI                     */
/*                                                                     */
/*     This software is distributed under the terms of a zlib/libpng   */
/*     License.                                                        */
/*                                                                     */
/*     $Id$                 */
/*                                                                     */
/***********************************************************************/
#include "reader.hpp"
#include "parser.hpp"
#include "decoder.hpp"

namespace yak { namespace pdf {

	const object& pdf_reader::get(const indirect_ref& ref) const
	{
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

	void pdf_reader::read_xref(int offset) // TODO: consider exception safety
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

	void pdf_reader::set_offsets(const indirect_ref& ref) const
	{
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

	const object& pdf_reader::get_compressed(const indirect_ref& ref, const xref_entry &ent) const
	{
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

}}
