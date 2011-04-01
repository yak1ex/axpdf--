/***********************************************************************/
/*                                                                     */
/* spirit_helper.hpp: Header file for utility routine for Boost.Spirit */
/*     Written by Yak!                                                 */
/*                                                                     */
/*     $Id$                  */
/*                                                                     */
/***********************************************************************/
#ifndef YAK_SPIRIT_HELPER_HPP
#define YAK_SPIRIT_HELPER_HPP

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_attributes.hpp>
#include <boost/spirit/include/support_container.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#ifndef BOOST_SPIRIT_AUTO
// Extracted from http://svn.boost.org/svn/boost/trunk/libs/spirit/example/qi/typeof.cpp@68572
#define BOOST_SPIRIT_AUTO(domain_, name, expr)                                  \
    typedef boost::proto::result_of::                                           \
        deep_copy<BOOST_TYPEOF(expr)>::type name##_expr_type;                   \
    BOOST_SPIRIT_ASSERT_MATCH(                                                  \
        boost::spirit::domain_::domain, name##_expr_type);                      \
    BOOST_AUTO(name, boost::proto::deep_copy(expr));                            \

#endif /* BOOST_SPIRIT_AUTO */

namespace yak { namespace spirit {

	struct append_impl
	{
		// string
		template<typename A1, typename A2>
		struct result { typedef void type; };
		void operator()(std::string &s1, const std::string &s2) const
		{
			s1.append(s2);
		}
		// container but not string
		template<typename Out, typename In>
		void operator()(Out &out, const In &in) const
		{
			using namespace boost::spirit;
			typedef typename traits::container_iterator<In const>::type 
				iterator_type;
			iterator_type end = traits::end(in);
			for (iterator_type i = traits::begin(in); i != end; traits::next(i))
				traits::push_back(out, traits::deref(i));
		}
	};
	boost::phoenix::function<append_impl> append;

	struct stringize_impl
	{
		template<typename A1>
		struct result { typedef std::string type; };
		template<typename T>
		std::string operator()(const T& t) const
		{
			return boost::lexical_cast<std::string>(t);
		}
	};
	boost::phoenix::function<stringize_impl> stringize;

}}

namespace yak { namespace spirit {

	BOOST_SPIRIT_TERMINAL_EX(delimited)

	template<typename Subject, typename Delimiter>
	struct delimited_parser
		: boost::spirit::qi::unary_parser<delimited_parser<Subject, Delimiter> >
	{
		template<typename Context, typename Iterator>
		struct attribute
		{
			typedef typename
				boost::spirit::traits::build_std_vector<
					typename boost::spirit::traits::attribute_of<
						Subject, Context, Iterator>::type
				>::type
			type;
		};

		delimited_parser(Subject const &subject, Delimiter const &delimiter)
			: subject(subject), delimiter(delimiter) {}

		template<typename Iterator, typename Context, typename Skipper, typename Attribute>
		bool parse(Iterator &first, Iterator const &last,
			Context &context, Skipper const &skipper, Attribute &attribute) const
		{
			using namespace boost::spirit;

			typedef typename traits::container_value<Attribute>::type value_type;
			std::vector<value_type> temp;
			value_type temp_val = value_type();

			typedef typename traits::container_iterator<const Delimiter>::type iterator_type;
			iterator_type first_ = traits::begin(delimiter);
			iterator_type last_ = traits::end(delimiter);
			iterator_type cur_ = first_;
			std::size_t idx = 0;
			std::vector<std::size_t> pos;

			iterator_type anchor_ = first_;
			while(cur_ != last_) {
				++cur_; pos.push_back(idx); idx = 0;
				while(cur_ != last_ && anchor_ != last_ && *anchor_ == *cur_) {
					++anchor_; ++cur_; ++idx; pos.push_back(0);
				}
				anchor_ = first_;
			}

			idx = 0; cur_ = first_;
			while(first != last && cur_ != last_) {
				if(subject.parse(first, last, context, skipper, temp_val)) {
					while(1) {
						if(temp_val == *cur_) {
							traits::next(cur_); ++idx; break;
						} else if(idx == 0) {
							traits::push_back(temp, temp_val);
							break;
						} else {
							std::size_t new_idx = pos[idx];
							cur_ = first_;
							for(std::size_t i = 0; i < idx - new_idx; ++i, traits::next(cur_)) {
								traits::push_back(temp, *cur_);
							}
							cur_ = first_;;
							for(std::size_t i = 0; i < new_idx; ++i) {
								traits::next(cur_);
							}
							idx = new_idx;
						}
					}
				} else {
					return false;
				}
			}
			if(cur_ == last_) {
				BOOST_FOREACH(value_type const& v, temp)
				{
					traits::push_back(attribute, v);
				}
				return true;
			}
			return false;
		}
        template <typename Context>
        boost::spirit::info what(Context& context) const
        {
            return boost::spirit::info("delimited", subject.what(context));
        }

		Subject subject;
		Delimiter delimiter;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        delimited_parser& operator= (delimited_parser const&);
	};
}}

namespace boost { namespace spirit { namespace qi {

	template <typename Delimiter, typename Subject, typename Modifiers>
	struct make_directive<
		terminal_ex<yak::spirit::tag::delimited, fusion::vector1<Delimiter> >, Subject, Modifiers>
	{
		typedef yak::spirit::delimited_parser<Subject, Delimiter> result_type;

		template <typename Terminal>
		result_type operator()(
			Terminal const& term, Subject const& subject, unused_type) const
		{
			return result_type(subject, fusion::at_c<0>(term.args));
		}
	};

}}}

namespace boost { namespace spirit {

// Enabler
	template<typename Delimiter>
	struct use_directive<qi::domain,
		terminal_ex<yak::spirit::tag::delimited, fusion::vector1<Delimiter> > > : mpl::true_ {};

	template<>
	struct use_lazy_directive<qi::domain, yak::spirit::tag::delimited, 1 // arity
	> : mpl::true_ {};

}}

namespace boost { namespace spirit { namespace traits
{
    template <typename Subject, typename T>
    struct has_semantic_action<yak::spirit::delimited_parser<Subject, T> >
      : unary_has_semantic_action<Subject> {};
}}}

#endif /* YAK_SPIRIT_HELPER_HPP */
