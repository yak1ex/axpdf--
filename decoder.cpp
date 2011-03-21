#include <fstream>
#include <iostream>
#include <iomanip>

#define BOOST_ZLIB_BINARY zlib

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/stream.hpp>

#include "decoder.hpp"

namespace yak { namespace pdf { namespace decoder {

	std::auto_ptr<std::istream> create_decoder(const yak::pdf::stream &s)
	{
		namespace io = boost::iostreams;

		std::auto_ptr<io::filtering_istream> pin(new io::filtering_istream);

		dictionary dummy;
		const dictionary& param = get_value(s.dic, name("DecodeParms"), dummy);
		if(get_value(param, name("Predictor"), 0) >= 10) {
			predicator_png pp(get_value(param, name("Columns"), 1));
			pin->push(pp);
		}

		if(has_value(s.dic, name("Filter"), name("FlateDecode"))) {
			io::zlib_decompressor zd;
			pin->push(zd);
		}

		io::array_source as(&s.data[0], s.data.size());
		pin->push(as);

		return pin;
	}

	void get_decoded_result(const yak::pdf::stream &st, std::string &s)
	{
		namespace io = boost::iostreams;

		std::auto_ptr<std::istream> pis = create_decoder(st);
		std::string r;
		io::stream< io::back_insert_device<std::string> > os(r);
		io::copy(*pis, os);
		s.swap(r);
	}

}}}
