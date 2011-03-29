#include "types_output.hpp"

namespace yak { namespace pdf {

	void out(std::ostream &os, const object& obj, int level)
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
	std::ostream& operator<<(std::ostream &os, const xref_section &xs)
	{
		os << "trailer" << std::endl;
		yak::pdf::output_visitor ov(os);
		ov(xs.trailer_dic);

		os << "xref" << std::endl;
		for(xref_table::const_iterator it = xs.entries.begin(); it != xs.entries.end();++it) {
			os << it->first << ':' << it->second.type << ':' << it->second.offset << ':' << it->second.generation << std::endl;
		}

		return os;
	}
	std::ostream& operator<<(std::ostream &os, const dictionary &dic)
	{
		yak::pdf::output_visitor ov(os);
		ov(dic);

		return os;
	}

}}
