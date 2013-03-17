#include "bit_buffer.hpp"

#include <boost/test/auto_unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <boost/mpl/range_c.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/at.hpp>
#include "mpl_util.hpp"

///////////////////////////////////////////////////////////////////////

#include <iostream>

typedef boost::mpl::range_c<int, 1, 9> test_idx_types;

typedef boost::mpl::list<unsigned char, unsigned short, unsigned int> types_types;
typedef yak::util::mpl::product_seq_seq<types_types, test_idx_types>::type product_type;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_8bit_rotate, T, product_type)
{
	typedef typename boost::mpl::at_c<T, 0>::type test_type;
	typedef typename boost::mpl::at_c<T, 1>::type test_idx_type;
	int div = test_idx_type();

	bit_buffer<test_type> buf;
	BOOST_CHECK_EQUAL(buf.size(), 0);

	for(int i=0; i<16; ++i) {
		buf.push(0xAA, 8);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(8), 0xAA);
		BOOST_CHECK_EQUAL(buf.size(), 0);
		buf.push(0x1, div);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(div), 0x1);
		BOOST_CHECK_EQUAL(buf.size(), 0);

		buf.push(0x55, 8);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(8), 0x55);
		BOOST_CHECK_EQUAL(buf.size(), 0);
		buf.push(0x1, div);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(div), 0x1);
		BOOST_CHECK_EQUAL(buf.size(), 0);

		buf.push(0x5A, 8);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(8), 0x5A);
		BOOST_CHECK_EQUAL(buf.size(), 0);
		buf.push(0x1, div);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(div), 0x1);
		BOOST_CHECK_EQUAL(buf.size(), 0);

		buf.push(0xA5, 8);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(8), 0xA5);
		BOOST_CHECK_EQUAL(buf.size(), 0);
		buf.push(0x1, div);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(div), 0x1);
		BOOST_CHECK_EQUAL(buf.size(), 0);

		buf.push(0xAA, 8);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(8), 0xAA);
		BOOST_CHECK_EQUAL(buf.size(), 0);
		buf.push(0x1, div);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(div), 0x1);
		BOOST_CHECK_EQUAL(buf.size(), 0);

		buf.push(0x55, 8);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(8), 0x55);
		BOOST_CHECK_EQUAL(buf.size(), 0);
		buf.push(0x1, div);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(div), 0x1);
		BOOST_CHECK_EQUAL(buf.size(), 0);

		buf.push(0x5A, 8);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(8), 0x5A);
		BOOST_CHECK_EQUAL(buf.size(), 0);
		buf.push(0x1, div);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(div), 0x1);
		BOOST_CHECK_EQUAL(buf.size(), 0);

		buf.push(0xA5, 8);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(8), 0xA5);
		BOOST_CHECK_EQUAL(buf.size(), 0);
		buf.push(0x1, div);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(div), 0x1);
		BOOST_CHECK_EQUAL(buf.size(), 0);
	}
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_16bit_rotate, T, product_type)
{
	typedef typename boost::mpl::at_c<T, 0>::type test_type;
	typedef typename boost::mpl::at_c<T, 1>::type test_idx_type;
	int div = test_idx_type();

	bit_buffer<test_type> buf;

	for(int i=0; i<16; ++i) {
		BOOST_CHECK_EQUAL(buf.size(), 0);

		buf.push(1, 1);
		BOOST_CHECK_EQUAL(buf.size(), 1);
		buf.push(0, 1);
		BOOST_CHECK_EQUAL(buf.size(), 2);
		buf.push(1, 1);
		BOOST_CHECK_EQUAL(buf.size(), 3);
		buf.push(0, 1);
		BOOST_CHECK_EQUAL(buf.size(), 4);

		buf.push(0, 1);
		BOOST_CHECK_EQUAL(buf.size(), 5);
		buf.push(1, 1);
		BOOST_CHECK_EQUAL(buf.size(), 6);
		buf.push(0, 1);
		BOOST_CHECK_EQUAL(buf.size(), 7);
		buf.push(1, 1);
		BOOST_CHECK_EQUAL(buf.size(), 8);

		buf.push(1, 1);
		BOOST_CHECK_EQUAL(buf.size(), 9);
		buf.push(0, 1);
		BOOST_CHECK_EQUAL(buf.size(), 10);
		buf.push(1, 1);
		BOOST_CHECK_EQUAL(buf.size(), 11);
		buf.push(0, 1);
		BOOST_CHECK_EQUAL(buf.size(), 12);

		buf.push(0, 1);
		BOOST_CHECK_EQUAL(buf.size(), 13);
		buf.push(1, 1);
		BOOST_CHECK_EQUAL(buf.size(), 14);
		buf.push(0, 1);
		BOOST_CHECK_EQUAL(buf.size(), 15);
		buf.push(1, 1);
		BOOST_CHECK_EQUAL(buf.size(), 16);

		BOOST_CHECK_EQUAL(buf.template pop_force<unsigned short>(16), 0xA5A5);

		buf.push(1, div);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(div), 1);
	}
	BOOST_CHECK_EQUAL(buf.size(), 0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_32bit_rotate, T, product_type)
{
	typedef typename boost::mpl::at_c<T, 0>::type test_type;
	typedef typename boost::mpl::at_c<T, 1>::type test_idx_type;
	int div = test_idx_type();
	BOOST_TEST_MESSAGE("test_32bit_rotate with " << div << " sizeof(T) " << sizeof(test_type));

	bit_buffer<test_type> buf;

	for(int i=0; i<16; ++i) {
		BOOST_TEST_MESSAGE("test_32bit_rotate with " << div << " sizeof(T) " << sizeof(test_type) << " : " << i);
		BOOST_CHECK_EQUAL(buf.size(), 0);

		buf.push(1, 1);
		buf.push(0, 1);
		buf.push(1, 1);
		buf.push(0, 1);

		buf.push(0, 1);
		buf.push(1, 1);
		buf.push(0, 1);
		buf.push(1, 1);

		buf.push(1, 1);
		buf.push(0, 1);
		buf.push(1, 1);
		buf.push(0, 1);

		buf.push(0, 1);
		buf.push(1, 1);
		buf.push(0, 1);
		buf.push(1, 1);

		buf.push(0, 1);
		buf.push(1, 1);
		buf.push(0, 1);
		buf.push(1, 1);

		buf.push(1, 1);
		buf.push(0, 1);
		buf.push(1, 1);
		buf.push(0, 1);

		buf.push(0, 1);
		buf.push(1, 1);
		buf.push(0, 1);
		buf.push(1, 1);

		buf.push(1, 1);
		buf.push(0, 1);
		buf.push(1, 1);
		buf.push(0, 1);

		BOOST_CHECK_EQUAL(buf.size(), 32);
		BOOST_CHECK_EQUAL(buf.template pop_force<unsigned int>(32), 0xA5A55A5A);

		buf.push(1, div);
		BOOST_CHECK_EQUAL(buf.template pop_force<test_type>(div), 1);
	}
	BOOST_CHECK_EQUAL(buf.size(), 0);
}
