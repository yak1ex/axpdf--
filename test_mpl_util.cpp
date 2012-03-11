#include "mpl_util.hpp"

#include <boost/test/auto_unit_test.hpp>

#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/quote.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/long.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/vector.hpp>

///////////////////////////////////////////////////////////////////////

//typedef boost::mpl::quote2<boost::mpl::equal_to> equal_to;
typedef boost::mpl::equal_to<boost::mpl::_1, boost::mpl::_2> equal_to;

BOOST_AUTO_TEST_CASE(test_equal_deep_scalar)
{
	using boost::mpl::int_;
	using boost::mpl::long_;
	using yak::util::mpl::equal_deep;

	BOOST_CHECK( (equal_deep<int_<0>, int_<0> >::type::value));
	BOOST_CHECK(!(equal_deep<int_<0>, int_<1> >::type::value));
	BOOST_CHECK(!(equal_deep<int_<1>, int_<0> >::type::value));
	BOOST_CHECK( (equal_deep<int_<1>, int_<1> >::type::value));

	BOOST_CHECK(!(equal_deep<int_<0>, long_<0> >::type::value));
	BOOST_CHECK(!(equal_deep<int_<0>, long_<1> >::type::value));
	BOOST_CHECK(!(equal_deep<int_<1>, long_<0> >::type::value));
	BOOST_CHECK(!(equal_deep<int_<1>, long_<1> >::type::value));

	BOOST_CHECK( (equal_deep<int_<0>, long_<0>, equal_to>::type::value));
	BOOST_CHECK(!(equal_deep<int_<0>, long_<1>, equal_to>::type::value));
	BOOST_CHECK(!(equal_deep<int_<1>, long_<0>, equal_to>::type::value));
	BOOST_CHECK( (equal_deep<int_<1>, long_<1>, equal_to>::type::value));
}

BOOST_AUTO_TEST_CASE(test_equal_deep_list)
{
	using boost::mpl::int_;
	using boost::mpl::long_;
	using boost::mpl::list;
	using yak::util::mpl::equal_deep;

	typedef list<int_<1>, int_<2> > int_list2;
	typedef list<int_<1>, int_<2>, int_<3> > int_list3;
	typedef list<long_<1>, long_<2> > long_list2;
	typedef list<long_<1>, long_<2>, long_<3> > long_list3;

	BOOST_CHECK(!(equal_deep<int_<0>, int_list2>::type::value));
	BOOST_CHECK(!(equal_deep<int_list2, int_<0> >::type::value));

	BOOST_CHECK( (equal_deep<int_list2, int_list2>::type::value));
	BOOST_CHECK(!(equal_deep<int_list2, int_list3>::type::value));
	BOOST_CHECK(!(equal_deep<int_list3, int_list2>::type::value));
	BOOST_CHECK( (equal_deep<int_list3, int_list3>::type::value));

	BOOST_CHECK(!(equal_deep<int_list2, long_list2>::type::value));
	BOOST_CHECK(!(equal_deep<int_list2, long_list3>::type::value));
	BOOST_CHECK(!(equal_deep<int_list3, long_list2>::type::value));
	BOOST_CHECK(!(equal_deep<int_list3, long_list3>::type::value));

	BOOST_CHECK( (equal_deep<int_list2, long_list2, equal_to>::type::value));
	BOOST_CHECK(!(equal_deep<int_list2, long_list3, equal_to>::type::value));
	BOOST_CHECK(!(equal_deep<int_list3, long_list2, equal_to>::type::value));
	BOOST_CHECK( (equal_deep<int_list3, long_list3, equal_to>::type::value));
}

BOOST_AUTO_TEST_CASE(test_equal_deep_seq)
{
	using boost::mpl::int_;
	using boost::mpl::long_;
	using boost::mpl::list;
	using boost::mpl::vector;
	using yak::util::mpl::equal_deep;

	typedef list<int_<1>, int_<2> > int_list2;
	typedef list<int_<1>, int_<2>, int_<3> > int_list3;
	typedef list<long_<1>, long_<2> > long_list2;
	typedef list<long_<1>, long_<2>, long_<3> > long_list3;

	typedef vector<int_<1>, int_<2> > int_vector2;
	typedef vector<int_<1>, int_<2>, int_<3> > int_vector3;
	typedef vector<long_<1>, long_<2> > long_vector2;
	typedef vector<long_<1>, long_<2>, long_<3> > long_vector3;

	BOOST_CHECK( (equal_deep<int_list2, int_vector2>::type::value));
	BOOST_CHECK(!(equal_deep<int_list2, long_vector2>::type::value));
	BOOST_CHECK(!(equal_deep<int_list2, int_vector3>::type::value));
	BOOST_CHECK(!(equal_deep<int_list2, long_vector3>::type::value));

	BOOST_CHECK(!(equal_deep<int_list3, int_vector2>::type::value));
	BOOST_CHECK(!(equal_deep<int_list3, long_vector2>::type::value));
	BOOST_CHECK( (equal_deep<int_list3, int_vector3>::type::value));
	BOOST_CHECK(!(equal_deep<int_list3, long_vector3>::type::value));

	BOOST_CHECK( (equal_deep<int_list2, int_vector2, equal_to>::type::value));
	BOOST_CHECK( (equal_deep<int_list2, long_vector2, equal_to>::type::value));
	BOOST_CHECK(!(equal_deep<int_list2, int_vector3, equal_to>::type::value));
	BOOST_CHECK(!(equal_deep<int_list2, long_vector3, equal_to>::type::value));

	BOOST_CHECK(!(equal_deep<int_list3, int_vector2, equal_to>::type::value));
	BOOST_CHECK(!(equal_deep<int_list3, long_vector2, equal_to>::type::value));
	BOOST_CHECK( (equal_deep<int_list3, int_vector3, equal_to>::type::value));
	BOOST_CHECK( (equal_deep<int_list3, long_vector3, equal_to>::type::value));
}

BOOST_AUTO_TEST_CASE(test_list_seqseq)
{
	using boost::mpl::list;
	using boost::mpl::int_;
	using yak::util::mpl::equal_deep;

	typedef list<int_<0>, int_<1> > list1_type;
	typedef list<int_<2>, int_<3> > list2_type;
	typedef list<int_<4>, int_<5> > list3_type;
	typedef list<list1_type, list2_type, list3_type> list_type;
	typedef yak::util::mpl::product_seqseq<list_type>::type list_product_type;

	BOOST_CHECK((equal_deep<list_product_type,
		list<
			list<int_<0>, int_<2>, int_<4> >,
			list<int_<0>, int_<2>, int_<5> >,
			list<int_<0>, int_<3>, int_<4> >,
			list<int_<0>, int_<3>, int_<5> >,
			list<int_<1>, int_<2>, int_<4> >,
			list<int_<1>, int_<2>, int_<5> >,
			list<int_<1>, int_<3>, int_<4> >,
			list<int_<1>, int_<3>, int_<5> >
		>
	>::type::value));
}

