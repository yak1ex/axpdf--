#ifndef YAK_PDF_TYPES_HPP
#define YAK_PDF_TYPES_HPP

#include <boost/variant/recursive_variant.hpp>
#include <boost/variant/get.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <string>
#include <vector>
#include <map>

#if BOOST_VERSION <= 104601
namespace boost {
	struct recursive_variant_ {};
}
#endif

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
	inline bool operator==(const name& n1, const name &n2)
	{
		return n1.value == n2.value;
	}
	inline bool operator<(const name& n1, const name &n2)
	{
		return n1.value < n2.value;
	}
	inline std::ostream& operator<<(std::ostream& os, const name &n)
	{
		os << '/' << n.value; return os;
	}
	struct null {};
	inline std::ostream& operator<<(std::ostream& os, const null &n)
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

	enum xref_type {
		XREF_FREE, XREF_USED, XREF_COMPRESSED
	};

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

BOOST_FUSION_DEFINE_STRUCT(
	(yak)(pdf),
	xref_entry,
	(yak::pdf::xref_type, type)
	(int, generation)
	(int, offset)
)

namespace yak { namespace pdf {

	typedef std::map<int, xref_entry> xref_table;

}}


BOOST_FUSION_DEFINE_STRUCT(
	(yak)(pdf),
	xref_section,
	(yak::pdf::xref_table, entries)
	(yak::pdf::dictionary, trailer_dic)
)

namespace yak { namespace pdf {

	template<typename T>
	inline bool is_type(const object &obj)
	{
		return boost::get<T>(&obj) != 0;
	}
	template<typename T>
	inline bool has_value(const dictionary &dic, const name &n, const T& t)
	{
		return dic.count(n) &&
			boost::get<T>(&dic.find(n)->second) &&
			boost::get<T>(dic.find(n)->second) == t;
	}
	template<typename T>
	inline const T& get_value(const dictionary &dic, const name &n)
	{
		if(dic.count(n) && boost::get<T>(&dic.find(n)->second))
			return boost::get<T>(dic.find(n)->second);
		throw 1; // TODO: throw proper exception
	}
	template<typename T>
	inline const T& get_value(const dictionary &dic, const name &n, const T& def)
	{
		if(dic.count(n) && boost::get<T>(&dic.find(n)->second))
			return boost::get<T>(dic.find(n)->second);
		return def;
	}
	inline bool has_key(const dictionary &dic, const name &n)
	{
		return dic.count(n) != 0;
	}
	typedef std::vector<object> array;

}}

#endif // YAK_PDF_TYPES_HPP
