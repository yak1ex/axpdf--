/***********************************************************************/
/*                                                                     */
/* parser.cpp: Source file for atcual parser part of PDF susie plugin  */
/*                                                                     */
/*     Copyright (C) 2011 Yak! / Yasutaka ATARASHI                     */
/*                                                                     */
/*     This software is distributed under the terms of a zlib/libpng   */
/*     License.                                                        */
/*                                                                     */
/*     $Id$                 */
/*                                                                     */
/***********************************************************************/

#include <windows.h>

#include <vector>
#include <set>
#include <utility>

#include <boost/foreach.hpp>

#include "Spi_api.h"

#include "reader.hpp"
#include "decoder.hpp"
#include "bmp_helper.hpp"
#define DEBUG
#include "odstream.hpp"
#include "types_output.hpp"

// TODO: Reconsider file separation

typedef std::pair<std::string, unsigned long> Key;
typedef std::vector<char> Data;
typedef std::pair<std::vector<SPI_FILEINFO>, std::vector<Data> > Value;

bool g_fDuplicate;

namespace {

inline int get_icc_components(const yak::pdf::pdf_reader &pr, const yak::pdf::object &cs)
{
	// NOTE: Assuming cs is obj, because ICCBased check should be passed
	const yak::pdf::array &csarr = boost::get<yak::pdf::array>(cs);
	const yak::pdf::stream &csstr = pr.resolve<yak::pdf::stream>(csarr[1]);
	return yak::pdf::get_value<int>(csstr.dic, yak::pdf::name("N"), 3); // Default: 3 (RGB)
}

inline bool is_rgb(const yak::pdf::pdf_reader &pr, const yak::pdf::object &cs)
{
	using yak::pdf::name;

	return is_value_or_array(cs, name("DeviceRGB")) ||
	       is_array_front(cs, name("CalRGB")) ||
	       (is_array_front(cs, name("ICCBased")) && get_icc_components(pr, cs) == 3);
}

inline bool is_gray(const yak::pdf::pdf_reader &pr, const yak::pdf::object &cs)
{
	using yak::pdf::name;

	return is_value_or_array(cs, name("DeviceGray")) ||
	       is_array_front(cs, name("CalGray")) ||
	       (is_array_front(cs, name("ICCBased")) && get_icc_components(pr, cs) == 1);
}

void CreateArchiveInfo_FlateDecode(
	const yak::pdf::pdf_reader &pr,
	const yak::pdf::stream &s,
	std::vector<SPI_FILEINFO> &v1,
	std::vector<Data> &v2
)
{
	using yak::pdf::name;
	using yak::pdf::array;
	using yak::pdf::stream;
	using yak::pdf::object;
	using yak::pdf::is_type;

	// TODO: support just falling back to PNG

	try {

		if(!has_key(s.dic, name("ColorSpace"))) throw yak::pdf::unsupported_pdf("ColorSpace is required.");
		yak::pdf::bmp_helper bh;
		const yak::pdf::object &cs = pr.resolve(s.dic, name("ColorSpace"));;
		if(is_rgb(pr, cs)) {
			bh.init_rgb(pr.resolve(s.dic, name("BitsPerComponent"), 8), pr.resolve<int>(s.dic, name("Width")), pr.resolve<int>(s.dic, name("Height")));
			std::string str;
			yak::pdf::decoder::get_decoded_result(s, str);
			bh.set_pixels_rgb(str.c_str());
		} else if(is_gray(pr, cs)) {
			bh.init_bw(pr.resolve(s.dic, name("BitsPerComponent"), 8), pr.resolve<int>(s.dic, name("Width")), pr.resolve<int>(s.dic, name("Height")));
			std::string str;
			yak::pdf::decoder::get_decoded_result(s, str);
			bh.set_pixels_bw_index(str.c_str());
		} else if(is_array_front(cs, name("Indexed")) && 
		          pr.resolve<array>(cs).size() == 4) { // Indexed colorspace
			const array &ar = pr.resolve<array>(cs);
			const object &cs2 = pr.resolve(ar[1]);
			const object &obj = pr.resolve(ar[3]);
			// TODO: shrink bits
			bh.init_index(pr.resolve(s.dic, name("BitsPerComponent"), 8), pr.resolve<int>(s.dic, name("Width")), pr.resolve<int>(s.dic, name("Height")));
			// TODO: maybe Filter is used for palette
			yak::debug::ods << cs2;
			if(is_rgb(pr, cs2)) {
				bh.set_index_rgb(is_type<stream>(obj) ? &boost::get<stream>(obj).data[0] : &boost::get<std::vector<char> >(obj)[0], pr.resolve<int>(ar[2]) + 1);
			} else if(is_gray(pr, cs2)) {
				bh.set_index_bw(is_type<stream>(obj) ? &boost::get<stream>(obj).data[0] : &boost::get<std::vector<char> >(obj)[0], pr.resolve<int>(ar[2]) + 1);
			} else {
				throw yak::pdf::unsupported_pdf("Unsupported colorspace is used for Indexed colorspace.");
			}
			std::string str;
			yak::pdf::decoder::get_decoded_result(s, str);
			bh.set_pixels_bw_index(str.c_str());
		} else {
			throw yak::pdf::unsupported_pdf("Unsuppoted ColorSpace.");
		}

		int length = bh.size();
		SPI_FILEINFO info = {
			{ 'F', 'L', 'A', 'T' },
			v1.size(),
			length,
			length
		};
		wsprintf(info.filename, "%08d.bmp", v1.size());
		v1.push_back(info);
		Data d1;
		v2.push_back(d1);
		v2.back().resize(length);
		bh.write(&v2.back()[0], length);
	} catch(std::exception &e) {
		OutputDebugString(e.what());
	}
}

INT CreateArchiveInfo(
	std::vector<SPI_FILEINFO> &v1,
	std::vector<Data> &v2,
	std::set<yak::pdf::indirect_ref> &set,
	const yak::pdf::pdf_reader &pr,
	const yak::pdf::dictionary &dic
)
{
	using yak::pdf::has_value;
	using yak::pdf::has_key;
	using yak::pdf::name;
	using yak::pdf::array;
	using yak::pdf::object;
	using yak::pdf::dictionary;
	using yak::pdf::indirect_ref;
	using yak::pdf::stream;

	if(has_value(dic, name("Type"), name("Pages"))) {
		const array & arr = pr.resolve<array>(dic, name("Kids"));
		BOOST_FOREACH(const object &obj, arr) {
			CreateArchiveInfo(v1, v2, set, pr, pr.resolve<dictionary>(obj));
		}
	} else if(has_value(dic, name("Type"), name("Page"))) {
		if(has_key(dic, name("Resources"))) {
			const dictionary &res = pr.resolve<dictionary>(dic, name("Resources"));
			if(has_key(res, name("XObject"))) {
				const dictionary & xobj = pr.resolve<dictionary>(res, name("XObject"));
				BOOST_FOREACH(const dictionary::value_type &v, xobj) {
					const indirect_ref &ref = boost::get<indirect_ref>(v.second);
					if(g_fDuplicate || !set.count(ref)) {
						const stream &s = pr.get<stream>(ref);
						if(has_value(s.dic, name("Type"), name("XObject")) && 
						   has_value(s.dic, name("Subtype"), name("Image"))) {
							if(has_value_or_array(s.dic, name("Filter"), name("DCTDecode"))) {
								int length = pr.resolve<int>(s.dic, name("Length"));
								SPI_FILEINFO info = {
									{ 'D', 'C', 'T' },
									v1.size(),
									length,
									length
								};
								wsprintf(info.filename, "%08d.jpg", v1.size());
								v1.push_back(info);
								Data d1;
								v2.push_back(d1); v2.back().assign(s.data.begin(), s.data.begin() + length);
							} else if(has_value_or_array(s.dic, name("Filter"), name("FlateDecode"))) {
								CreateArchiveInfo_FlateDecode(pr, s, v1, v2);
							}
						}
						set.insert(ref);
					}
				}
			}
		}
	} else {
		throw yak::pdf::invalid_pdf("Unknown type in page tree");
	}
	return SPI_ERR_NO_ERROR;
}

}

void GetArchiveInfoImp_(std::vector<SPI_FILEINFO> &v1, std::vector<std::vector<char> > &v2, LPSTR first, LPSTR last)
{
	yak::pdf::pdf_reader pr(first, last);
	if(yak::pdf::has_key(pr.get_trailer(), yak::pdf::name("Encrypt"))) {
		throw yak::pdf::unsupported_pdf("Protected PDF is not supported.");
	}
	std::set<yak::pdf::indirect_ref> set;
	CreateArchiveInfo(v1, v2, set, pr, pr.resolve<yak::pdf::dictionary>(pr.get_root(), yak::pdf::name("Pages")));
}
