#ifndef YAK_PDF_UTIL_HPP
#define YAK_PDF_UTIL_HPP

namespace yak { namespace util {

	template<typename InputIterator1, typename InputIterator2>
	std::pair<InputIterator1, InputIterator2>
	safe_mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
	{
		while(first1 != last1 && first2 != last2 && *first1 == *first2) {
			++first1; ++first2;
		}
		return std::make_pair(first1, first2);
	}
	template<typename InputIterator1, typename InputIterator2, typename BinaryPredicate>
	std::pair<InputIterator1, InputIterator2>
	safe_mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, BinaryPredicate pred)
	{
		while(first1 != last1 && first2 != last2 && pred(*first1, *first2)) {
			++first1; ++first2;
		}
		return std::make_pair(first1, first2);
	}
	template<typename InputIterator1, typename InputIterator2>
	bool safe_equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
	{
		while(first1 != last1 && first2 != last2 && *first1 == *first2) {
			++first1; ++first2;
		}
		return first1 == last1 && first2 == last2;
	}
	template<typename InputIterator1, typename InputIterator2, typename BinaryPredicate>
	bool safe_equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, BinaryPredicate pred)
	{
		while(first1 != last1 && first2 != last2 && pred(*first1, *first2)) {
			++first1; ++first2;
		}
		return first1 == last1 && first2 == last2;
	}

}}

#endif // YAK_PDF_UTIL_HPP
