#ifndef YAK_MPL_UTILITY_HPP
#define YAK_MPL_UTILITY_HPP

#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/apply.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/mpl/at.hpp> // iterator_range
#include <boost/mpl/deref.hpp>

namespace yak {
namespace util {
namespace mpl {

template<typename Arg1, typename Arg2, typename Pred = boost::is_same<boost::mpl::_1, boost::mpl::_2> >
struct equal_deep
{
	typedef typename boost::mpl::eval_if<
		boost::mpl::and_<boost::mpl::is_sequence<Arg1>, boost::mpl::is_sequence<Arg2> >,
		boost::mpl::equal<Arg1, Arg2, equal_deep<boost::mpl::_1, boost::mpl::_2, typename boost::mpl::lambda<Pred>::type> >,
		boost::mpl::eval_if<
			boost::mpl::or_<boost::mpl::is_sequence<Arg1>, boost::mpl::is_sequence<Arg2> >,
			boost::mpl::false_,
			boost::mpl::apply<Pred, Arg1, Arg2>
		>
	>::type type;
};

template<typename T, typename Seq, typename Init = boost::mpl::list<>, typename Entry = boost::mpl::list<> >
struct product_1_seq
{
	typedef typename boost::mpl::reverse_fold<
		Seq,
		Init,
		boost::mpl::push_front<
			boost::mpl::_1,
			boost::mpl::push_front<
				boost::mpl::if_<
					boost::mpl::is_sequence<boost::mpl::_2>,
					boost::mpl::_2,
					boost::mpl::push_front<Entry, boost::mpl::_2>
				>,
			T>
		>
	>::type type;
};

template<typename Seq1, typename Seq2, typename Init = boost::mpl::list<>, typename Entry = boost::mpl::list<>  >
struct product_seq_seq
{
	typedef typename boost::mpl::reverse_fold<
		Seq1,
		Init,
		product_1_seq<boost::mpl::_2, Seq2, boost::mpl::_1, Entry>
	>::type type;
};

template<typename Seq>
struct product_seqseq
{
private:
	typedef typename boost::mpl::size<Seq>::type Size;
	BOOST_MPL_ASSERT((boost::mpl::greater<Size, boost::mpl::int_<1> >));
	typedef typename boost::mpl::begin<Seq>::type HeadBegin;
	typedef typename boost::mpl::advance<HeadBegin, typename boost::mpl::prior<Size>::type>::type HeadEnd;
	typedef typename boost::mpl::iterator_range<HeadBegin, HeadEnd>::type HeadList;
	typedef typename boost::mpl::deref<HeadEnd>::type Tail;
public:
	typedef typename boost::mpl::reverse_fold<
		HeadList,
		Tail,
		product_seq_seq<boost::mpl::_2, boost::mpl::_1>
	>::type type;
};

} // namespace mpl
} // namespace util
} // namespace yak

#endif
