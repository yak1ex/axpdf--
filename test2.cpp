#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/define_struct.hpp>

#include <string>

namespace yak { namespace spirit {

	// Dummy type for csv separator
	// Using to make use of original meta_create_sequence implementation
	struct csv_separator {};

	namespace detail {

		// extracted from boost/spirit/home/support/auto/meta_create.hpp (v1.46)
		template <typename T>
		struct is_fusion_sequence_but_not_proto_expr
		  : boost::mpl::and_<
				boost::fusion::traits::is_sequence<T>
			  , boost::mpl::not_<boost::proto::is_expr<T> > >
		{};
	}

}}

namespace boost { namespace spirit { namespace traits {

	// deep_copy may be unnecessary...

	// specialization for csv_separator
	template<>
	struct create_parser<yak::spirit::csv_separator>
	{
		typedef proto::result_of::deep_copy<
			BOOST_TYPEOF(qi::lit(','))
		>::type type;

		static type call()
		{
			return proto::deep_copy(qi::lit(','));
		}
	};

	// specialization for std::string
	template<>
	struct create_parser<std::string>
	{
		typedef proto::result_of::deep_copy<
			BOOST_TYPEOF(*(qi::char_ - qi::lit(',')))
		>::type type;

		static type call()
		{
			return proto::deep_copy(*(qi::char_ - qi::lit(',')));
		}
	};

	// specialization for Fusion Sequence
	// meta_create_sequence with slight modification
	template <typename Sequence>
	struct create_parser<Sequence, typename boost::enable_if<
				yak::spirit::detail::is_fusion_sequence_but_not_proto_expr<Sequence> 
			>::type>
	{

		// create a mpl sequence from the given fusion sequence
		typedef typename mpl::fold<
			typename fusion::result_of::as_vector<Sequence>::type
		  , mpl::vector<>, mpl::push_back<mpl::push_back<mpl::_1, mpl::_2>, yak::spirit::csv_separator>
		>::type sequence_type_;
		typedef typename mpl::if_<mpl::empty<sequence_type_>, sequence_type_, typename mpl::pop_back<sequence_type_>::type>::type sequence_type;

		typedef make_nary_proto_expr<
			sequence_type, proto::tag::shift_right, qi::domain
		> make_proto_expr;

		typedef typename make_proto_expr::type type;

		static type call()
		{
			return make_proto_expr::call();
		}
	};


}}}

// BOOST_FUSION_DEFINE_STRUCT has the same effect as struct definition and BOOST_FUSION_ADAPT_STRUCT.
BOOST_FUSION_DEFINE_STRUCT(
	(), // places global namespace
	data,
	(std::string, namae)
	(int, wanryoku)
	(int, kiyosa)
	(int, subayasa)
	(int, tairyoku)
	(int, maryoku)
	(int, seishin)
	(int, miryoku)
)

BOOST_FUSION_DEFINE_STRUCT(
	(), // places global namespace
	data2,
	(std::string, namae)
	(double, rate)
	(int, wanryoku)
	(int, kiyosa)
	(int, subayasa)
	(int, tairyoku)
	(int, maryoku)
	(int, seishin)
	(int, miryoku)
)

template<typename Attr>
void test_parse(const std::string &s, Attr &attr)
{
	namespace qi = boost::spirit::qi;

	typedef std::string::const_iterator Iterator;
	Iterator first = s.begin(), last = s.end();

	qi::phrase_parse(first, last, qi::auto_, qi::space, attr);
	boost::fusion::out(std::cout, attr); // output as fusion sequence
	std::cout << std::endl;
}

int main()
{
	data d;
	test_parse("Name1, 21, 12, 20, 17, 15, 19, 20", d);

	data2 d2;
	test_parse("Name1, 3.5, 21, 12, 20, 17, 15, 19, 20", d2);

	return 0;
}
