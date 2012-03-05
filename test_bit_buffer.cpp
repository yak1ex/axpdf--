#include "bit_buffer.hpp"

#include <boost/test/auto_unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/assert.hpp>

///////////////////////////////////////////////////////////////////////

#include <iostream>

using boost::mpl::int_;

typedef boost::mpl::list<int_<1>, int_<2>, int_<3>, int_<4>, int_<5>, int_<6>, int_<7>, int_<8> > test_idx_types;

using boost::mpl::fold;
using boost::mpl::reverse_fold;
using boost::mpl::push_front;
using boost::mpl::is_sequence;
using namespace boost::mpl::placeholders;

template<typename T, typename Seq, typename Init = boost::mpl::list<> >
struct product_1_seq
{
	typedef typename fold<Seq, Init, push_front<_1, push_front<boost::mpl::if_<is_sequence<_2>, _2, push_front<boost::mpl::list<>, _2> >, T> > >::type type;
};

template<typename Seq1, typename Seq2, typename Init = boost::mpl::list<> >
struct product_seq_seq
{
	typedef typename fold<Seq1, Init, product_1_seq<_2, Seq2, _1> >::type type;
};

template<typename Seq>
struct product_seq
{
	typedef typename boost::mpl::size<Seq>::type Size;
	BOOST_MPL_ASSERT((boost::mpl::greater<Size, int_<1> >));
	typedef typename boost::mpl::begin<Seq>::type HeadBegin;
	typedef typename boost::mpl::advance<HeadBegin, typename boost::mpl::prior<Size>::type>::type HeadEnd;
	typedef typename boost::mpl::iterator_range<HeadBegin, HeadEnd>::type HeadList;
	typedef typename boost::mpl::deref<HeadEnd>::type Tail;
	typedef typename reverse_fold<HeadList, Tail, product_seq_seq<_2, _1> >::type type;
};

typedef boost::mpl::list<int_<0>, int_<1> > list1_type;
typedef boost::mpl::list<int_<2>, int_<3> > list2_type;
typedef boost::mpl::list<int_<4>, int_<5> > list3_type;
typedef boost::mpl::list<list1_type, list2_type, list3_type> list_type;
typedef product_seq<list_type>::type list_product_type;

BOOST_AUTO_TEST_CASE_TEMPLATE(test, T, list_product_type)
{
	BOOST_MPL_ASSERT((boost::mpl::greater<typename boost::mpl::size<T>::type, int_<2> >));
	typedef typename boost::mpl::at_c<T, 0>::type T1;
	typedef typename boost::mpl::at_c<T, 1>::type T2;
	typedef typename boost::mpl::at_c<T, 2>::type T3;
	BOOST_TEST_MESSAGE(T1() << ':' << T2() << ':' << T3());
}

typedef boost::mpl::list<unsigned char, unsigned short, unsigned int> types_types;
typedef product_seq_seq<types_types, test_idx_types>::type product_type;

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
